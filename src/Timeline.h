#pragma once

#include "ofMain.h"
#include <memory>
#include "ofxJSON.h"
#include "ofxOsc.h"

#include "FunctionCall.h"
#include "Function.h"
#include "Script.h"

/* 
This timeline is using the ofThread wrapper around a Poco::Thread.
Create an instance and run timeline.startThread(true); on it to start, and timeline.stopThread(); on exit.
The message queue is a shared resource. Always lock the mutex before accessing it and then release the mutex:

timeline.lock();
// copy message from queue
timeline.unlock();

*/

class TimelineMessage {
public:
  string type;
  map<string, float> parameters;
};

class Timeline : public ofThread  {
private:
  ofMutex oscMutex;
  
  uint64_t timeCursor = 0;
  float timeScale = 1;
  bool playing = false;
  bool rendering = false;
  float frameRate = 60;
  double currentTime = 0;
  ofMutex nonScaledCurrentTimeMutex;
  double nonScaledCurrentTime = 0;
  double lastNonScaledFrameTime = 0;
  double lastFrameTime = 0;
  ofMutex renderCurrentTimeMutex;
  double renderCurrentTime = 0;
  double numTimeStepsToProgress = 0;
  
  uint64_t firstts = 1000000000000;
  uint64_t lastts = 0;
  uint64_t timeWidth = 0;
  uint32_t numScripts = 0;
  uint32_t maxScriptId = 0;
  
  ofxJSONElement json;
  ofFbo timelineFbo;
  int timelineHeight = 0;
  
  int WIDTH = 0, HEIGHT = 0;
  
  ofxOscSender oscSender;
  ofxOscMessage oscMess;
  
  // all data
  
  vector<FunctionCall> functionCalls;
  unordered_map<uint64_t, FunctionCall> callMap;
  unordered_map<uint32_t, Function> functionMap;
  vector<Script> scripts;
  

  // the thread function
  void threadedFunction() {

    // start
    while(isThreadRunning()) {
      static float lastTime = 0;
      static float nonScaledLastTime = 0;
      if(!rendering) {
        currentTime = ofGetElapsedTimef();
      
        double dt = (currentTime-lastTime) * timeScale;
        
        nonScaledCurrentTime += (currentTime-lastTime);
        lastTime = currentTime;
        
        if(playing) {
          progressQueue(dt);
        }
      }
    }
      // done
  }
  
  void progressQueue(float dt) {
    // calculate how many time steps whould be gone through this frame
    const double timeStepsPerSecond = 1000000;
    numTimeStepsToProgress += dt * timeStepsPerSecond;
    while(numTimeStepsToProgress > 0) {
      timeCursor += 1;
      auto search = callMap.find(timeCursor);
      if (search != callMap.end()) {
        // create message
        TimelineMessage mess;
        mess.type = "functionCall";
        mess.parameters.insert({"id", search->second.id});
        mess.parameters.insert({"parent", search->second.parent});
        mess.parameters.insert({"scriptId", search->second.scriptId});
        // send it as OSC
        sendViaOsc(mess);
        // lock access to the resource
        lock();
        // put the message in the queue
        messageFIFO.push_back(mess);
        // done with the resource
        unlock();
      }
      numTimeStepsToProgress -= 1;
    }
    
    if(timeCursor >= lastts) {
      TimelineMessage mess;
      mess.type = "timelineReset";
      lock();
      messageFIFO.push_back(mess);
      unlock();
      timeCursor = firstts;
    }
  }
    
  void sendViaOsc(TimelineMessage& mess) {
    oscMutex.lock();
    oscMess.clear();
    oscMess.setAddress("/timeline-message");
    if(mess.type == "functionCall") {
      oscMess.addStringArg(mess.type);
      oscMess.addInt32Arg(mess.parameters["id"]);
      oscMess.addInt32Arg(mess.parameters["parent"]);
      oscMess.addInt32Arg(mess.parameters["scriptId"]);
    } else if(mess.type == "changeSpeed") {
      oscMess.addStringArg(mess.type);
      oscMess.addFloatArg(mess.parameters["speed"]);
    }
    
    oscSender.sendMessage(oscMess);
    oscMutex.unlock();
  }
  
public:
  
