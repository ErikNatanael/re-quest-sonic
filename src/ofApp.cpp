#include "ofApp.h"
#include <filesystem>
namespace fs = std::filesystem;

//--------------------------------------------------------------
void ofApp::setup() {
  WIDTH = ofGetWidth();
  HEIGHT = ofGetHeight();
  
  // must set makeContours to true in order to generate paths
  font.load("SourceCodePro-Regular.otf", 16, false, false, true);
  
  string profilePath = "profiles/software_art/";
  
  timeline.init(WIDTH, HEIGHT);
  timeline.parseScriptingProfile(profilePath + "scores/scripting_events.json");
  timeline.parseUserEventProfile(profilePath + "scores/user_events.json");
  timeline.sendBackgroundInfoOSC();
  timeline.generateScore();
  
  // load all screenshot images
  uint64_t firstts = timeline.getFirstts();
  //some path, may be absolute or relative to bin/data
  string path = profilePath + "screenshots"; 
  ofDirectory dir(path);
  //only show png files
  dir.allowExt("jpg");
  //populate the directory object
  dir.listDir();
  for (int i = 0; i < dir.size(); i++) {
    Screenshot newScreen;
    string filepath = dir.getPath(i);
    string filename = dir.getName(i);
    newScreen.img.load(filepath);
    size_t pos = filename.find(".");
    if(pos != string::npos) {
      string microts = filename.substr(0, pos);
      newScreen.ts = (strtoul(microts.c_str(), NULL, 0)-firstts)/1000000.0;
      ofLogNotice("screenshot ts") << strtoul(microts.c_str(), NULL, 0) << " " << newScreen.ts;
    }
    screenshots.push_back(newScreen);
  }  
  std::sort(screenshots.begin(), screenshots.end());
  
  // copy data from timeline to the local equivalents
  functionCalls = timeline.getFunctionCalls();
  functionMap = timeline.getFunctionMap();
  scripts = timeline.getScripts();
  userEvents = timeline.getUserEvents();
  
  // get the maximum scriptId 
  maxScriptId = 0;
  for(auto& s : scripts) {
    if(s.scriptId > maxScriptId) maxScriptId = s.scriptId;
  }
  
  
  // ***************************** INIT openFrameworks STUFF
  ofBackground(0);

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
  
  
  if(timeline.isPlaying()) {
    // get the message queue from the timeline
    timeline.lock();
    // swap queues to get access to the full queue with only one lock
    messageFIFOLocal.swap(timeline.messageFIFO);
    timeline.unlock();
    
    for(auto& m : messageFIFOLocal) {
      if(m.type == "functionCall") {   
        ofSetColor(ofRandom(255), ofRandom(255), ofRandom(255), 255);
        ofDrawCircle(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()), 30);  
      } else if (m.type == "timelineReset") {
        // time cursor has reached the last event and has been reset
      }
    }
    messageFIFOLocal.clear(); // clear the local queue in preparation for the next swap
    
  }
  
  // set the current screenshot to use
  if(screenshots[0].ts > timeline.getTimeCursor()) {
    currentScreen = 0;
  } else {
    for(int i = 0; i < screenshots.size()-1; i++) {
      if(screenshots[i].ts < timeline.getTimeCursor()
        && screenshots[i+1].ts > timeline.getTimeCursor()
      ) {
        currentScreen = i;
        break;
      } else if (screenshots[i+1].ts < timeline.getTimeCursor()) {
        currentScreen = i+1;
      }
    }
  }
  ofSetColor(255, 255);
  screenshots[currentScreen].img.draw(0, 0, ofGetWidth(), ofGetHeight());
  
  // if(!rendering) timeline.draw();
  ofSetColor(0, 255);
  timeline.draw();
  drawStaticRepresentation();
  // drawSpiral();
  // ofLogNotice("timeCursor: ") << timeline.getTimeCursor();
}

void ofApp::drawStaticRepresentation() {
  // draw scripts in a spiral using polar coordinates
  ofPushMatrix();
  ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
  ofSetColor(230, 100, 100, 100);
  for(auto& s : scripts) {
    glm::vec2 pos = s.getSpiralCoordinate(maxScriptId);
    float size = ofClamp(pow(s.numFunctions, 2)*0.02, 5, 100);
    // float size = 3;
    ofSetColor(ofColor::fromHsb(s.scriptId*300 % 360, 210, 200, 60));
    ofDrawCircle(pos.x, pos.y, size);
  }
  ofSetColor(230, 50, 50, 50);
  ofSetLineWidth(5);
  for(auto& fc : functionCalls) {
    auto script = std::find(scripts.begin(), scripts.end(), fc.scriptId);
    auto parentScript = std::find(scripts.begin(), scripts.end(), fc.parentScriptId);
    if(script == scripts.end()) ofLogNotice("drawStaticRepresentation") << "ERROR: script not found, id: " << fc.scriptId;
    if(parentScript == scripts.end()) ofLogNotice("drawStaticRepresentation") << "ERROR: parent script not found, id: " << fc.parentScriptId;
    if(script != scripts.end() && parentScript != scripts.end()) {
      glm::vec2 p1 = script->getSpiralCoordinate(maxScriptId);
      glm::vec2 p2 = parentScript->getSpiralCoordinate(maxScriptId);
      ofSetColor(ofColor::fromHsb(fc.scriptId*300 % 360, 210, 200, 60));
      ofPolyline line;
      line.addVertex(p1.x, p1.y, 0);
      glm::vec2 c1 = p1 + 0.25*(p2-p1);
      glm::vec2 c2 = p1 + 0.75*(p2-p1);
      // c1 *= 1.4;
      // rotate the point
      c1 = glm::rotate(c1, 0.1f);
      c2 *= 1.1;
      line.bezierTo(c1.x, c1.y, c2.x, c2.y, p2.x, p2.y);
      line.draw();
      // ofDrawLine(p1, p2);
    }
  }
  ofPopMatrix();
}

void ofApp::drawSpiral() {
  static int numScriptsToDraw = 1;
  const int max = 270;
  const float sqrt_two = sqrt(2.0);
  ofPushMatrix();
  ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
  for(int i = 0; i < numScriptsToDraw; i++) {
    float distance = float(i)/float(max * 2); // the distance along the spiral based on the scriptId
    float angle = (pow((1-distance), 2) + pow((1-distance)*1, 6.) * PI) * TWO_PI * 2;
    float radius = 0;
    if(i!=0) radius = (1-pow((1-distance), 2.0) + 0.05 ) * ofGetHeight() * 0.4;
    float x = cos(angle) * radius;
    float y = sin(angle) * radius;
    float size = 3;
    ofDrawCircle(x, y, size);
  }
  ofPopMatrix();
  numScriptsToDraw = (numScriptsToDraw+1) % 270;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if(key == ' ') {
    timeline.togglePlay();
  }
}



//--------------------------------------------------------------
void ofApp::keyReleased(int key){

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