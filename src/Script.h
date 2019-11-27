#pragma once

#include "ofMain.h"

class Script {
public:
  int scriptId;
  string url;
  int numFunctions = 0;

  // stats of calls from this function (this function is parent)
  int numCallsWithinScript = 0;
  int numCallsToOtherScript = 0;
  // stats of calls to this function
  int numCalledFromOutsideScript = 0;
  
  Script() {
  }

  bool operator<(const Script& s) {
    return this->numFunctions > s.numFunctions;
  }
  
  bool operator==(const int id) {
    return this->scriptId == id;
  }
};

