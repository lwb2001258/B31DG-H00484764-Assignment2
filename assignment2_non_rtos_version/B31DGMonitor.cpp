#include "Arduino.h"
#include "B31DGMonitor.h"

B31DGCyclicExecutiveMonitor::B31DGCyclicExecutiveMonitor(int offset) {
  this->timeStart = micros();
  this->offset = offset;
}


unsigned long B31DGCyclicExecutiveMonitor::getTimeStart() {
  return this->timeStart;
}

unsigned long B31DGCyclicExecutiveMonitor::startMonitoring() {
  this->bActive = true;
  // find the next deadlines
  timeStart = micros() + offset;

  for (int i = 0; i < NUMBER_TASKS; i++) {
    this->timeFirstEnd[i] = 0;
    this->timeFirstBegin[i] = 0;
    this->timeRelease[i] = timeStart;
    this->counterJobs[i] = 0;
    this->taskViolations[i] = 0;
    this->timeNextDeadlines[i] = timeStart + this->taskRateRequirements[i];
  }

  return timeStart;
}

void B31DGCyclicExecutiveMonitor::jobStarted(int taskNumber) {
  if (!this->bActive) return;

  taskNumber--;

  unsigned long now = micros();
  if (this->timeFirstBegin[taskNumber] == 0) {
    this->timeFirstBegin[taskNumber] = now;
  }

  if ((now - this->timeStart) > TEST_TIME) {
    this->printSummary();
    exit(0);
  }

  if (now < this->timeRelease[taskNumber]) {

    taskViolations[taskNumber]++;
  }
}

void B31DGCyclicExecutiveMonitor::jobEnded(int taskNumber) {
  if (!this->bActive) return;

  unsigned long now = micros();

  taskNumber--;

  if (this->timeFirstEnd[taskNumber] == 0) {
    this->timeFirstEnd[taskNumber] = now;
  }

  if ((now - this->timeStart) > TEST_TIME) {
    this->printSummary();
    exit(0);
  }

  this->counterJobs[taskNumber]++;
  if (now > this->timeNextDeadlines[taskNumber]) {

    this->taskViolations[taskNumber]++;
  }

  this->timeRelease[taskNumber] = this->timeNextDeadlines[taskNumber];
  this->timeNextDeadlines[taskNumber] += this->taskRateRequirements[taskNumber];
}


void B31DGCyclicExecutiveMonitor::printSummary() {
  Serial.println("PERFORMANCE SUMMARY");
  Serial.printf("Start monitoring at ");
  Serial.println(this->timeStart);
  for (int i = 0; i < NUMBER_TASKS; i++) {
    Serial.print("Task #");
    Serial.print((i + 1));
    Serial.print("  ");
    Serial.print(taskViolations[i]);
    Serial.print("/");
    Serial.print(this->counterJobs[i]);
    Serial.print(" violations");
    Serial.print("  First job from ");
    Serial.print(this->timeFirstBegin[i]);
    Serial.print(" to ");
    Serial.println(this->timeFirstEnd[i]);
    Serial.flush(); 
  }
}


void B31DGCyclicExecutiveMonitor::doWork() {
  delayMicroseconds(500);
}
