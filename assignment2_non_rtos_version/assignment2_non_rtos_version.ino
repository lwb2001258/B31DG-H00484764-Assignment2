#include <B31DGMonitor.h>  // Custom monitor for timing and violation tracking
#include <Ticker.h>        // Allows periodic function calls via timers

// -------------------- PIN DEFINITIONS --------------------
#define OUTPUT_PIN_1 26     // Digital output pin for Task 1
#define OUTPUT_PIN_2 27     // Digital output pin for Task 2
#define INPUT_PIN_F1 25     // Digital input pin to measure frequency F1 (Task 3)
#define INPUT_PIN_F2 33     // Digital input pin to measure frequency F2 (Task 4)
#define LED_PIN 17          // LED that lights up if F1 + F2 exceeds threshold
#define BUTTON_PIN 16       // Pushbutton input for triggering Task 5
#define BUTTON_LED_PIN 32   // LED toggled on button press
#define DEBOUNCE_DELAY 500  // Debounce time for button press (in ms)

// -------------------- OBJECT INITIALIZATION --------------------
B31DGCyclicExecutiveMonitor monitor;  // Real-time task monitor
Ticker ticker1;                       // Used to call slack time updater every 1 ms

// -------------------- FREQUENCY VARIABLES --------------------
unsigned long F1, F2, F;  // Frequencies for Task 3, Task 4, and their sum

// -------------------- EDGE DETECTION VARIABLES --------------------
unsigned long lastF1EdgeTime, lastF2EdgeTime;
volatile uint64_t lastButtonInterruptTime = 0;
static bool last_F1_input_state = LOW;
static bool last_F2_input_state = LOW;
volatile bool ButtonLedState = false;

// -------------------- TASK TIMING AND SCHEDULING --------------------
volatile int slackTimes[5] = { 3400, 2650, 7200, 7800, 4500 };      // Slack time (us) for each task
volatile int jobCounts[5] = { 0, 0, 0, 0, 0 };                      // Count of task completions
int executeTimes[5] = { 600, 350, 2800, 2200, 500 };                // Estimated execution time per task (us)
int periodList[5] = { 4000, 3000, 10000, 10000, 5000 };              // Task execution periods (us)
volatile bool doneList[5] = { false, false, false, false, false };  // Track whether each task has been completed in current cycle

// -------------------- SLACK TIME CALCULATION FUNCTION --------------------
void calculateSlackTime() {
  /*
    Updates slack time for each task based on current time.
    Slack = time until deadline - estimated execution time.
  */
  volatile unsigned long now = micros();
  volatile unsigned long totalTime = now - monitor.getTimeStart();

  for (int i = 0; i < 5; i++) {
    if ((int)(totalTime / periodList[i]) + 1 > jobCounts[i]) {
      doneList[i] = false;
      slackTimes[i] = periodList[i] - (totalTime % periodList[i]) - executeTimes[i];
    }
  }
}

// -------------------- BUTTON INTERRUPT HANDLER --------------------
void IRAM_ATTR buttonPressedHandle() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonInterruptTime > DEBOUNCE_DELAY) {
    ButtonLedState = !ButtonLedState;  // Toggle LED state
    monitor.doWork();                  // Perform background work
    lastButtonInterruptTime = currentTime;
  }
}

// -------------------- SETUP FUNCTION --------------------
void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;  // Wait for serial to be ready

  // Initialize pin modes
  pinMode(OUTPUT_PIN_1, OUTPUT);
  pinMode(OUTPUT_PIN_2, OUTPUT);
  pinMode(INPUT_PIN_F1, INPUT);
  pinMode(INPUT_PIN_F2, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);

  // Set initial pin states
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUTTON_LED_PIN, LOW);

  // Attach interrupt for button
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPressedHandle, RISING);

  // Start slack time update every 1 ms
  ticker1.attach_ms(1, calculateSlackTime);

  // Start the monitor
  monitor.startMonitoring();
}

