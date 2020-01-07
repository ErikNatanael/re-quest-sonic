#include "ofApp.h"
#include <filesystem>
namespace fs = std::filesystem;

//--------------------------------------------------------------
void ofApp::setup() {
  WIDTH = ofGetWidth();
  HEIGHT = ofGetHeight();
  
  // must set makeContours to true in order to generate paths
  font.load("SourceCodePro-Regular.otf", 16, false, false, true);
  
  timeline.init(WIDTH, HEIGHT);
  timeline.parseScriptingProfile("profiles/software_art/scores/scripting_events.json");
  timeline.parseUserEventProfile("profiles/software_art/scores/user_events.json");
  timeline.sendBackgroundInfoOSC();
  timeline.generateScore();
  
  // load all screenshot images
  uint64_t firstts = timeline.getFirstts();
  //some path, may be absolute or relative to bin/data
  string path = "profiles/software_art/screenshots"; 
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
  // ofLogNotice("timeCursor: ") << timeline.getTimeCursor();
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