#include "ofApp.h"
#include <filesystem>
namespace fs = std::filesystem;

//--------------------------------------------------------------
void ofApp::setup() {
  WIDTH = ofGetWidth();
  HEIGHT = ofGetHeight();
  
  // must set makeContours to true in order to generate paths
  font.load("SourceCodePro-Regular.otf", 16, false, false, true);
  
  string profilePath = "profiles/whyamisotired20200108/";
  
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
  
  // calculate function position
  for(auto& fp : functionMap) {
    auto& f = fp.second;
    auto script = std::find(scripts.begin(), scripts.end(), f.scriptId);
    glm::vec2 scriptPos = script->getSpiralCoordinate(maxScriptId);
    float scriptSize = script->getSize() * ofGetHeight() * 0.075;
    glm::vec2 funcPos = f.getRelativeSpiralPos();
    f.pos = scriptPos + (funcPos * scriptSize);
  }
  
  // ***************************** INIT openFrameworks STUFF
  setupGui();
  ofBackground(0);
  ofEnableAlphaBlending();
  
  functionCallFbo.allocate(WIDTH, HEIGHT, GL_RGBA);
  functionCallFbo.begin();
    ofClear(0, 0);
  functionCallFbo.end();

  timeline.startThread(true);
}

void ofApp::setupGui() {
  // all of the setup code for the ofxGui GUI
  // add listener function for buttons
  saveSVGButton.addListener(this, &ofApp::saveSVGButtonPressed);
  sendActivityEnvelopeToSCButton.addListener(this, &ofApp::sendActivityDataOSC);
  
  // create the GUI panel
  gui.setup();
  gui.add(saveSVGButton.setup("Save SVG"));
  gui.add(sendActivityEnvelopeToSCButton.setup("Send activity envelope to SC"));
  showGui = true;
}

void ofApp::saveSVGButtonPressed() {
  // the save SVG button was pressed
  ofLogNotice() << "SVG button pressed";
  ofBeginSaveScreenAsSVG("svg_" + ofGetTimestampString() + ".svg", false, false, ofRectangle(0, 0, ofGetWidth(), ofGetHeight()));
  ofClear(255, 255);
  drawStaticPointsOfScripts();
  drawStaticPointsOfFunctions();
  drawStaticFunctionCallLines();
  // drawStaticRepresentation();
  ofEndSaveScreenAsSVG();
}

void ofApp::sendActivityDataOSC() {
  timeline.sendActivityDataOSC();
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
        auto fc = std::find(functionCalls.begin(), functionCalls.end(), int(m.parameters["id"]));  
        functionCallsToDraw.push_back(*fc);
        functionCallFbo.begin();
          // ofEnableBlendMode(OF_BLENDMODE_ADD);
          drawSingleStaticFunctionCallLine(m.stringParameters["function_id"], m.parameters["parent"], m.parameters["scriptId"]);
          // ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        functionCallFbo.end();
      } else if (m.type == "timelineReset") {
        // time cursor has reached the last event and has been reset
        functionCallFbo.begin();
          ofClear(0, 0);
        functionCallFbo.end();
        functionCallsToDraw.clear();
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
  // drawStaticRepresentation();
  drawStaticPointsOfScripts();
  drawStaticPointsOfFunctions();
  ofSetColor(255, 255);
  // functionCallFbo.draw(0, 0);
  for(auto& fc : functionCallsToDraw) {
    drawSingleStaticFunctionCallLine(fc.function_id, fc.parent, fc.scriptId);
  }
  // drawStaticFunctionCallLines();
  // drawSpiral();
  // ofLogNotice("timeCursor: ") << timeline.getTimeCursor();
  if(showGui){
		gui.draw();
	}
}

void ofApp::drawStaticFunctionCallLines() {
  // draw scripts in a spiral using polar coordinates
  // ofSetLineWidth(5);
  for(auto& fc : functionCalls) {
    drawSingleStaticFunctionCallLine(fc.function_id, fc.parent, fc.scriptId);
  }
}

