#include "ofApp.h"
#include <filesystem>
namespace fs = std::filesystem;

//--------------------------------------------------------------
void ofApp::setup() {
  WIDTH = ofGetWidth();
  HEIGHT = ofGetHeight();
  
  // graphX = WIDTH * 0.67;
  // graphY = HEIGHT * .5;
  graphY = HEIGHT * -0.15;
  graphX = WIDTH * 0.15;

  graphScaling = HEIGHT * 0.4;
  
  // must set makeContours to true in order to generate paths
  font.load("SourceCodePro-Regular.otf", 16, false, false, true);
  
  string profilePath = "profiles/filming-baudry-Profile-20200210T170430/";
  
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

  // TRIANGLE POSITIONS
  if(doTrianglePositions) {
    // split scripts into categories: 0: built-in, 1: typing, 2: fetching (pressed enter), 3: displaying results
    float fetching_starts = 7.044;
    float displaying_starts = 8.36;
    // for(auto& s : scripts) {
    //   if(s.scriptType == "built-in") s.category = 0;
    //   else if(s.firstCalled < fetching_starts) s.category = 1;
    //   else if(s.firstCalled < displaying_starts) s.category = 2;
    //   else s.category = 3;
    // }
    for(auto& s : scripts) {
      if(s.scriptType == "built-in") s.category = 0;
      else if(s.scriptType == "remote") s.category = 1;
      else if(s.scriptType == "extension") s.category = 2;
      else s.category = 3;
    }
    // create triangles
    glm::vec2 p1, p2, p3, p4, p5, p6; // all the points we need
    float stretch = 0.3;
    p1 = glm::vec2(-.5, 0.3333);
    p2 = glm::vec2(.5, 0.3333);
    p3 = glm::vec2(0, -0.53);
    p4 = glm::vec2(0, 1.199) + glm::vec2(cos(TWO_PI*0.25)*stretch, sin(TWO_PI*0.25)*stretch);
    p5 = glm::vec2(1, -0.53) + glm::vec2(cos(TWO_PI*0.9166666)*stretch, sin(TWO_PI*0.9166666)*stretch);
    p6 = glm::vec2(-1, -0.53) + glm::vec2(cos(TWO_PI*0.5833333)*stretch, sin(TWO_PI*0.5833333)*stretch);
    float scale = 1.1;
    p1 *= scale;
    p2 *= scale;
    p3 *= scale;
    p4 *= scale;
    p5 *= scale;
    p6 *= scale;
    
    Triangle built_in_triangle = Triangle(p1, p2, p3);
    Triangle typing_triangle = Triangle(p4, p2, p1);
    Triangle fetching_triangle = Triangle(p2, p5, p3);
    Triangle displaying_triangle = Triangle(p1, p3, p6);
    triangles.push_back(built_in_triangle);
    triangles.push_back(typing_triangle);
    triangles.push_back(fetching_triangle);
    triangles.push_back(displaying_triangle);

    circles.clear();
    // sort scripts after number of functions
    sort(scripts.begin(), scripts.end(), 
        [](const Script & a, const Script & b) -> bool
    { 
        return a.numFunctions > b.numFunctions; 
    });
    for(auto& s : scripts) {
      // scripts should be sorted so that the one with the most functions is first
      // so that the biggest circles get placed first
      glm::vec2 triCenter = triangles[s.category].getCenter();
      float angle = 0, radius = 0; // polar coordinate offset
      Circle c;
      c.col = ofColor::fromHsb(s.category*50, 255, 255);
      c.r = ofClamp(s.getSize(), 0.09, 1.0) * 0.32;
      ofLogNotice("script size") << s.getSize();
      bool overlaps_with_circle = false;
      do {
        // set new position of circle
        c.p = triCenter + glm::vec2(cos(angle)*radius, sin(angle)*radius);
        angle += PI - .1;
        radius = fmod(radius + PI/10.0, 1.5);
        overlaps_with_circle = false;
        for(auto& tc : circles) {
          if(c.circleOverlaps(tc)) {
            overlaps_with_circle = true;
            break;
          }
        }
      } while(!triangles[s.category].isCircleInside(c.p, c.r) || overlaps_with_circle);
      s.scriptCircle = c;
      s.pos = c.p * (float)graphScaling;
      s.radius = c.r * (float)graphScaling;
      circles.push_back(c);
      // create sphere
      ofIcoSpherePrimitive sphere;
      sphere.setRadius(s.radius);
      sphere.setPosition(s.pos.x, s.pos.y, 0);
      sphere.setResolution(1);
      scriptSpheres.push_back(sphere);
    }
    ofLogNotice("setup") << "Script positions calculated";
    // script positions finished
    // function positions
    for(auto& fp : functionMap) {
      auto& f = fp.second;
      auto script = std::find(scripts.begin(), scripts.end(), f.scriptId);
      // get the non-scaled up positions
      glm::vec2 scriptPos = script->scriptCircle.p;
      float scriptSize = script->scriptCircle.r;

      float angle = 0, radius = 0; // polar coordinate offset
      Circle c;
      c.col = ofColor::fromHsb(100, 255, 255);
      c.r = float(ofClamp(sqrt(f.calledTimes + f.callingTimes)*6, 18, 100))/100.0 * 0.039;
      bool overlaps_with_circle = false;
      do {
        // set new position of circle
        c.p = scriptPos + glm::vec2(cos(angle)*radius, sin(angle)*radius);
        angle += PI - .02;
        radius = fmod(radius + PI/5.0, scriptSize);
        overlaps_with_circle = false;
        for(auto& tc : script->functionCircles) {
          if(c.circleOverlaps(tc)) {
            overlaps_with_circle = true;
            break;
          }
        }
      } while(!script->scriptCircle.isCircleInside(c) || overlaps_with_circle);
      script->functionCircles.push_back(c);
      circles.push_back(c);

      f.functionCircle = c;
      f.pos = c.p * (float)graphScaling;
      ofIcoSpherePrimitive sphere;
      sphere.setPosition(f.pos.x, f.pos.y, 0);
      sphere.setRadius(2);
      sphere.setResolution(1);
      f.sphereIndex = funcSpheres.size();
      funcSpheres.push_back(sphere);
    }
  }

  ofLogNotice("setup") << "Script and function positions calculated";
  
  // generateMesh();
  // easyCam.enableMouseInput();

  triangle.p1 = glm::vec2(WIDTH/2, HEIGHT/4);
  triangle.p2 = glm::vec2(WIDTH/4, HEIGHT*0.75);
  triangle.p3 = glm::vec2(WIDTH*0.75, HEIGHT*0.5);
  
  // ***************************** INIT openFrameworks STUFF
  // load the video
  traceVideo.init("video_files/taylorswift_2.mov", WIDTH, HEIGHT);
  pauseVideo.init("video_files/taylorswift_1.mov", WIDTH, HEIGHT);

  setupGui();
  ofLogNotice("setup") << "GUI setup finished";
  ofBackground(0);
  ofEnableAlphaBlending();
  cam.setVFlip(false);
  cam.enableOrtho();
  cam.setNearClip(0);
  cam.setFarClip(-5000);
  
  renderFbo.allocate(WIDTH, HEIGHT, GL_RGB);
  
  invertShader.load("shaders/invertColours/shader");
  flipShader.load("shaders/flipShader/shader");

  // start the timeline thread that communicates with SC at higher than framerate
  timeline.startThread(true);
}