  // message queue
  list<TimelineMessage> messageFIFO;
  
  // functions
  void init(int w, int h) {
    timelineFbo.allocate(w, h, GL_RGBA32F);
    timelineFbo.begin();
    ofBackground(0, 0);
    timelineFbo.end();
    WIDTH = w;
    HEIGHT = h;
    timelineHeight = h*0.01;
    
    oscSender.setup("127.0.0.1", 57120); // send to SuperCollider on the local machine
  }
  void parseProfile(string filepath) {
    // load and parse the json data in the path provided

    bool parsingSuccessful = json.open(filepath);

    if (parsingSuccessful)
    {
        ofLogNotice("ofApp::setup JSON parsing successful");
    }
    else
    {
        ofLogNotice("ofApp::setup")  << "Failed to parse JSON" << endl;
    }
    ofLog() << json["events"];
    ofLog() << json["events"][3]["ts"];
    
    set<int> scriptIds; // set to see how many script ids there is

    if (json["events"].isArray())
    {
      const Json::Value& events = json["events"];
      for (Json::ArrayIndex i = 0; i < events.size(); ++i) {
        if(events[i]["name"] == "ProfileChunk"
          && events[i]["hasNodes"] == true) {
          uint64_t ts = events[i]["ts"].asLargestUInt();
          if(ts < firstts) firstts = ts;
          
          uint64_t chunkTime = 0;
          const Json::Value& timeDeltas = events[i]["timeDeltas"];
          for (Json::ArrayIndex k = 0; k < timeDeltas.size(); ++k) {
            chunkTime += timeDeltas[k].asInt();
          }
          const Json::Value& nodes = events[i]["nodes"];
          for (Json::ArrayIndex j = 0; j < nodes.size(); ++j) {
            FunctionCall tempCall;
            // TODO: more accurate division of the chunk time into functions
            // divide the chunk time evenly among the functions in the chunk
            tempCall.ts = ts + long(double(chunkTime)*0.001*j);
            tempCall.scriptId = nodes[j]["callFrame"]["scriptId"].asInt();
            tempCall.name = nodes[j]["callFrame"]["functionName"].asString();
            tempCall.id = nodes[j]["id"].asInt();
            tempCall.parent = nodes[j]["parent"].asInt();
            functionCalls.push_back(tempCall);
            callMap.insert({tempCall.ts, tempCall});
            scriptIds.insert(tempCall.scriptId);
            
            // create the associated script and store its url
            auto searchScript = find(scripts.begin(), scripts.end(), tempCall.scriptId);
            if(searchScript == scripts.end()) {
              Script tempScript;
              tempScript.scriptId = tempCall.scriptId;
              tempScript.url = nodes[j]["callFrame"]["url"].asString();
              scripts.push_back(tempScript);
            }
            
            // create the associated function
            auto search = functionMap.find(tempCall.id);
            if (search != functionMap.end()) {
                search->second.calledTimes += 1;
            } else {
               Function tempFunc;
               tempFunc.name = tempCall.name;
               tempFunc.id = tempCall.id;
               tempFunc.scriptId = tempCall.scriptId;
               tempFunc.lineNumber = nodes[j]["callFrame"]["lineNumber"].asInt();
               tempFunc.columnNumber = nodes[j]["callFrame"]["columnNumber"].asInt();
               functionMap.insert({tempFunc.id, tempFunc});
            }

            if(tempCall.scriptId > maxScriptId) maxScriptId = tempCall.scriptId;
            if(tempCall.ts > lastts) lastts = tempCall.ts;
          }
        }
      }
    }
    
    timeWidth = lastts - firstts;
    numScripts = scriptIds.size();
    cout << functionCalls.size() << " function calls registered" << endl;
    cout << scriptIds.size() << " script ids registered" << endl;
    cout << "first ts: " << firstts << endl;
    cout << "last ts: " << lastts << endl;
    cout << "time width: " << timeWidth << endl;
    timeCursor = firstts;
    
    // count how many functions are in each script
    for(auto& functionMapPair : functionMap) {
      // add the script to the scriptMap if it does not yet exist
      auto searchScript = find(scripts.begin(), scripts.end(), functionMapPair.second.scriptId);
      if(searchScript == scripts.end()) {
        Script tempScript;
        tempScript.scriptId = functionMapPair.second.scriptId;
        tempScript.numFunctions = 1;
        scripts.push_back(tempScript);
      } else {
        // else increase the number of scripts
        searchScript->numFunctions++;
      }
    }
    // sort scripts after number of functions to find a position for the biggest one first
    std::sort (scripts.begin(), scripts.end());
  }
  
