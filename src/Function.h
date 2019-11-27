#pragma once

#include "ofMain.h"

class Function {
public:
  int id;
  string name;
  int scriptId;
  int calledTimes = 0;
  int callingTimes = 0; // how many times it is a parent to a function call
  int lineNumber;
  int columnNumber;

  // stats of calls from this function (this function is parent)
  int numCallsWithinScript = 0;
  int numCallsToOtherScript = 0;
  // stats of calls to this function
  int numCalledFromOutsideScript = 0;

  bool operator<(const Function& f) {
    return this->calledTimes > f.calledTimes;
  }

  bool operator-(const Function& f) {
    return this->calledTimes - f.calledTimes;
  }
  
  void print() {
    cout << std::left << std::setw(26) << setfill(' ') << name << " ";
    cout << std::left << std::setw(7) << setfill(' ') << id;
    cout << std::left << std::setw(12) << setfill(' ') << calledTimes;
    cout << std::left << std::setw(13) << setfill(' ') << callingTimes;
    cout << std::left << std::setw(21) << setfill(' ') << numCallsWithinScript;
    cout << std::left << std::setw(22) << setfill(' ') << numCallsToOtherScript;
    cout << std::left << std::setw(26) << setfill(' ') << numCalledFromOutsideScript;
    cout << endl;
  }

  void printHeaders() {
    cout << std::left << std::setw(26) << setfill(' ') << "Name" << " ";
    cout << std::left << std::setw(7) << setfill(' ') << "id";
    cout << std::left << std::setw(12) << setfill(' ') << "calledTimes";
    cout << std::left << std::setw(13) << setfill(' ') << "callingTimes";
    cout << std::left << std::setw(21) << setfill(' ') << "numCallsWithinScript";
    cout << std::left << std::setw(22) << setfill(' ') << "numCallsToOtherScript";
    cout << std::left << std::setw(26) << setfill(' ') << "numCalledFromOutsideScript";
    cout << endl;
  }
};