void ofApp::setupGui() {
  // all of the setup code for the ofxGui GUI
  // add listener function for buttons
  saveSVGButton.addListener(this, &ofApp::saveSVGButtonPressed);
  exportTrianglesSVGButton.addListener(this, &ofApp::saveTrianglesSVG);
  sendActivityEnvelopeToSCButton.addListener(this, &ofApp::sendActivityDataOSC);
  doLoopToggle.addListener(this, &ofApp::doLoopToggleFunc);
  doGraphicsToggle.addListener(this, &ofApp::toggleDoDrawGraphics);
  exportMeshButton.addListener(this, &ofApp::exportMesh);
  exportMeshGridButton.addListener(this, &ofApp::exportMeshGrid);
  exportMeshGridPieceButton.addListener(this, &ofApp::exportMeshGridPiece);
  regenerateMeshButton.addListener(this, &ofApp::regenerateMesh);
  graphScaling.addListener(this, &ofApp::updateScaling);
  
  // create the GUI panel
  gui.setup();
  gui.add(saveSVGButton.setup("Save SVG"));
  gui.add(exportTrianglesSVGButton.setup("Save triangles as SVG"));
  gui.add(sendActivityEnvelopeToSCButton.setup("Send activity envelope to SC"));
  gui.add(doLoopToggle.setup("loop", true));
  gui.add(doGraphicsToggle.setup("draw graphics", true));
  gui.add(showTriangle.set("show triangle", false));
  gui.add(triangleScale.set("triangle scale", WIDTH*0.2, 1, WIDTH));
  gui.add(doDrawScreenshots.set("draw screenshots", false));
  gui.add(showMesh.set("show mesh", false));
  gui.add(regenerateMeshButton.setup("regenerate mesh"));
  gui.add(exportMeshButton.setup("export mesh"));
  gui.add(exportMeshGridButton.setup("export mesh grid"));
  gui.add(exportMeshGridPieceButton.setup("export mesh grid piece"));
  gui.add(meshGridPieceX.set("mesh piece X", 3, 0, 6));
  gui.add(meshGridPieceY.set("mesh piece Y", 4, 0, 6));
  gui.add(functionPointOffsetRatio.set("function offset ratio", 0.001, -0.05, 0.05));
  gui.add(numScriptsToDraw.set("num scripts to draw", maxScriptId, 0, maxScriptId));
  gui.add(hueRotation.set("hueRotation", 24, 0, 255));
  gui.add(hueOffset.set("hueOffset", 247, 0, 255));
  gui.add(saturation.set("saturation", 210));
  gui.add(brightness.set("brightness", 180));
  gui.add(videoOffset.set("videoOffset", -3.975, -4.18-0.5, -4.18+0.5));
  gui.add(graphX.set("graphX", WIDTH * 0.15, -WIDTH, WIDTH));
  gui.add(graphY.set("graphY", HEIGHT * -0.15, -HEIGHT, HEIGHT));
  gui.add(graphScaling.set("graphScaling", HEIGHT * 0.4, HEIGHT * 0.3, HEIGHT * 2.0));
  gui.add(drawFunctionCallsOneAtATime.set("fc one aat", false));
  gui.add(currentFunctionCall.set("currentFunctionCall", 0, 0, functionCalls.size()-1));
  showGui = true;
}