// -------------------- MAIN LOOP --------------------
void loop() {
  // Set button LED based on button state
  digitalWrite(BUTTON_LED_PIN, ButtonLedState ? HIGH : LOW);

  volatile int jobIndex = 10;  // Invalid default value
  volatile int minSlackTime = 100000;

  // Select the task with the least slack that hasn't been executed yet
  for (int i = 0; i < 5; i++) {
    if (!doneList[i]) {
      if (slackTimes[i] < minSlackTime) {
        jobIndex = i;
        minSlackTime = slackTimes[i];
      }
    }
  }

  // Execute selected task
  switch (jobIndex) {
    case 0: JobTask1(); break;
    case 1: JobTask2(); break;
    case 2: JobTask3(); break;
    case 3: JobTask4(); break;
    case 4: JobTask5(); break;
    default: break;
  }
  if (jobIndex < 5) {
    doneList[jobIndex] = true;
    jobCounts[jobIndex]++;
  }
}

// -------------------- TASK DEFINITIONS --------------------

void JobTask1(void) {
  monitor.jobStarted(1);
  digitalWrite(OUTPUT_PIN_1, HIGH);
  delayMicroseconds(250);
  digitalWrite(OUTPUT_PIN_1, LOW);
  delayMicroseconds(50);
  digitalWrite(OUTPUT_PIN_1, HIGH);
  delayMicroseconds(300);
  digitalWrite(OUTPUT_PIN_1, LOW);
  monitor.jobEnded(1);
}

void JobTask2(void) {
  monitor.jobStarted(2);
  digitalWrite(OUTPUT_PIN_2, HIGH);
  delayMicroseconds(100);
  digitalWrite(OUTPUT_PIN_2, LOW);
  delayMicroseconds(50);
  digitalWrite(OUTPUT_PIN_2, HIGH);
  delayMicroseconds(200);
  digitalWrite(OUTPUT_PIN_2, LOW);
  monitor.jobEnded(2);
}

void JobTask3(void) {
  monitor.jobStarted(3);
  int count = 0;
  while (1) {
    bool input_state = digitalRead(INPUT_PIN_F1);
    if ((last_F1_input_state == LOW && input_state == LOW) || (last_F1_input_state == HIGH && input_state == HIGH) || (last_F1_input_state == HIGH && input_state == LOW)) {
      last_F1_input_state = input_state;
      continue;
    }
    if (input_state == HIGH && last_F1_input_state == LOW) {
      count++;
      last_F1_input_state = input_state;
      unsigned long now_edge = micros();
      unsigned long period = now_edge - lastF1EdgeTime;
      lastF1EdgeTime = now_edge;
      if (period > 0 && count > 1) {
        F1 = 1000000UL / period;
        F = F1 + F2;
        digitalWrite(LED_PIN, F > 1600 ? HIGH : LOW);
        break;
      }
    }
  }
  monitor.jobEnded(3);
}

void JobTask4(void) {
  monitor.jobStarted(4);
  int count = 0;
  while (1) {
    bool input_state = digitalRead(INPUT_PIN_F2);
    if ((last_F2_input_state == LOW && input_state == LOW) || (last_F2_input_state == HIGH && input_state == HIGH) || (last_F2_input_state == HIGH && input_state == LOW)) {
      last_F2_input_state = input_state;
      continue;
    }
    if (input_state == HIGH && last_F2_input_state == LOW) {
      count++;
      last_F2_input_state = input_state;
      unsigned long now_edge = micros();
      unsigned long period = now_edge - lastF2EdgeTime;
      lastF2EdgeTime = now_edge;
      if (period > 0 && count > 1) {
        F2 = 1000000UL / period;
        F = F1 + F2;  // (FIXED: previously F2 + F2)
        digitalWrite(LED_PIN, F > 1500 ? HIGH : LOW);
        break;
      }
    }
  }
  monitor.jobEnded(4);
}

void JobTask5(void) {
  monitor.jobStarted(5);
  monitor.doWork();  // Simulates ~500μs work
  monitor.jobEnded(5);
}
