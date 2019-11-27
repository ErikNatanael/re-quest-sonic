#pragma once

#include "ofMain.h"

class ScriptDrawable {
public:
  glm::vec2 pos;
  float radius = 0;
  int scriptId;
  string url;
  int numFunctions = 0;
  
  bool focused = false;
  vector<ofPath> idPaths; // one polyline per character of the script Id'
  vector<vector<ofPath>> urlPaths;
  vector<string> urlParts;
  float halfNumChars = 0;
  float characterWidth = 0;
  float idStringWidth = 0;
  float urlCharacterWidth = 0;
  float urlCharacterHeight = 0;
  float urlStringWidth = 0;
  float textRotation = 0;
  float rotationVel = 0;
  // url
  
  ScriptDrawable() {
    textRotation = ofRandom(TWO_PI*4);
    rotationVel = ofRandom(-10, 10);
  }
  
  void generatePaths(ofTrueTypeFont font) {
    // get the string as paths
    bool vflip = true; // OF flips y coordinate in the default perspective, 
                       // should be false if using a camera for example
    bool filled = true; // or false for contours, must be false to create outline polylines
    idPaths = font.getStringAsPoints(to_string(scriptId), vflip, filled);
    int numChars = idPaths.size();
    halfNumChars = float(numChars-1)/2.;
    idStringWidth = font.stringWidth(to_string(scriptId));
    characterWidth = idStringWidth/float(numChars);
    
    extractUrlParts();
    
    for(auto& s : urlParts) {
      vector<ofPath> paths = font.getStringAsPoints(s, vflip, filled);
      urlPaths.push_back(paths);
    }
    urlStringWidth = font.stringWidth(urlParts[0]);
    urlCharacterHeight = font.stringHeight(urlParts[0]);
    urlCharacterWidth = urlStringWidth/urlPaths[0].size();
  }
  
  void extractUrlParts() {
    // protocol
    // everything up until after the first two '/'
    string delimiter = "://";
    size_t pos = 0;
    if((pos = url.find(delimiter)) == string::npos) {
      // there is no protocol
      urlParts.push_back(url);
      return;
    } else {
      string protocol = url.substr(0, pos + delimiter.length());
      url.erase(0, pos + delimiter.length());
      urlParts.push_back(protocol);
    }
    
    // domain
    delimiter = "/";
    if((pos = url.find(delimiter)) != string::npos) {
      string domain = url.substr(0, pos + delimiter.length());
      url.erase(0, pos + delimiter.length());
      urlParts.push_back(domain);
    }
    
    // filename if applicable
    // if url ends in .js, extract this part, otherwise add all the rest
    size_t jsPos = 0;
    if((jsPos = url.find(".js")) != string::npos) {
      delimiter = "/";
      pos = url.rfind(delimiter);
      url.erase(0, pos + delimiter.length());
      urlParts.push_back(url);
    } else {
      urlParts.push_back(url);
      return;
    }
    
  }
  
  void update(float dt) {
    textRotation += (rotationVel/radius) * dt;
    rotationVel += ofRandom(-0.1, 0.1) * dt;
    rotationVel = ofClamp(rotationVel, -10., 10.);
  }
  
  void draw(Camera2D& camera2d) {
    ofNoFill();
    if(urlParts[0] == "http://" || urlParts[0] == "https://") ofSetColor(70, 70, 200, 255);
    else if(urlParts[0] == "chrome-extension://") ofSetColor(120, 50, 150, 255);
    else  ofSetColor(110, 150, 50, 255);
    
    ofDrawCircle(pos, radius + 5);
    
    if(focused) {
      ofSetColor(90, 90, 255, 255);
      ofPushMatrix();
      
      ofTranslate(pos.x, pos.y);
      ofRotateRad(textRotation);
      ofTranslate(-idStringWidth*0.5, -radius-8); // move to the edge of the ring
      for (int i = 0; i < idPaths.size(); i++){
        idPaths[i].draw(0, 0);
        // for every character break it out to polylines
        // vector <ofPolyline> polylines = idPaths[i].getOutline();
        // ofLog() << "polylines.size(): " << polylines.size();
        // 
        // // for every polyline, draw every fifth point
        // for (int j = 0; j < polylines.size(); j++){
        //   for (int k = 0; k < polylines[j].size(); k+=5){         // draw every "fifth" point
        //     ofDrawCircle( polylines[j][k], 3);
        //   }
        // }
      }
      ofPopMatrix();
      
      // draw url
      for(int k = 0; k < urlPaths.size(); k++) {
        for(int i = 0; i < urlPaths[k].size(); i++) {
          // float maxLineLength = (radius * 1.8);
          float maxLineLength = 300;
          float lineInt;
          float lineFract = modf( (urlCharacterWidth*i) / maxLineLength, &lineInt);
          
          // int offsetx = maxLineLength*-1*lineInt;
          // int offsety = lineInt * urlCharacterHeight * 1.1;
          int offsetx = 0;
          int offsety = radius + urlCharacterHeight + urlCharacterHeight*k;
          urlPaths[k][i].draw((pos.x - (radius*0.9) + offsetx), (pos.y + offsety));
        }
      }
      
    }
  }
  
  bool checkIfInside(glm::vec2 pointerPos, Camera2D& camera2d) {
    if(glm::distance(pos, pointerPos) < radius) {
      focused = true;
      return true;
    } else {
      focused = false;
    }
    return false;
  }
  
  bool operator<(const ScriptDrawable& s) {
    return this->numFunctions > s.numFunctions;
  }
  
  bool operator==(const int id) {
    return this->scriptId == id;
  }
};

