
#ifndef B31DGMonitor_h
#define B31DGMonitor_h

#include "Arduino.h"

#define NUMBER_TASKS 5
#define TEST_TIME 10000000

class B31DGCyclicExecutiveMonitor {

private:
// public:

    boolean bActive = false;

    unsigned long timeStart = 0;  
    unsigned long timeRelease[NUMBER_TASKS];
    unsigned long timeNextDeadlines[NUMBER_TASKS];

    unsigned long counterJobs[NUMBER_TASKS];
    unsigned long taskViolations[NUMBER_TASKS];
    unsigned long timeFirstEnd[NUMBER_TASKS];
    unsigned long timeFirstBegin[NUMBER_TASKS];

    unsigned long taskRateRequirements[NUMBER_TASKS] = 
         {    4000,  // 1 - digital output signal 1
              3000,  // 2 - digital output signal 2 
             10000,  // 3 - frequency monitor 1
             10000,  // 4 - frequency monitor 2
              5000,  // 5 - doWork
         };
     
    void printSummary();              

public:
    B31DGCyclicExecutiveMonitor(int offset);// declare one global instance of this class in your program, specify an offset (in microseconds) between the start of the monitoring and the start of your task (e.g. if scheduled using FreeRTOS)
    B31DGCyclicExecutiveMonitor() : B31DGCyclicExecutiveMonitor(0) {};
    void jobStarted(int task);    // Call this method whenever your program starts executing an instance of task #task
    void jobEnded(int task);      // Call this method whenever your program completed the execution of an instance of task #task
    unsigned long startMonitoring();   // Call this method once, i.e. in the setup/main function of your program, to activate the monitoring. It returns the time the monitor started to monitor
    unsigned long getTimeStart(); // Returns the time the monitor started to monitor (also returned by startMonitoring
    boolean isActive() {return bActive; }
    TickType_t getxStartTime();
    unsigned long offset;
    void doWork(); // waste CPU
};

#endif