void ofApp::updateScaling(float& scaling) {
  // update scripts to new scaling
  for(int i = 0; i < scripts.size(); i++) {
    auto& s = scripts[i];
    Circle c = s.scriptCircle;
    s.pos = c.p * (float)graphScaling;
    s.radius = c.r * (float)graphScaling;
    circles.push_back(c);
    auto& sphere = scriptSpheres[i];
    sphere.setRadius(s.radius);
    sphere.setPosition(s.pos.x, s.pos.y, 0);
  }
  // update functions
  for(auto& fp : functionMap) {
      auto& f = fp.second;
      Circle c = f.functionCircle;
      f.pos = c.p * (float)graphScaling;
      auto& sphere = funcSpheres[f.sphereIndex];
      sphere.setPosition(f.pos.x, f.pos.y, 0);
      sphere.setRadius(2);
    }
}

void ofApp::saveSVGButtonPressed() {
  // the save SVG button was pressed
  ofLogNotice() << "SVG button pressed";
  ofBeginSaveScreenAsSVG("svg_" + ofGetTimestampString() + ".svg", false, false, ofRectangle(0, 0, WIDTH, HEIGHT));
  ofClear(255, 255);
  drawStaticPointsOfScripts();
  // drawStaticPointsOfFunctions();
  // drawStaticFunctionCallLines();
  // drawStaticRepresentation();
  ofEndSaveScreenAsSVG();
}

