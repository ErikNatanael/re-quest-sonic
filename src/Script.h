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

  map<uint32_t, uint32_t> fromScriptCounter;
  map<uint32_t, uint32_t> toScriptCounter;
  map<uint32_t, float> scriptInterconnectedness;
  
  Script() {
  }

  bool operator<(const Script& s) {
    return this->numFunctions > s.numFunctions;
  }
  
  bool operator==(const int id) {
    return this->scriptId == id;
  }

  void calculateInterconnectedness() {
    scriptInterconnectedness.clear();
    map<uint32_t, uint32_t> totalScriptCounter;
    float totalCalls = 0;
    // sum counters together into one
    for(auto& n : fromScriptCounter) {
      auto search = totalScriptCounter.find(n.first);
      if(search == totalScriptCounter.end()) {
        totalScriptCounter.insert({n.first, n.second});
      } else {
        search->second += n.second;
      }
      totalCalls += n.second;
    }
    for(auto& n : toScriptCounter) {
      auto search = totalScriptCounter.find(n.first);
      if(search == totalScriptCounter.end()) {
        totalScriptCounter.insert({n.first, n.second});
      } else {
        search->second += n.second;
      }
      totalCalls += n.second;
    }

    // turn into ratios of the whole
    for(auto& n : totalScriptCounter) {
      scriptInterconnectedness.insert({n.first, float(n.second)/float(totalCalls)});
    }

  }

  void print() {
    cout << std::left << std::setw(9) << setfill(' ') << scriptId;
    cout << std::left << std::setw(13) << setfill(' ') << numFunctions;
    cout << std::left << std::setw(15) << setfill(' ') << numCallsWithinScript;
    cout << std::left << std::setw(12) << setfill(' ') << numCallsToOtherScript;
    cout << std::left << std::setw(12) << setfill(' ') << numCalledFromOutsideScript;
    cout << std::left << std::setw(3) << setfill(' ') << url;
    cout << endl;
  }

  void printToAndFrom() {
    // std::sort (fromScriptCounter.begin(), fromScriptCounter.end());
    // std::sort (toScriptCounter.begin(), toScriptCounter.end());
    cout << "   this script is called from the following scripts: " << endl;
    for(auto& n : fromScriptCounter) {
      cout << n.first << ": " << n.second << endl;
    }
    cout << "   this script calls to the following scripts: " << endl;
    for(auto& n : toScriptCounter) {
      cout << n.first << ": " << n.second << endl;
    }
    cout << "   interconnectedness with script: " << endl;
    for(auto& n : scriptInterconnectedness) {
      cout << n.first << ": " << n.second << endl;
    }
    cout << endl;
  }

  void printHeaders() {
    cout << std::left << std::setw(9) << setfill(' ') << "scriptId";
    cout << std::left << std::setw(13) << setfill(' ') << "numFunctions";
    cout << std::left << std::setw(15) << setfill(' ') << "numCallsWithin";
    cout << std::left << std::setw(12) << setfill(' ') << "numCallsOut";
    cout << std::left << std::setw(12) << setfill(' ') << "numCalledIn";
    cout << std::left << std::setw(3) << setfill(' ') << "url";
    cout << endl;
  }
};

