#include <B31DGMonitor.h>  // Custom monitor for timing and violation tracking
#include <Ticker.h>        // Allows periodic function calls via timers

// -------------------- PIN DEFINITIONS --------------------
#define OUTPUT_PIN_1 26      // Digital output pin for Task 1
#define OUTPUT_PIN_2 27      // Digital output pin for Task 2
#define INPUT_PIN_F1 25      // Digital input pin to measure frequency F1 (Task 3)
#define INPUT_PIN_F2 33      // Digital input pin to measure frequency F2 (Task 4)
#define LED_PIN 17           // LED that lights up if F1 + F2 exceeds threshold
#define BUTTON_PIN 16        // Pushbutton input for triggering Task 5
#define BUTTON_LED_PIN 32    // LED toggled on button press
#define DEBOUNCE_DELAY 200   // Debounce time for button press (in ms)
#define TOTAL_SLOTS 60       // 60 slots => 60 * 1ms = 120ms hyperperiod

// -------------------- OBJECT INITIALIZATION --------------------
B31DGCyclicExecutiveMonitor monitor;  // Real-time task monitor
Ticker ticker1;                       // Used to call slack time updater every 1 ms

// -------------------- FREQUENCY VARIABLES --------------------
unsigned long F1, F2, F;  // Frequencies for Task 3, Task 4, and their sum
volatile long lastButtonInterruptTime; // define the last interupt time for the button
volatile bool ButtonLedState=false; // initial the ButtonLedState to false;

// -------------------- BUTTON INTERRUPT HANDLER --------------------
void IRAM_ATTR buttonPressedHandle() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonInterruptTime > DEBOUNCE_DELAY) {
    ButtonLedState = !ButtonLedState;  // Toggle LED state
    // Set button LED based on button state
    digitalWrite(BUTTON_LED_PIN, ButtonLedState ? HIGH : LOW);
    monitor.doWork();  // Perform background work
    lastButtonInterruptTime = currentTime;
  }
}



volatile unsigned long frameCounter = 0;
// B31DGCyclicExecutiveMonitor monitor;
// int frame_count = 0;
void frame() {
  frameCounter++;
  unsigned int slot = frameCounter % TOTAL_SLOTS;
  switch (slot) {
    case 0:
      JobTask2();
      JobTask5();
      break;
    case 1:
      JobTask3();
      JobTask1();
      break;
    case 2: break;
    case 3: break;
    case 4: JobTask2(); break;
    case 5:
      JobTask4();
      JobTask1();
      break;
    case 6: break;
    case 7: break;
    case 8:
      JobTask2();
      JobTask5();
      break;
    case 9: JobTask1(); break;
    case 10: JobTask2(); break;
    case 11:
      JobTask5();
      JobTask3();
      break;
    case 12: break;
    case 13: JobTask2(); break;
    case 14: JobTask1(); break;
    case 15: JobTask4(); break;
    case 16: break;
    case 17: JobTask2(); break;
    case 18: JobTask1(); break;
    case 19: JobTask5(); break;
    case 20:
      JobTask2();
      JobTask4();
      break;
    case 21: break;
    case 22:
      JobTask1();
      break;
    case 23:
      JobTask2();
      JobTask5();
      break;
    case 24: JobTask1(); break;
    case 25:
      JobTask2();
      JobTask3();
      break;
    case 26: break;
    case 27: break;
    case 28:
      JobTask2();
      JobTask5();
      break;
    case 29: JobTask1(); break;
    case 30:
      JobTask2();
      JobTask3();
      break;
    case 31: break;
    case 32: break;
    case 33:
      JobTask2();
      JobTask5();
      break;
    case 34: JobTask1(); break;
    case 35: JobTask4(); break;
    case 36: break;
    case 37: JobTask2(); break;
    case 38: JobTask1(); break;
    case 39:
      JobTask2();
      JobTask5();
      break;
    case 40:
      JobTask1();
      JobTask3();
      break;
    case 41: break;
    case 42: break;
    case 43:
      JobTask2();
      JobTask5();
      break;
    case 44: JobTask1(); break;
    case 45:
      JobTask2();
      JobTask5();
      break;
    case 46: JobTask4(); break;
    case 47: break;
    case 48: JobTask2(); break;
    case 49: JobTask1(); break;
    case 50: JobTask5(); break;
    case 51:
      JobTask2();
      JobTask3();
      break;
    case 52: break;
    case 53: break;
    case 54: JobTask1(); break;
    case 55:
      JobTask2();
      JobTask5();
      break;
    case 56: JobTask4(); break;
    case 57: JobTask1(); break;
    case 58: JobTask2(); break;
    case 59:
      break;
    default:break;
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



  ticker1.attach_ms(1, frame);
  // Start the monitor
  monitor.startMonitoring();
  JobTask2();
  JobTask5();
}

// -------------------- MAIN LOOP --------------------
void loop() {
  
  
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
  unsigned long duration_f1 = pulseIn(INPUT_PIN_F1, HIGH); 
  F1 = 1000000.0 / (2 * duration_f1);
  F = F1 + F2;
  digitalWrite(LED_PIN, F > 1500 ? HIGH : LOW);
  monitor.jobEnded(3);
}

void JobTask4(void) {
  monitor.jobStarted(4);
  unsigned long duration_f2 = pulseIn(INPUT_PIN_F2, HIGH);  
  F2 = 1000000.0 / (2 * duration_f2);
  F = F1 + F2;
  digitalWrite(LED_PIN, F > 1500 ? HIGH : LOW);
  monitor.jobEnded(4);
}


void JobTask5(void) {
  monitor.jobStarted(5);
  monitor.doWork();  // Simulates ~500μs work
  monitor.jobEnded(5);
}