void ofApp::saveTrianglesSVG() {
  ofBeginSaveScreenAsSVG("svg_triangles_" + ofGetTimestampString() + ".svg", false, false, ofRectangle(0, 0, WIDTH, HEIGHT));
  ofClear(255, 255);
  ofPushMatrix();
  ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
  ofSetColor(255, 0, 0, 255);
  ofNoFill();
  for(auto& t : triangles) {
    t.draw(triangleScale);
  }
  for(auto& c : circles) {
    c.draw(triangleScale);
  }
  ofPopMatrix();
  ofEndSaveScreenAsSVG();
}

void ofApp::toggleDoDrawGraphics(bool &b) {
  doDrawGraphics = b;
}

void ofApp::exportMesh() {
  mesh.save("mesh_" + ofGetTimestampString() + ".ply");
}

void ofApp::exportMeshGrid() {
  ModelExport gp;
  gp.maxScriptId = maxScriptId;
  for(auto& s : scripts) {
    gp.addScriptPoint(s);
  }
  for(auto& fp : functionMap) {
    auto script = std::find(scripts.begin(), scripts.end(), fp.second.scriptId);
    gp.addFunctionPoint(fp.second, *script, functionPointOffsetRatio);
  }
  int gridSize = 7; // n by n grid
  vector<ofMesh> meshes = gp.generateMeshGrid(gridSize, 1);
  string timestamp = ofGetTimestampString();
  for(int y = 0; y < gridSize; y++) {
    for(int x = 0; x < gridSize; x++) {
      meshes[x + (y*gridSize)].save("mesh_grid" + timestamp + "/mesh_" + to_string(x) + "x" + to_string(y) + ".ply");
    }
  }
}

void ofApp::exportMeshGridPiece() {
  ModelExport gp;
  gp.maxScriptId = maxScriptId;
  for(auto& s : scripts) {
    gp.addScriptPoint(s);
  }
  for(auto& fp : functionMap) {
    auto script = std::find(scripts.begin(), scripts.end(), fp.second.scriptId);
    gp.addFunctionPoint(fp.second, *script, functionPointOffsetRatio);
  }
  int gridSize = 7; // n by n grid
  string timestamp = ofGetTimestampString();
  gp.generateMeshGridPiece(gridSize, meshGridPieceX, meshGridPieceY, 1)
    .save("mesh_grid_piece" + timestamp + "_" + to_string(meshGridPieceX) + "x" + to_string(meshGridPieceY) + ".ply");
}

void ofApp::generateMesh() {
  ofLogNotice("generateMesh") << "Mesh generation started";
  ModelExport gp;
  gp.maxScriptId = maxScriptId;
  // numScripts and numFuncs only for debug
  int numScripts = 0;
  for(auto& s : scripts) {
    if(numScripts < 1000)
      gp.addScriptPoint(s);
    numScripts++;
  }
  int numFuncs = 0;
  for(auto& fp : functionMap) {
    if(numFuncs < 1000) {
      auto script = std::find(scripts.begin(), scripts.end(), fp.second.scriptId);
      gp.addFunctionPoint(fp.second, *script, functionPointOffsetRatio);
    }
    numFuncs++;
  }
  mesh = gp.generateMesh();
}

void ofApp::regenerateMesh()  {
  generateMesh();
}

