#pragma once

#include "ofMain.h"

class Script {
public:
  int scriptId;
  string url;
  int numFunctions = 0;
  vector<string> urlParts;
  string scriptType; // extension, built-in, remote
  string name; // e.g. "google" "gstatic" "kth"

  // stats of calls from this function (this function is parent)
  int numCallsWithinScript = 0;
  int numCallsToOtherScript = 0;
  // stats of calls to this function
  int numCalledFromOutsideScript = 0;

  map<uint32_t, uint32_t> fromScriptCounter;
  map<uint32_t, uint32_t> toScriptCounter;
  map<uint32_t, float> scriptInterconnectedness;
  
  // for mesh export
  float meshRadius = 0;
  glm::vec2 meshPos;
  
  Script() {
  }
  
  void extractInfo() {
    extractUrlParts();
    string delimiter;
    size_t pos;
    if(urlParts[0] == "http://" || urlParts[0] == "https://") {
      scriptType = "remote";
      // remove subdomain and top-level domain
      vector<string> domainParts;
      string domain = urlParts[1];
      delimiter = ".";
      while((pos = domain.find(delimiter)) != string::npos) {
        string part = domain.substr(0, pos);
        domain.erase(0, pos + delimiter.length());
        domainParts.push_back(part);
      }
      // last part will be the correct one (top-level domain is not added)
      name = domainParts[domainParts.size()-1];
    }
    else if(urlParts[0] == "chrome-extension://") {
      scriptType = "extension";
      // use filename as name
      if(urlParts.size() >=3) {
        name = urlParts[2];
      }
    }
    else  scriptType = "built-in";
    
    cout << "urlParts[0]: " << urlParts[0] << endl;
  }
  
  void extractUrlParts() {
    string urlCopy = url;
    // protocol
    // everything up until after the first two '/'
    string delimiter = "://";
    size_t pos = 0;
    if((pos = urlCopy.find(delimiter)) == string::npos) {
      // there is no protocol
      urlParts.push_back(urlCopy);
      return;
    } else {
      string protocol = urlCopy.substr(0, pos + delimiter.length());
      urlCopy.erase(0, pos + delimiter.length());
      urlParts.push_back(protocol);
    }
    
    // domain
    delimiter = "/";
    if((pos = urlCopy.find(delimiter)) != string::npos) {
      string domain = urlCopy.substr(0, pos + delimiter.length());
      urlCopy.erase(0, pos + delimiter.length());
      urlParts.push_back(domain);
    }
    
    // filename if applicable
    // if url ends in .js, extract this part, otherwise add all the rest
    size_t jsPos = 0;
    if((jsPos = urlCopy.find(".js")) != string::npos) {
      delimiter = "/";
      pos = url.rfind(delimiter);
      urlCopy.erase(0, pos + delimiter.length());
      urlParts.push_back(urlCopy);
    } else {
      urlParts.push_back(urlCopy);
      return;
    }
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
    cout << std::left << std::setw(12) << setfill(' ') << scriptType;
    cout << std::left << std::setw(10) << setfill(' ') << name;
    cout << std::left << std::setw(13) << setfill(' ') << numFunctions;
    cout << std::left << std::setw(15) << setfill(' ') << numCallsWithinScript;
    cout << std::left << std::setw(12) << setfill(' ') << numCallsToOtherScript;
    cout << std::left << std::setw(12) << setfill(' ') << numCalledFromOutsideScript;
    cout << std::left << std::setw(3) << setfill(' ') << url;
    cout << endl;
  }

  void printHeaders() {
    cout << std::left << std::setw(9) << setfill(' ') << "scriptId";
    cout << std::left << std::setw(12) << setfill(' ') << "scriptType";
    cout << std::left << std::setw(10) << setfill(' ') << "name";
    cout << std::left << std::setw(13) << setfill(' ') << "numFunctions";
    cout << std::left << std::setw(15) << setfill(' ') << "numCallsWithin";
    cout << std::left << std::setw(12) << setfill(' ') << "numCallsOut";
    cout << std::left << std::setw(12) << setfill(' ') << "numCalledIn";
    cout << std::left << std::setw(3) << setfill(' ') << "url";
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
  
  glm::vec2 getSpiralCoordinate(uint32_t maxScriptId, int HEIGHT) {
    // float distance = float(this->scriptId + 1)/float(maxScriptId + 1); // the distance along the spiral based on the scriptId
    // float angle = pow((1-distance) * TWO_PI, 3.0);
    // float radius = distance * ofGetHeight() * 0.4;
    float distance = float(this->scriptId)/float(maxScriptId * 2); // the distance along the spiral based on the scriptId
    float angle = (pow((1-distance), 2) + pow((1-distance)*1, 6.) * PI) * TWO_PI * 6;
    float radius = 0;
    if(distance!=0) radius = ( (1-pow((1-distance), 2.0) + 0.05 ) + sin((1-pow((1-distance), 2.0))*maxScriptId*2) * distance * 0.1 ) * HEIGHT * 0.55;
    float x = cos(angle) * radius;
    float y = sin(angle) * radius;
    return glm::vec2(x, y);
  }
  
  float getSize() {
    return float(ofClamp(sqrt(numFunctions)*6, 1, 100))/100.0;
  }
};

