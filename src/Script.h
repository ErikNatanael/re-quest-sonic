#pragma once

#include "ofMain.h"

class Script {
public:
  int scriptId;
  string url;
  int numFunctions = 0;
  
  Script() {
  }

  bool operator<(const Script& s) {
    return this->numFunctions > s.numFunctions;
  }
  
  bool operator==(const int id) {
    return this->scriptId == id;
  }
};