void ofApp::drawMesh() {
  easyCam.begin();
  ofPushMatrix();
  ofTranslate(-WIDTH/2,-HEIGHT/2);
  ofSetColor(255, 255);
  mesh.drawWireframe();
  ofPopMatrix();
  easyCam.end();
}

void ofApp::sendActivityDataOSC() {
  timeline.sendActivityDataOSC();
}

void ofApp::doLoopToggleFunc(bool &b) {
  timeline.setLoop(b);
}

//--------------------------------------------------------------
void ofApp::update(){
  
  
	//ofLog() << "fps: " << ofGetFrameRate();
}

//--------------------------------------------------------------
void ofApp::draw(){
  float dt = timeline.getNonScaledFramedt();
  float scaleddt = timeline.getFramedt();
  renderFbo.begin();
  
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
      } else if (m.type == "timelineReset") {
        // time cursor has reached the last event and has been reset
        functionCallFbo.begin();
          ofClear(0, 0);
        functionCallFbo.end();
        functionCallsToDraw.clear();
        if(rendering) {
          rendering = false;
          timeline.stopRendering();
        }
      } else if (m.type == "startPlaying") {

      } else if (m.type == "changeSpeed") {
        traceVideo.setSpeed(timeline.getTimeScale());
      }
    }
    messageFIFOLocal.clear(); // clear the local queue in preparation for the next swap
  } else {
    // timeline isn't playing
  }

  // update graphX and graphY
  if(drawFunctionCallsOneAtATime) {
    graphX = float(ofGetMouseX() - WIDTH/2)/float(WIDTH) * graphScaling * 2.0;
    graphY = float(ofGetMouseY() - HEIGHT* 0.35)/float(HEIGHT) * -graphScaling * 2.0;
  }
  
  if(doDrawGraphics) {
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
    if(doDrawScreenshots) {
      ofSetColor(205, 191, 255, 255);
      invertShader.begin();
      invertShader.setUniformTexture("tex0", screenshots[currentScreen].img.getTexture(), 1);
      invertShader.setUniform2f("resMult", float(screenshots[currentScreen].img.getWidth()) / float(WIDTH), float(screenshots[currentScreen].img.getHeight()) / float(HEIGHT));
      invertShader.setUniform2f("imgRes", screenshots[currentScreen].img.getWidth(), screenshots[currentScreen].img.getHeight());
      invertShader.setUniform2f("resolution", WIDTH, HEIGHT);
      invertShader.setUniform1f("time", timeline.getTimeCursor());
      screenshots[currentScreen].img.draw(0, 0, WIDTH, HEIGHT);
      ofDrawRectangle(0, 0, WIDTH, HEIGHT);
      invertShader.end();
    } else {
      // clear screen if we don't do screenshots
      ofBackground(0, 255);
    }

    if(timeline.isPlaying() && timeline.getTimeScale() > 0) {
      // frequently changing speed seems to lead to crashes so
      // it is safer to just use the Timeline clock and set the position
      // of the video every frame.
      // This will not work with ofVideoPlayer, requires HAP video
      traceVideo.setPosition(timeline.getTimeCursor(), videoOffset);
      traceVideo.draw(WIDTH, HEIGHT);
    } else {
      pauseVideo.draw(WIDTH, HEIGHT);
      if(ofRandomuf() > 0.95) {
        pauseVideoPosition = ofRandomuf() * 0.8 * pauseVideo.getDuration();
      }
      pauseVideoPosition += dt;
    }
    
    
    cam.begin();
    drawStaticPointsOfScripts();
    drawStaticPointsOfFunctions();
    ofSetColor(255, 255);
    // functionCallFbo.draw(0, 0);
    if(drawFunctionCallsOneAtATime) {
      auto& fc = functionCalls[currentFunctionCall];
      drawSingleStaticFunctionCallLine(fc.function_id, fc.parent, fc.scriptId);
    } else {
      for(auto& fc : functionCallsToDraw) {
        drawSingleStaticFunctionCallLine(fc.function_id, fc.parent, fc.scriptId);
      } 
    }
    
    cam.end();
  } else {
    // if not drawing stuff, clear the screen
    ofClear(0, 255);
  }
  
  // draw the timeline
  ofSetColor(130, 80);
  timeline.draw();

  renderFbo.end();
  ofSetColor(255, 255);
  // flip y in renderFbo
  flipShader.begin();
  flipShader.setUniformTexture("tex0", renderFbo.getTextureReference(), 1);
  flipShader.setUniform2f("resolution", WIDTH, HEIGHT);
  ofSetColor(255, 255, 255);
  //ofDrawRect(0, 0, ofGetWidth(), ofGetHeight()); // doesn't give a texcoord
  renderFbo.draw(0, 0);
  flipShader.end();
  
  if(rendering) {
    // write frame to disk
    // glReadBuffer(GL_FRONT);
    // grabImg.grabScreen(0, 0 , WIDTH, HEIGHT);
    renderFbo.readToPixels(renderPixels);
    grabImg.setFromPixels(renderPixels);
    grabImg.save(renderDirectory + "frame-" + to_string(frameNumber) + ".png");
    frameNumber++;
    // move time forward by one frame time
    timeline.progressFrame();
  }
  
  if(showMesh) drawMesh();
  
  if(showGui){
		gui.draw();
	}
  if(showTriangle) {
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
    ofSetColor(255, 0, 0, 255);
    ofNoFill();
    for(auto& t : triangles) {
      t.draw(triangleScale);
    }
    for(auto& c : circles) {
      c.draw(triangleScale);
    }
    ofPopMatrix();
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
  ofTranslate(graphX, graphY);
  
  auto function = functionMap.find(function_id);
  if(function == functionMap.end()) ofLogNotice("drawStaticRepresentation") << "ERROR: function not found, id: " << function_id;
  if(function->second.scriptId <= numScriptsToDraw) {
    if(parent != 0) { // the function call with id 0 doesn't exist
      auto parentCall = std::find(functionCalls.begin(), functionCalls.end(), parent);
      if(parentCall == functionCalls.end()) ofLogNotice("drawStaticRepresentation") << "ERROR: parentCall not found, id: " << parent;
      else {
        auto parentFunction = functionMap.find(parentCall->function_id);
        if(parentFunction == functionMap.end()) ofLogNotice("drawStaticRepresentation") << "ERROR: parentFunction not found, id: " << parentCall->function_id;
        else {
          glm::vec2 p1 = function->second.pos;
          glm::vec2 p2 = parentFunction->second.pos;
          
          float distance = glm::distance(p1, p2);
          if(distance > HEIGHT*0.02 && !drawFunctionCallsOneAtATime) {
            ofPolyline line;
            line.addVertex(p1.x, p1.y, 0);
            glm::vec2 c1 = p1 + 0.25*(p2-p1);
            glm::vec2 c2 = p1 + 0.75*(p2-p1);
            // rotate the point
            float rotation = (distance/float(HEIGHT));
            c1 = glm::rotate(c1, rotation);
            c2 = glm::rotate(c2, -rotation);
            line.bezierTo(c1.x, c1.y, c2.x, c2.y, p2.x, p2.y);
            float thickness = ofMap(distance, WIDTH*0.01, WIDTH*0.3, .5, 7.0);
            // ofLogNotice("functionline") << "thickness: " << thickness;
            // ofSetColor(ofColor::fromHsb((scriptId*300 - 100) % 360, 150, 255, 60));
            // ofSetColor(ofColor::fromHsb((scriptId*300 - 200) % 360, 150, 255, 60)); // bright colours
            ofSetColor(getColorFromScriptId(scriptId, 60)); // dark colours
            
            if(thickness > 1.2) drawThickPolyline(line, thickness);
            line.draw();
            
          } else if (drawFunctionCallsOneAtATime) {
            ofPolyline line;
            line.addVertex(p1.x, p1.y, 0);
            
            line.lineTo(p2.x, p2.y);
            float thickness = ofMap(graphScaling, 1.0, 2000, 2.0, 10);
            ofSetColor(0, 0, 150, 255);
            
            drawThickPolyline(line, thickness);
          }
           else {
            ofSetColor(getColorFromScriptId(scriptId, 60)); // dark colours
            ofDrawLine(p1, p2);
          }
        }
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
  float angleSmooth = 0;

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

void ofApp::drawStaticPointsOfScripts(bool drawCenters) {
  ofPushMatrix();
  ofTranslate(graphX, graphY);
  // ofSetColor(120, 200);
  // for(auto& s : scripts) {
  //   ofSetColor(ofColor::fromHsb((s.scriptId*300 - 200) % 360, 150, 255, 120));
  //   glm::vec2 pos = s.getSpiralCoordinate(maxScriptId, HEIGHT);
  //   // float size = 3;
  //   float size = s.getSize() * HEIGHT * 0.075;
  //   // ofSetColor(ofColor::fromHsb(s.scriptId*300 % 360, 210, 200, 60));
  //   ofDrawCircle(pos.x, pos.y, size);
  //   if(drawCenters) {
  //     ofSetColor(0, 255);
  //     ofDrawCircle(pos.x, pos.y, 1);
  //   }
  // }
  for(int i = 0; i < scripts.size(); i++) {
    if(scripts[i].scriptId <= numScriptsToDraw) {
      // ofSetColor(ofColor::fromHsb((scripts[i].scriptId*300 - 200) % 360, 150, 255, 120)); // bright colours
      if(drawFunctionCallsOneAtATime) {
        auto& fc = functionCalls[currentFunctionCall];
        if(scripts[i].scriptId == fc.scriptId) {
          ofSetColor(255, 0, 0, 255);
        } else if (scripts[i].scriptId == fc.parentScriptId) {
          ofSetColor(0, 255, 0, 255);
        } else {
          ofSetColor(100, 100, 100, 100);
        }
      } else {
        ofSetColor(getColorFromScriptId(scripts[i].scriptId, 120)); // dark colours
      }

      scriptSpheres[i].drawWireframe();
    }
  }
  ofPopMatrix();
  
}

void ofApp::drawStaticPointsOfFunctions() {
  ofPushMatrix();
  ofTranslate(graphX, graphY);
  ofSetColor(190, 255);
  // for(auto& fp : functionMap) {
  //   glm::vec2 pos = fp.second.pos;
  //   float size = 2;
  //   // ofSetColor(ofColor::fromHsb(s.scriptId*300 % 360, 210, 200, 60));
  //   ofDrawCircle(pos.x, pos.y, size);
  // }
  for(auto& s : funcSpheres) {
    s.draw();
  }
  ofPopMatrix();
}

ofColor ofApp::getColorFromScriptId(int scriptId, int alpha) {
  float hue = (scriptId*hueRotation + hueOffset) % 255;
  // saturation and brightness are set in the GUI as ofParameters
  return ofColor::fromHsb(hue, saturation, brightness, alpha);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if(key == ' ') {
    timeline.togglePlay();
  } else if (key == 'g') {
    showGui = !showGui;
  } else if(key == 'r') {
    ofFileDialogResult result = ofSystemLoadDialog("Render folder", true, renderDirectory);
    if(result.bSuccess) {
      renderDirectory = result.getPath() + "/";
      rendering = true;
      timeline.startRendering();
    }
  } else if(key == OF_KEY_LEFT) {
    currentFunctionCall -= 1;
    if(currentFunctionCall < 0) currentFunctionCall = 0;
  } else if(key == OF_KEY_RIGHT) {
    currentFunctionCall += 1;
    if(currentFunctionCall >= functionCalls.size()) currentFunctionCall = functionCalls.size()-1;
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
  if(!showGui) {
    timeline.click(x, y);
  }
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

