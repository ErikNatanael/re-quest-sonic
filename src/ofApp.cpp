#include "ofApp.h"

void ofApp::saveFrame() {
  glReadBuffer(GL_FRONT);
  grabImg.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
  grabImg.save("screenshots/screenshot" + ofGetTimestampString() + ".png");
}

//--------------------------------------------------------------
void ofApp::setup() {
  WIDTH = ofGetWidth();
  HEIGHT = ofGetHeight();
  
  // must set makeContours to true in order to generate paths
  font.load("SourceCodePro-Regular.otf", 16, false, false, true);
  
  timeline.init(WIDTH, HEIGHT);
  timeline.parseProfile("profiles/software_art/scores/scripting_events.json");
  
  auto& tlFunctionMap = timeline.getFunctionMap(); // borrow the function map to create a map of FunctionDrawable
  auto& tlScripts = timeline.getScripts(); // borrow the script vector to create a vector of ScriptDrawable
  // create collections from these
  for(auto& s : tlScripts) {
    ScriptDrawable script;
    script.url = s.url;
    script.scriptId = s.scriptId;
    script.numFunctions = s.numFunctions;
    scripts.push_back(script);
  }
  
  for(auto& fPair : tlFunctionMap) {
    auto& f = fPair.second;
    FunctionDrawable func;
    func.id = f.id;
    func.name = f.name;
    func.scriptId = f.scriptId;
    func.calledTimes = f.calledTimes;
    func.lineNumber = f.lineNumber;
    func.columnNumber = f.columnNumber;
    functionMap.insert({func.id, func});
  }
  
  // go through all scripts and calculate their radii and positions
  bool redoAllPositions = false;
  do {
    redoAllPositions = false;
    for(auto& script : scripts) {
      script.radius = 20 + (sqrt(script.numFunctions) * 5);
      //ofLogNotice() << "radius: " << script.radius;
      script.pos = findNewScriptPosition(script.radius);
      //ofLogNotice() << "pos: " << script.pos;
      if(script.pos == glm::vec2(-1, -1)) {
        ofLog() << "Could not find position, redo all positions!";
        // could not find a suitable position, redo all positions
        redoAllPositions = true;
        break; // break out of the for loop and start over
      }
      script.generatePaths(font);
    }
    if(redoAllPositions) {
      // set all positions back to 0, 0 for recalculation
      for(auto& script : scripts) {
        script.pos = glm::vec2(0, 0);
      }
    }
  } while(redoAllPositions);
  
  // set all function positions depending on the script position
  for(auto& pair : functionMap) {
    auto& func = pair.second;
    // get script position
    glm::vec2 scriptPos;
    float maxRadius = 0;
    auto searchScript = find(scripts.begin(), scripts.end(), func.scriptId);
    if(searchScript != scripts.end()) {
      scriptPos = searchScript->pos;
      maxRadius = searchScript->radius;
    }
    // add a polar coordinate distance to the script position
    float angle = ofRandom(0, TWO_PI*4);
    float radius = ofRandom(10, maxRadius-10);
    func.pos = glm::vec2(scriptPos.x + (cos(angle)*radius), scriptPos.y + (sin(angle)*radius));
    func.boundCenter = scriptPos;
    func.boundRadius = maxRadius;
  }
    
  // ***************************** INIT openFrameworks STUFF
  ofBackground(0);
  ofSetFrameRate(60);
  ofEnableAlphaBlending();
  ofSetBackgroundAuto(false); //disable automatic background redraw
  ofSetCircleResolution(100);
  backgroundFbo.allocate(WIDTH, HEIGHT, GL_RGBA32F);
  foregroundFbo.allocate(WIDTH, HEIGHT, GL_RGBA32F);
  canvasFbo.allocate(WIDTH, HEIGHT, GL_RGBA32F);
  resultFbo.allocate(WIDTH, HEIGHT, GL_RGBA32F);
  renderFbo.allocate(WIDTH, HEIGHT, GL_RGB);
  focusShader.init();
  
  timeline.startThread(true);
}

//--------------------------------------------------------------
void ofApp::update(){
  
  
	//ofLog() << "fps: " << ofGetFrameRate();
}