  unordered_map<uint32_t, Function>& getFunctionMap() {
    return functionMap;
  }
  
  vector<Script>& getScripts() {
    return scripts;
  }
  
  void draw() {
    // draw time cursor
    int cursorX = ( double(timeCursor-firstts)/double(timeWidth) ) * ofGetWidth();
    timelineFbo.begin();
    ofBackground(0, 0);
    ofSetColor(255, 255);
    ofDrawRectangle(0, HEIGHT - timelineHeight, cursorX, HEIGHT);
    timelineFbo.end();
    timelineFbo.draw(0, 0);
  }
  
  void setCursor(uint64_t cur) {
    timeCursor = cur;
  }
  
  void togglePlay() {
    playing = !playing;
  }
  
  bool isPlaying() {
    return playing;
  }
  
  float getTimeScale() {
    return timeScale;
  }
  
  void reduceSpeed() {
    timeScale *= 0.9;
    TimelineMessage mess;
    mess.type = "changeSpeed";
    mess.parameters.insert({"speed", timeScale});
    sendViaOsc(mess);
  }
  
  void increaseSpeed() {
    timeScale *= 1.11;
    TimelineMessage mess;
    mess.type = "changeSpeed";
    mess.parameters.insert({"speed", timeScale});
    sendViaOsc(mess);
  }
  
  void click(int x, int y) {
    if(y > HEIGHT - timelineHeight) {
      // move the time cursor to where you clicked on the timeline
      timeCursor = (double(timeWidth)/double(ofGetWidth())) * x + firstts;
      // clear the timeline fbo
      timelineFbo.begin();
      ofBackground(0, 0);
      timelineFbo.end();
    }
  }
  
  float getFramedt() {
    // update time
    // currentTime = ofGetElapsedTimef();
    double localCurrentTime;
    if(!rendering) {
      localCurrentTime = currentTime;
    } else {
      localCurrentTime = renderCurrentTime;
    }
    
    if(lastFrameTime == 0) lastFrameTime = localCurrentTime; // dt will be 0 the first frame
    float dt = localCurrentTime - lastFrameTime;
    lastFrameTime = localCurrentTime;
    return dt;
  }
  
  float getNonScaledFramedt() {
    // update time
    // currentTime = ofGetElapsedTimef();
    nonScaledCurrentTimeMutex.lock();
    double localCurrentTime = nonScaledCurrentTime;
    nonScaledCurrentTimeMutex.unlock();
    
    if(lastNonScaledFrameTime == 0) lastNonScaledFrameTime = localCurrentTime; // dt will be 0 the first frame
    float dt = localCurrentTime - lastNonScaledFrameTime;
    lastNonScaledFrameTime = localCurrentTime;
    return dt;
  }
  
  void progressFrame() {
    renderCurrentTimeMutex.lock();
    renderCurrentTime += (1./frameRate) * timeScale;
    renderCurrentTimeMutex.unlock();
    
    nonScaledCurrentTimeMutex.lock();
    nonScaledCurrentTime += (1./frameRate);
    nonScaledCurrentTimeMutex.unlock();
    progressQueue((1./frameRate) * timeScale);
  }
  
  void startRendering() {
    rendering = true;
  }
  
};