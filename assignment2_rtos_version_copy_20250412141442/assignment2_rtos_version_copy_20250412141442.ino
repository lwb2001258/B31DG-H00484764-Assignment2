#include <B31DGMonitor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "esp_timer.h"
#include "esp_task_wdt.h"

// -------------------- PIN DEFINITIONS --------------------
#define OUTPUT_PIN_1 26
#define OUTPUT_PIN_2 27
#define INPUT_PIN_F1 25
#define INPUT_PIN_F2 33
#define LED_PIN 17
#define BUTTON_PIN 16
#define BUTTON_LED_PIN 32
#define DEBOUNCE_DELAY 500

B31DGCyclicExecutiveMonitor monitor;

// -------------------- SHARED VARIABLES --------------------
volatile unsigned long F1, F2, F, lastF1EdgeTime, lastF2EdgeTime;
volatile uint64_t lastButtonInterruptTime = 0;
static bool last_F1_input_state = LOW;
static bool last_F2_input_state = LOW;
volatile bool ButtonLedState = false;

volatile SemaphoreHandle_t mutex, mutex0;
TaskHandle_t jobHandles[7];
volatile int jobIndex = 10;
volatile bool jobDone = true;

static int jobIds0[] = { 0, 1, 4 };
static int jobIds1[] = { 2, 3 };
volatile int scheduledCount = 0;

// -------------------- TASK SCHEDULING PARAMETERS --------------------
volatile int slackTimes[5] = { 3400, 2650, 7200, 7800, 4500 };
volatile int jobCounts[5] = { 0, 0, 0, 0, 0 };
int executeTimes[5] = { 600, 350, 2800, 2200, 500 };
int periodList[5] = { 4000, 3000, 10000, 10000, 5000 };
volatile bool doneList[5] = { false, false, false, false, false };
int count = 0;

// -------------------- SLACK TIME CALCULATION FUNCTION --------------------
void updateSlackTime() {
  count += 1;
  unsigned long now = esp_timer_get_time();
  unsigned long totalTime = now - monitor.getTimeStart();
  for (int i = 0; i < 5; i++) {
    if ((int)(totalTime / periodList[i]) + 1 > jobCounts[i]) {
      doneList[i] = false;
      slackTimes[i] = periodList[i] - totalTime % periodList[i] - executeTimes[i];
    }
  }
  digitalWrite(BUTTON_LED_PIN, ButtonLedState ? HIGH : LOW);
}

// -------------------- BUTTON ISR --------------------
void IRAM_ATTR buttonPressedHandle() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonInterruptTime > DEBOUNCE_DELAY) {
    ButtonLedState = !ButtonLedState;
    monitor.doWork();
    lastButtonInterruptTime = currentTime;
  }
}

// -------------------- SLACK TIME UPDATER TASK --------------------
void slackTimeUpdate(void *pvParameters) {
  const TickType_t interval = pdMS_TO_TICKS(1);  // 1ms
  TickType_t lastWakeTime = xTaskGetTickCount();
  while (true) {
    updateSlackTime();
    vTaskDelayUntil(&lastWakeTime, interval);
  }
}

bool isInArray(int value, int arr[], int size) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == value) {
      return true;
    }
  }
  return false;
}


// -------------------- SCHEDULER TASK --------------------
void schedulerTask1(void *param) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    volatile int jobIndex;
    volatile int minSlack;
    jobIndex =10;
    minSlack =100000;
    for (int i = 0; i < sizeof(jobIds0) / sizeof(jobIds0[0]); i++) {
      if (!doneList[jobIds0[i]] && slackTimes[jobIds0[i]] < minSlack) {
        jobIndex = jobIds0[i];
        minSlack = slackTimes[jobIndex];
      }
    }
    // Serial.print("jobIndex:");
    // Serial.println(jobIndex);
    switch (jobIndex) {
      case 0: xTaskNotifyGive(jobHandles[0]);  break;
      case 1: xTaskNotifyGive(jobHandles[1]);   break;
      case 4: xTaskNotifyGive(jobHandles[4]); break;
      default:
        // Serial.println("default");
        xTaskNotifyGive(jobHandles[5]);
        break;
    }
  }
}

