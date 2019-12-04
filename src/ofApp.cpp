#include "ofApp.h"


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
  
  // if(!rendering) timeline.draw();
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