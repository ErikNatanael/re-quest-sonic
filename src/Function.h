#pragma once

#include "ofMain.h"

class Function {
public:
  int id;
  string name;
  int scriptId;
  int calledTimes = 0;
  int lineNumber;
  int columnNumber;
  
};