// -------------------- SCHEDULER TASK --------------------
void schedulerTask2(void *param) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
    volatile int jobIndex = 10;
    volatile int minSlack = 1000000;
    for (int i = 0; i < sizeof(jobIds1) / sizeof(jobIds1[0]); i++) {
      if (!doneList[jobIds1[i]] && slackTimes[jobIds1[i]] < minSlack) {
        jobIndex = jobIds1[i];
        minSlack = slackTimes[jobIndex];
      }
    }
    switch (jobIndex) {
      case 2: xTaskNotifyGive(jobHandles[2]); break;
      case 3: xTaskNotifyGive(jobHandles[3]); break;
      default:
        xTaskNotifyGive(jobHandles[6]);
        break;
    }
  }
}



// -------------------- SETUP FUNCTION --------------------
void setup() {
  Serial.begin(9600);
  pinMode(OUTPUT_PIN_1, OUTPUT);
  pinMode(OUTPUT_PIN_2, OUTPUT);
  pinMode(INPUT_PIN_F1, INPUT);
  pinMode(INPUT_PIN_F2, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUTTON_LED_PIN, LOW);

  mutex = xSemaphoreCreateMutex();
  mutex0 = xSemaphoreCreateBinary();


  xTaskCreatePinnedToCore(slackTimeUpdate, "slackTimeUpdate", 2048, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(JobTask1, "Job1", 4096, NULL, 1, &jobHandles[0], 1);
  xTaskCreatePinnedToCore(JobTask2, "Job2", 4096, NULL, 1, &jobHandles[1], 1);
  xTaskCreatePinnedToCore(JobTask3, "Job3", 4096, NULL, 3, &jobHandles[2], 0);
  xTaskCreatePinnedToCore(JobTask4, "Job4", 4096, NULL, 3, &jobHandles[3], 0);
  xTaskCreatePinnedToCore(JobTask5, "Job5", 4096, NULL, 1, &jobHandles[4], 1);
  xTaskCreatePinnedToCore(schedulerTask1, "schedulerTask1", 4096, NULL, 1, &jobHandles[5], 1);
  xTaskCreatePinnedToCore(schedulerTask2, "schedulerTask2", 4096, NULL, 1, &jobHandles[6], 1);
  xSemaphoreGive(mutex);
  xSemaphoreGive(mutex0);
  xTaskNotifyGive(jobHandles[5]);
  xTaskNotifyGive(jobHandles[6]);
  monitor.startMonitoring();
}

// -------------------- MAIN LOOP (IDLE) --------------------
void loop() {
  // Empty loop, all tasks are handled via FreeRTOS
}


void JobTask1(void *param) {
  while (1) {
    // Serial.println(micros());
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    // Serial.println(micros());
    monitor.jobStarted(1);
    digitalWrite(OUTPUT_PIN_1, HIGH);
    delayMicroseconds(250);  // HIGH for 250μs
    digitalWrite(OUTPUT_PIN_1, LOW);
    delayMicroseconds(50);  // LOW for 50μs
    digitalWrite(OUTPUT_PIN_1, HIGH);
    delayMicroseconds(300);  // HIGH for 300μs
    digitalWrite(OUTPUT_PIN_1, LOW);
    doneList[0] = true;
    jobCounts[0] = jobCounts[0] + 1;
    monitor.jobEnded(1);
    xTaskNotifyGive(jobHandles[5]);
    taskYIELD();
    xSemaphoreGive(mutex);
    // vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// Task 2, takes 1.8ms
void JobTask2(void *param) {
  while (1) {
    // Serial.println("task 2");
    unsigned long now = micros();
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // Serial.println("task 2 notify");
    // Serial.printf("notify:");
    // Serial.println(micros() - now);
    xSemaphoreTake(mutex, portMAX_DELAY);
    // Serial.printf("Semaphore:");
    // Serial.println(micros() - now);
    // Serial.println("task 2 Semaphor");
    // ex_num++;
    // Serial.printf("2:");
    // Serial.println(ex_num);
    // Serial.println(2);
    monitor.jobStarted(2);
    digitalWrite(OUTPUT_PIN_2, HIGH);
    delayMicroseconds(100);
    digitalWrite(OUTPUT_PIN_2, LOW);
    delayMicroseconds(50);
    digitalWrite(OUTPUT_PIN_2, HIGH);
    delayMicroseconds(200);
    digitalWrite(OUTPUT_PIN_2, LOW);
    doneList[1] = true;
    jobCounts[1] = jobCounts[1] + 1;
    monitor.jobEnded(2);
    xTaskNotifyGive(jobHandles[5]);
    taskYIELD();
    xSemaphoreGive(mutex);
    // vTaskDelay(pdMS_TO_TICKS(1));
  }
}


// Task 3, takes 1ms
void JobTask3(void *param) {
  while (1) {
    // Serial.println("task 3");
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // Serial.println("task 3 notify");
    // xSemaphoreTake(mutex, portMAX_DELAY);
    xSemaphoreTake(mutex0, portMAX_DELAY);
    // Serial.println("task 3 Semaphor");
    // ex_num++;
    // Serial.printf("3:");
    // Serial.println(ex_num);
    monitor.jobStarted(3);
    int count = 0;
    // unsigned long start = micros();
    while (1) {
      // taskENTER_CRITICAL(&myMux);
      bool input_state = digitalRead(INPUT_PIN_F1);
      if (last_F1_input_state == LOW && input_state == LOW) {
        last_F1_input_state = input_state;
        continue;
      } else if (last_F1_input_state == HIGH && input_state == HIGH) {
        last_F1_input_state = input_state;
        continue;
      } else if (last_F1_input_state == HIGH && input_state == LOW) {
        last_F1_input_state = input_state;
        continue;
      } else if (input_state == HIGH && last_F1_input_state == LOW) {
        count = count + 1;
        last_F1_input_state = input_state;
        unsigned long now_edge = esp_timer_get_time();
        ;
        unsigned long period = now_edge - lastF1EdgeTime;
        lastF1EdgeTime = now_edge;
        if (period > 0 && count > 1) {
          F1 = 1000000UL / period;
          F = F1 + F2;
          if (F > 1600) {
            digitalWrite(LED_PIN, HIGH);
          } else {
            digitalWrite(LED_PIN, LOW);
          }
          // taskEXIT_CRITICAL(&myMux);
          break;
        }
      }
    }
    doneList[2] = true;
    jobCounts[2] = jobCounts[2] + 1;
    monitor.jobEnded(3);
    xTaskNotifyGive(jobHandles[6]);
    // Serial.println("trigger schedule...");
    // taskYIELD();
    xSemaphoreGive(mutex0);
    // Serial.println("release mutex 0");

    // vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// Task 4, takes about 600-2200us
void JobTask4(void *param) {
  while (1) {
    // Serial.println("task 4");
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // Serial.println("task 4 notify");
    xSemaphoreTake(mutex0, portMAX_DELAY);
    // Serial.println("task 4 Semaphor");
    monitor.jobStarted(4);
    int count = 0;
    while (1) {
      // taskENTER_CRITICAL(&myMux);
      bool input_state = digitalRead(INPUT_PIN_F2);
      if (last_F2_input_state == LOW && input_state == LOW) {
        last_F2_input_state = input_state;
        continue;
      } else if (last_F2_input_state == HIGH && input_state == HIGH) {
        last_F2_input_state = input_state;
        continue;
      } else if (last_F2_input_state == HIGH && input_state == LOW) {
        last_F2_input_state = input_state;
        continue;
      } else if (input_state == HIGH && last_F2_input_state == LOW) {
        count = count + 1;
        last_F2_input_state = input_state;
        unsigned long now_edge = esp_timer_get_time();
        unsigned long period = now_edge - lastF2EdgeTime;
        lastF2EdgeTime = now_edge;
        if (period > 0 && count > 1) {
          F2 = 1000000UL / period;  // 计算测得的频率
          F = F2 + F2;
          if (F > 1500) {
            digitalWrite(LED_PIN, HIGH);
          } else {
            digitalWrite(LED_PIN, LOW);
          }
          break;
        }
      }
    }
    doneList[3] = true;
    jobCounts[3] = jobCounts[3] + 1;
    monitor.jobEnded(4);
    xTaskNotifyGive(jobHandles[6]);
    taskYIELD();
    xSemaphoreGive(mutex0);
    // vTaskDelay(pdMS_TO_TICKS(1));
  }
}


// Task 5, takes 0.5ms
void JobTask5(void *param) {
  while (1) {
    // Serial.println("task 5");
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // Serial.println("task 5 notify");
    xSemaphoreTake(mutex, portMAX_DELAY);
    // Serial.println(5);
    monitor.jobStarted(5);
    monitor.doWork();
    doneList[4] = true;
    jobCounts[4] = jobCounts[4] + 1;
    monitor.jobEnded(5);
    xTaskNotifyGive(jobHandles[5]);
    taskYIELD();
    xSemaphoreGive(mutex);
    // vTaskDelay
  }
}
