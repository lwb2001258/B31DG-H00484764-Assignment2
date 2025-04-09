// the code for generate square wave for task3 and task4
#define PIN1 18   //the first pin for task3
#define PIN2 19   //the second pin for task4

// set the parameters for first pin
unsigned int freq1 = 666;
bool increasing1 = true;
unsigned long lastToggle1 = 0;
bool state1 = LOW;

// set the parameters for second pin
unsigned int freq2 = 833;
bool increasing2 = true;
unsigned long lastToggle2 = 0;
bool state2 = LOW;

void setup() {
    pinMode(PIN1, OUTPUT);
    pinMode(PIN2, OUTPUT);
}


void loop() {
    unsigned long now = micros();

    // first wave square wave
    unsigned long halfPeriod1 = 1000000UL / (2 * freq1); 
    if (now - lastToggle1 >= halfPeriod1) {
        lastToggle1 = now;
        state1 = !state1;
        digitalWrite(PIN1, state1);
    }

   //change the frequency of first pin
    static unsigned long lastFreqUpdate1 = 0;
    if (millis() - lastFreqUpdate1 >= 10) { // change 1Hz every 10ms
        lastFreqUpdate1 = millis();
        if (increasing1) {
            freq1++;
            if (freq1 >= 1000) increasing1 = false;
        } else {
            freq1--;
            if (freq1 <= 666) increasing1 = true;
        }
    }

    // second wave square wave
    unsigned long halfPeriod2 = 1000000UL / (2 * freq2);
    if (now - lastToggle2 >= halfPeriod2) {
        lastToggle2 = now;
        state2 = !state2;
        digitalWrite(PIN2, state2);
    }

       //change the frequency of second pin
    static unsigned long lastFreqUpdate2 = 0;
    if (millis() - lastFreqUpdate2 >= 10) { 
        lastFreqUpdate2 = millis();
        if (increasing2) {
            freq2++;
            if (freq2 >= 1500) increasing2 = false;
        } else {
            freq2--;
            if (freq2 <= 833) increasing2 = true;
        }
    }

}