void ofApp::drawSingleStaticFunctionCallLine(string function_id, int parent, int scriptId) {
  ofPushMatrix();
  ofTranslate(ofGetWidth()/2, ofGetHeight()/2); // draw relative to the center of the window
  
  auto function = functionMap.find(function_id);
  if(function == functionMap.end()) ofLogNotice("drawStaticRepresentation") << "ERROR: function not found, id: " << function_id;
  auto parentCall = std::find(functionCalls.begin(), functionCalls.end(), parent);
  if(parentCall == functionCalls.end()) ofLogNotice("drawStaticRepresentation") << "ERROR: parentCall not found, id: " << parent;
  else {
    auto parentFunction = functionMap.find(parentCall->function_id);
    if(parentFunction == functionMap.end()) ofLogNotice("drawStaticRepresentation") << "ERROR: parentFunction not found, id: " << parentCall->function_id;
    else {
      glm::vec2 p1 = function->second.pos;
      glm::vec2 p2 = parentFunction->second.pos;
      
      float distance = glm::distance(p1, p2);
      if(distance > ofGetHeight()*0.02) {
        ofPolyline line;
        line.addVertex(p1.x, p1.y, 0);
        glm::vec2 c1 = p1 + 0.25*(p2-p1);
        glm::vec2 c2 = p1 + 0.75*(p2-p1);
        // rotate the point
        float rotation = (distance/float(ofGetHeight()));
        c1 = glm::rotate(c1, rotation);
        c2 = glm::rotate(c2, -rotation);
        line.bezierTo(c1.x, c1.y, c2.x, c2.y, p2.x, p2.y);
        float thickness = ofMap(distance, WIDTH*0.01, WIDTH*0.3, .5, 7.0);
        ofLogNotice("functionline") << "thickness: " << thickness;
        ofSetColor(ofColor::fromHsb(scriptId*300 % 360, 210, 200, 60));
        if(thickness > 1.2) drawThickPolyline(line, thickness);
        line.draw();
        
      } else {
        ofDrawLine(p1, p2);
      }
    }
  }
  ofPopMatrix(); // restore drawing position
}

void ofApp::drawThickPolyline(ofPolyline line, float width) {
  // code by zach https://github.com/ofZach/drawingCodeACT/blob/master/thickness/src/ofApp.cpp#L22-L70
  ofMesh meshy;
  meshy.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

  float widthSmooth = width;
  float angleSmooth;

  for (int i = 0;  i < line.getVertices().size(); i++){        
      int me_m_one = i-1;
      int me_p_one = i+1;
      if (me_m_one < 0) me_m_one = 0;
      if (me_p_one ==  line.getVertices().size()) me_p_one =  line.getVertices().size()-1;
      
      ofPoint diff = line.getVertices()[me_p_one] - line.getVertices()[me_m_one];
      float angle = atan2(diff.y, diff.x);
      
      if (i == 0) angleSmooth = angle;
      else {
          angleSmooth = ofLerpDegrees(angleSmooth, angle, 1.0);
      }
      float dist = diff.length();
      float w = ofMap(dist, 0, 20, 10, 2, true);
      widthSmooth = 0.9f * widthSmooth + 0.1f * w;
      
      ofPoint offset;
      offset.x = cos(angleSmooth + PI/2) * widthSmooth;
      offset.y = sin(angleSmooth + PI/2) * widthSmooth;
      meshy.addVertex(  line.getVertices()[i] +  offset );
      meshy.addVertex(  line.getVertices()[i] -  offset );    
  }
  // ofSetColor(0,0,0);
  meshy.draw();
}

void ofApp::drawStaticPointsOfScripts() {
  ofPushMatrix();
  ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
  ofSetColor(80, 200);
  for(auto& s : scripts) {
    glm::vec2 pos = s.getSpiralCoordinate(maxScriptId);
    // float size = 3;
    float size = s.getSize() * ofGetHeight() * 0.075;
    // ofSetColor(ofColor::fromHsb(s.scriptId*300 % 360, 210, 200, 60));
    ofDrawCircle(pos.x, pos.y, size);
  }
  ofPopMatrix();
}

void ofApp::drawStaticPointsOfFunctions() {
  ofPushMatrix();
  ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
  ofSetColor(190, 255);
  for(auto& fp : functionMap) {
    glm::vec2 pos = fp.second.pos;
    float size = 1;
    // ofSetColor(ofColor::fromHsb(s.scriptId*300 % 360, 210, 200, 60));
    ofDrawCircle(pos.x, pos.y, size);
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
    if(i!=0) radius = ( (1-pow((1-distance), 2.0) + 0.05 ) + sin((1-pow((1-distance), 2.0))*max*2) * distance * 0.1 ) * ofGetHeight() * 0.4;
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
  } else if (key == 'g') {
    showGui = !showGui;
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
  // only move timeline if GUI is not shown as otherwise interacting with the GUI would move the timeline every time
  if(!showGui) timeline.click(x, y);

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