//--------------------------------------------------------------
void ofApp::draw(){
  float dt = timeline.getNonScaledFramedt();
  float scaleddt = timeline.getFramedt();
  
  // move functions
  if(timeline.isPlaying()) {
    for(auto& pair : functionMap) {
      pair.second.update(dt);
    }
    for(auto& script : scripts) {
      script.update(dt);
    }
  }
  
  if(timeline.isPlaying()) {
    // draw the location of every function in the background
    backgroundFbo.begin();
      ofBackground(0);
      // for(auto& callPair : callMap) {
      //   ofFill();
      // 
      //   // Find the function connected to the function call
      //   auto searchFunc = functionMap.find(callPair.second.id);
      //   if(searchFunc != functionMap.end()) {
      //     auto& func = searchFunc->second;
      //     func.drawShapeBackground();
      //   }
      // }
      for(auto& fPair : functionMap) {
        auto& func = fPair.second;
        func.drawShapeBackground(camera2d);
      }
      // draw every script
      for(auto& script: scripts) {
        script.draw(camera2d);
      }
    backgroundFbo.end();
    
    
    foregroundFbo.begin();
    ofSetColor(0, 1);
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    foregroundFbo.end();
    
    // get the message queue from the timeline
    timeline.lock();
    // swap queues to get access to the full queue with only one lock
    messageFIFOLocal.swap(timeline.messageFIFO);
    timeline.unlock();
    
    for(auto& m : messageFIFOLocal) {
      if(m.type == "functionCall") {
        // assume it exists
        auto& func = functionMap[m.parameters["id"]];
        auto& parent = functionMap[m.parameters["parent"]];
        glm::vec2 acc = glm::normalize(parent.pos - func.pos)*20;
        func.acc = acc;
        func.activate();
        foregroundFbo.begin();
          func.drawForeground(parent, camera2d);
        foregroundFbo.end();
        backgroundFbo.begin();
          func.drawLineBackground(parent, camera2d);
        backgroundFbo.end();
        camera2d.targetPos = func.pos;
          
      } else if (m.type == "timelineReset") {
        // time cursor has reached the last event and has been reset
        // reset fbos
        foregroundFbo.begin();
        ofBackground(0, 0);
        foregroundFbo.end();
        backgroundFbo.begin();
        ofBackground(0);
        backgroundFbo.end();
        // reset all movement related things
        for(auto& fPair : functionMap) {
          auto& func = fPair.second;
          func.reset();
        }
      }
    }
    messageFIFOLocal.clear(); // clear the local queue in preparation for the next swap
    
  }
  
  camera2d.update(dt);
  
  if(!manualFocus) {
    focusShader.centerX = (camera2d.currentPos.x)/float(ofGetWidth());
    focusShader.centerY = (camera2d.currentPos.y)/float(ofGetHeight());
  }
  
  focusShader.density = ofClamp(pow(1.0-timeline.getTimeScale(), 6.0), 0.0, 1.0)+0.01;
  
  canvasFbo.begin();
    ofSetColor(255, 255);
    backgroundFbo.draw(0, 0);
    foregroundFbo.draw(0, 0);
  canvasFbo.end();
  if(doBlur) {
    focusShader.render(canvasFbo, resultFbo);
    if(rendering) renderFbo.begin();
    resultFbo.draw(0, 0);
    if(rendering) renderFbo.end();
  } else {
    if(rendering) renderFbo.begin();
    canvasFbo.draw(0, 0);
    if(rendering) renderFbo.end();
  }
  
  if(rendering) renderFbo.draw(0, 0);
  
  if(rendering) {
    // write frame to disk
    // glReadBuffer(GL_FRONT);
    // grabImg.grabScreen(0, 0 , ofGetWidth(), ofGetHeight());
    renderFbo.readToPixels(renderPixels);
    grabImg.setFromPixels(renderPixels);
    grabImg.save(renderDirectory + "frame-" + to_string(frameNumber) + ".png");
    frameNumber++;
    // move time forward by one frame time
    timeline.progressFrame();
  }
  
  // if(!rendering) timeline.draw();
  timeline.draw();
  
  // Alpha needs to be cleared in order to accurately capture with OBS or grabScreen()
  // see: https://forum.openframeworks.cc/t/different-transparency-and-colours-on-screen-in-recording/32784
  ofClearAlpha();
}

glm::vec2 ofApp::findNewScriptPosition(float radius) {
  // choose a new position at random
  // check if it's at least n pixels from any previous position
  glm::vec2 newPos;
  bool isGood = true;
  float minDistance = radius  + 50;
  const int margin = 100;
  int numTries = 0;
  const int maxTries = 500;
  
  do {
    newPos = glm::vec2(ofRandom(margin + radius, WIDTH-margin-radius), ofRandom(margin+radius, HEIGHT-margin-radius));
    isGood = true;
    for(auto& script : scripts) {
      float distance = glm::distance(newPos, script.pos);
      distance -= script.radius;
      if(distance < minDistance) {
        isGood = false;
        ++numTries;
        if(numTries >= maxTries) {
          return glm::vec2(-1, -1); // signal that a position could not be found
        }
        break;
      }
    }
  } while(!isGood);
  
  return newPos;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if(key == ' ') {
    timeline.togglePlay();
  } else if (key=='s') {
    saveFrame();
  } else if (key == 'm') {
    manualFocus = !manualFocus;
  } else if (key == 'b') {
    doBlur = !doBlur;
  } else if(key == 'r') {
    ofFileDialogResult result = ofSystemLoadDialog("Render folder", true, renderDirectory);
    if(result.bSuccess) {
      renderDirectory = result.getPath() + "/";
      rendering = true;
      timeline.startRendering();
    }
  }
}



//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
  for(auto& script : scripts) {
    // scripts cannot overlap so break if a match is found
    if(script.checkIfInside(glm::vec2(x, y), camera2d)) break;
  }
  if(manualFocus) {
    focusShader.centerX = float(x)/float(ofGetWidth());
    focusShader.centerY = float(y)/float(ofGetHeight());
  }
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
  timeline.click(x, y);

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){
  if(scrollY < 0) {
    timeline.reduceSpeed();
  } else if(scrollY > 0) {
    timeline.increaseSpeed();
  }
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

void ofApp::exit() {
  timeline.stopThread();
}