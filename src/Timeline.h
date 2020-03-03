#pragma once

#include "ofMain.h"
#include <memory>
#include "ofxJSON.h"
#include "ofxOsc.h"

#include "FunctionCall.h"
#include "Function.h"
#include "Script.h"
#include "UserEvent.h"

const double timeStepsPerSecond = 1000000;
#define OSC_IN_PORT 12000

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
  map<string, string> stringParameters;
  double ts = 0;
  bool parsed = false;
  
  TimelineMessage() {}
  
  TimelineMessage(float ts_, string t) : 
    ts(ts_), type(t){
  }
  // an initializer_list makes it east to initialise with e.g. { {"id", 4.2 }, {"rad", 0.01 } }
  TimelineMessage(float ts_, string t, std::initializer_list<std::pair<const string, float>> p) : 
    ts(ts_), type(t), parameters(p) {
  }
    
  TimelineMessage(float ts_, string t, std::initializer_list<std::pair<const string, string>> sp) : 
    ts(ts_), type(t), stringParameters(sp) {
  }
    
  bool operator<(const TimelineMessage& t) {
    return this->ts < t.ts;
  }
  
  void print() {
    cout << ts << ": " << type << " {";
    for(auto& pair : parameters) {
      cout << "{" << pair.first << ", " << pair.second << "}";
    }
    cout << "}" << endl;
  }
};

typedef TimelineMessage TM;

class Timeline : public ofThread  {
private:
  ofMutex oscMutex;
  
  double timeCursor = 0.0;
  float timeScale = 0.10;
  uint32_t nextEvent = 0;
  bool playing = false;
  bool rendering = false;
  bool doLoop = true;
  float frameRate = 25;
  double currentTime = 0;
  bool resetLastTime = true;
  ofMutex nonScaledCurrentTimeMutex;
  double nonScaledCurrentTime = 0;
  double lastNonScaledFrameTime = 0;
  double lastFrameTime = 0;
  ofMutex renderCurrentTimeMutex;
  double renderCurrentTime = 0;
  double numTimeStepsToProgress = 0;

  // Filtering out events before and after a certain timestamp
  double filterStart = 4.18;
  double filterEnd = 24.07;
  
  uint64_t firstts = 1000000000000;
  double firstts_d = 0; // in seconds as a decimal number
  uint64_t lastts = 0;
  double lastts_d = 0;
  uint64_t timeWidth = 0;
  double timeWidth_d = 0;
  // stats for this trace
  uint32_t numScripts = 0;
  uint32_t maxScriptId = 0;
  float ratioCallOutIn = 0;
  uint32_t callsWithin = 0;
  uint32_t callsOut = 0;

  vector<TimelineMessage> score;
  
  ofxJSONElement json;
  ofFbo timelineFbo;
  int timelineHeight = 0;
  ofTrueTypeFont font;
  int fontSize;
  int counterStringWidth = 0;
  int WIDTH = 0, HEIGHT = 0;
  
  ofxOscSender oscSender;
  ofxOscMessage oscMess;
  ofxOscReceiver receiver;
  
  // all data
  
  vector<FunctionCall> functionCalls;
  // unordered_map<uint64_t, FunctionCall> callMap;
  map<string, Function> functionMap;
  vector<Script> scripts;
  vector<UserEvent> userEvents;
  

  // the thread function
  void threadedFunction() {

    // start
    while(isThreadRunning()) {
      
      static float lastTime = 0; // the timestamp last time this function was run
      static float nonScaledLastTime = 0;

      receiveOSC(); // receive OSC messages from SuperCollider

      currentTime = ofGetElapsedTimef();
        if(lastTime == 0 || resetLastTime) {
          lastTime = currentTime;
          resetLastTime = false;
        }
      
      double dt = (currentTime-lastTime) * timeScale;
      
      nonScaledCurrentTime += (currentTime-lastTime);
      lastTime = currentTime;
      
      if(!rendering && playing) {
        timeCursor += dt;
        progressQueue(timeCursor);
        if(timeCursor > filterEnd && doLoop) {
          // send a timelineReset message
          TM timelineReset = TM(0, "timelineReset");
          sendViaOsc(timelineReset);
          lock();
          messageFIFO.push_back(timelineReset);
          unlock();
          // reset and go back to the beginning
          timeCursor = filterStart;
          nextEvent = 0;
        }
      }
    }
      // done
  }
  
  void progressQueue(double timeCursor) {    
    if(nextEvent < score.size()) {
      while(score[nextEvent].ts - timeCursor < 0) {
        // handle timeline events
        if(score[nextEvent].type == "time_scale") {
          timeScale = score[nextEvent].parameters["scale"];
        }
        // send it as OSC
        sendViaOsc(score[nextEvent]);
        // lock access to the resource
        lock();
        // put the message in the queue
        messageFIFO.push_back(score[nextEvent]);
        // done with the resource
        unlock();
        
        score[nextEvent].parsed = true;
        //score.erase(score.begin());
        // if(score.size() == 0) break; // otherwise it segfaults
        nextEvent++;
        if(nextEvent >= score.size()) {
          break; // break out of the loop to avoid index out of bound segfault
        }
      }
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
      oscMess.addInt32Arg(mess.parameters["parentScriptId"]);
      oscMess.addInt32Arg(mess.parameters["withinScript"]);
    } else if(mess.type == "changeSpeed") {
      oscMess.addStringArg(mess.type);
      oscMess.addFloatArg(mess.parameters["speed"]);
    } else if(mess.type == "userEvent") {
      oscMess.addStringArg(mess.type);
      oscMess.addStringArg(mess.stringParameters["type"]);
    } else {
      oscMess.addStringArg(mess.type);
    }
    
    oscSender.sendMessage(oscMess);
    oscMutex.unlock();
  }
  
public:
  
  // message queue
  list<TimelineMessage> messageFIFO;
  
  // functions
  void init(int w, int h) {
    WIDTH = w;
    HEIGHT = h;
    timelineHeight = h*0.01;
    
    fontSize = WIDTH/60;
    font.load("SourceCodePro-Regular.otf", fontSize, true, false, true);
    counterStringWidth = font.stringWidth("0.00 s");
    
    oscSender.setup("127.0.0.1", 57120); // send to SuperCollider on the local machine
    sendTimeScale(); // send the timeScale start value
    // init OSC receiver
    receiver.setup(OSC_IN_PORT);

    timeCursor = filterStart;
  }
  
  void parseScriptingProfile(string filepath) {
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
    // ofLog() << json["events"];
    // ofLog() << json["events"][3]["ts"];
    
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
            tempCall.function_id = to_string(tempCall.scriptId) + tempCall.name;
            functionCalls.push_back(tempCall);
            // callMap.insert({tempCall.ts, tempCall});
            scriptIds.insert(tempCall.scriptId);
            
            // create the associated script and store its url
            auto searchScript = find(scripts.begin(), scripts.end(), tempCall.scriptId);
            if(searchScript == scripts.end()) {
              Script tempScript;
              tempScript.scriptId = tempCall.scriptId;
              tempScript.url = nodes[j]["callFrame"]["url"].asString();
              tempScript.extractInfo();
              scripts.push_back(tempScript);
            }
            
            // create the associated function
            auto search = functionMap.find(tempCall.function_id);
            if (search != functionMap.end()) {
                search->second.calledTimes += 1;
            } else {
               Function tempFunc;
               tempFunc.name = tempCall.name;
               tempFunc.id = tempCall.id;
               tempFunc.scriptId = tempCall.scriptId;
               tempFunc.lineNumber = nodes[j]["callFrame"]["lineNumber"].asInt();
               tempFunc.columnNumber = nodes[j]["callFrame"]["columnNumber"].asInt();
               tempFunc.calledTimes = 1;
               functionMap.insert({tempCall.function_id, tempFunc});
            }

            if(tempCall.scriptId > maxScriptId) maxScriptId = tempCall.scriptId;
            if(tempCall.ts > lastts) lastts = tempCall.ts;
          }
        }
      }
    } // json parsing finished

    // find if a function call is within or across scripts
    for(auto& f : functionCalls) {
      auto parent = find(functionCalls.begin(), functionCalls.end(), f.parent);
      if(parent != functionCalls.end()) {
        f.parentScriptId = parent->scriptId; // store the scriptId of the parent for later
        auto parentFunc = functionMap.find(parent->function_id);
        parentFunc->second.callingTimes += 1;
        // set the FunctionCall variable and Function stats
        if(parent->scriptId == f.scriptId) {
          f.withinScript = true;
          parentFunc->second.numCallsWithinScript += 1;
        } else {
          f.withinScript = false;
          parentFunc->second.numCallsToOtherScript += 1;
          auto thisFunc = functionMap.find(f.function_id);
          thisFunc->second.numCalledFromOutsideScript += 1;
        }
      }
      else {
        ofLogNotice("Timeline::parseScriptingProfile") << "parent function call to functionCall not found, parent id: " << f.parent;
        f.parentScriptId = 0;
      }
    }

    vector<Function> functionVector;
    // create scripts
    // count how many functions are in each script
    for(auto& functionMapPair : functionMap) {
      auto& func = functionMapPair.second;
      functionVector.push_back(func);
      // add the script to the scriptMap if it does not yet exist
      auto script = find(scripts.begin(), scripts.end(), func.scriptId);

      // create script if it doesn't exist
      if(script == scripts.end()) {
        Script tempScript;
        tempScript.scriptId = functionMapPair.second.scriptId;
        tempScript.numFunctions = 0;
        scripts.push_back(tempScript);
        script = scripts.end();
      } 

      // add stats to script
      script->numFunctions++;
      script->numCallsWithinScript += func.numCallsWithinScript;
      script->numCallsToOtherScript += func.numCallsToOtherScript;
      script->numCalledFromOutsideScript += func.numCalledFromOutsideScript;
    }

    // add which script a certain script is called from or calls the most
    for(auto& f : functionCalls) {
      auto thisScript = find(scripts.begin(), scripts.end(), f.scriptId);
      // set the first time the script is called
      double functionts = double(f.ts-firstts)/timeStepsPerSecond;
      if(thisScript->firstCalled > functionts) thisScript->firstCalled = functionts;
      if(!f.withinScript) {
        auto parent = find(functionCalls.begin(), functionCalls.end(), f.parent);
        auto parentScript = find(scripts.begin(), scripts.end(), parent->scriptId);
        auto searchParent = parentScript->toScriptCounter.find(f.scriptId);
        if(searchParent != parentScript->toScriptCounter.end()) {
          searchParent->second++;
        } else {
          parentScript->toScriptCounter.insert({f.scriptId, 1});
        }
        auto searchThis = thisScript->fromScriptCounter.find(parent->scriptId);
        if(searchThis != thisScript->fromScriptCounter.end()) {
          searchThis->second++;
        } else {
          thisScript->fromScriptCounter.insert({parent->scriptId, 1});
        }
      }
    }

    // set phases for scripts
    for(auto& script : scripts) {
      if(script.firstCalled < 2.13) {
        script.phase = 0;
      } else if(script.firstCalled < 3.25) {
        script.phase = 1;
      } else {
        script.phase = 2;
      }
      if(script.scriptType == "built-in") {
        script.phase = -1;
      }
    }

    cout << "** Functions sorted by how many times they are called: **" << endl << endl;
    functionVector[0].printHeaders();
    std::sort (functionVector.begin(), functionVector.end());
    for(auto& func : functionVector) {
      func.print();
    }
    
    // sort scripts after number of functions to find a position for the biggest one first
    std::sort (scripts.begin(), scripts.end());
    cout << endl << endl << "** Scripts sorted by when they are first called: **" << endl << endl;
    scripts[0].printHeaders();
    for(auto& script : scripts) {
      script.calculateInterconnectedness();
      script.print();
      // script.printToAndFrom();
    }

    cout << endl << endl << "** Functions within script 317: **" << endl << endl;
    for(auto& func : functionVector) {
      if(func.scriptId == 317)
        func.print();
    }

    callsWithin = 0;
    callsOut = 0;
    for(auto& f : functionCalls) {
      if(f.withinScript) callsWithin++;
      else callsOut++;
    }
    ratioCallOutIn = (float)callsOut/(float)callsWithin;

    timeWidth = lastts - firstts;
    timeWidth_d = double(timeWidth)/timeStepsPerSecond;
    firstts_d = double(firstts)/timeStepsPerSecond;
    lastts_d = double(lastts)/timeStepsPerSecond;
    numScripts = scriptIds.size();
    cout << "calls within script: " << callsWithin << endl;
    cout << "calls out from script: " << callsOut << endl;
    cout << "ratio out/within: " << ratioCallOutIn << endl;
    cout << functionCalls.size() << " function calls registered" << endl;
    cout << functionMap.size() << " functions registered" << endl;
    cout << scriptIds.size() << " script ids registered" << endl;
    cout << "first ts: " << firstts << endl;
    cout << "last ts: " << lastts << endl;
    cout << "time width: " << timeWidth << endl;
    cout << "first ts: " << firstts_d << endl;
    cout << "last ts: " << lastts_d << endl;
    cout << "time width: " << timeWidth_d << endl;
  }
  void parseUserEventProfile(string filepath) {
    // load and parse the json data in the path provided
    bool parsingSuccessful = json.open(filepath);
    if (parsingSuccessful) {
        ofLogNotice("ofApp::setup JSON parsing successful");
    }
    else {
        ofLogNotice("ofApp::setup")  << "Failed to parse JSON" << endl;
    }

    if (json["events"].isArray()) {
      const Json::Value& events = json["events"];
      for (Json::ArrayIndex i = 0; i < events.size(); ++i) {
        UserEvent tempEvent;
        tempEvent.type = events[i]["name"].asString();
        tempEvent.ts = events[i]["ts"].asLargestUInt();
        userEvents.push_back(tempEvent);
      }
    } // json parsing finished
    
  }

  void generateScore() {
    score.clear();

    for(auto& f : functionCalls) {
      double timestamp = double(f.ts-firstts)/timeStepsPerSecond;
      if(timestamp > filterStart && timestamp < filterEnd) {
          TM tempMessage = TM(
          timestamp,
          "functionCall",
          {
            {"id", f.id},
            {"parent", f.parent},
            {"scriptId", f.scriptId},
            {"parentScriptId", f.parentScriptId},
            {"withinScript", int(f.withinScript)},
          }
        );
        tempMessage.stringParameters.insert({"function_id", f.function_id});
        score.push_back(tempMessage);
      }
    }
    for(auto& u : userEvents) {
      double timestamp = double(u.ts-firstts)/timeStepsPerSecond;
      if(timestamp > filterStart && timestamp < filterEnd) {
        score.push_back(TM(
          timestamp,
          "userEvent",
          {
            {"type", u.type},
          }
        ));
      }
    }
    std::sort (score.begin(), score.end());

    // ofLogNotice("Finished score");
    // for(auto& s : score) {
    //   s.print();
    // }
    progressScoreIndexToCursor();
  }

  void sendBackgroundInfoOSC() {
    oscMutex.lock();

    // send all script data
    for(auto& script : scripts) {
      oscMess.clear();
      oscMess.setAddress("/script");
      oscMess.addInt32Arg(script.scriptId);
      oscMess.addInt32Arg(script.numFunctions);
      oscMess.addStringArg(script.scriptType);
      oscMess.addStringArg(script.name);
      for(auto& inter : script.scriptInterconnectedness) {
        oscMess.addInt32Arg(inter.first);
        oscMess.addFloatArg(inter.second);
      }
      oscSender.sendMessage(oscMess); 
    }
    
    oscMutex.unlock();
  }
  
  void sendActivityDataOSC() {
    oscMutex.lock();
    oscMess.clear();
    oscMess.setAddress("/functionCallTransmission");

    // send all script data
    for(auto& f : functionCalls) {
      // send the timing of the function call as value between 0 and 1
      oscMess.addDoubleArg(double(f.ts-firstts)/timeWidth);
    }
    oscSender.sendMessage(oscMess); 
    oscMutex.unlock();
  }
  
  map<string, Function>& getFunctionMap() {
    return functionMap;
  }
  
  vector<Script>& getScripts() {
    return scripts;
  }
  
  vector<FunctionCall>& getFunctionCalls() {
    return functionCalls;
  }
  
  vector<UserEvent>& getUserEvents() {
    return userEvents;
  }
  
  void draw() {
    // draw time cursor
    int cursorX = ( timeCursor/timeWidth_d ) * WIDTH;
    // timelineFbo.begin();
    // ofBackground(0, 0);
    // ofSetColor(255, 255);
    timelineHeight = HEIGHT*0.01;
    int y = HEIGHT - timelineHeight;
    ofDrawRectangle(0, y, cursorX, timelineHeight);
    // timelineFbo.end();
    // timelineFbo.draw(0, 0);
    
    // int textX = ofClamp(cursorX-(fontSize*2), 0, WIDTH-(fontSize*10)); // position relative to cursor
    
    int textHeight = HEIGHT * 0.04;
    int textY = HEIGHT * 0.05;
    int textX = WIDTH - textY - counterStringWidth;
    textY += HEIGHT * 0.01;
    textY = HEIGHT - (textY*1.5);
    // int textX = WIDTH*0.8;
    std::ostringstream out;
    out.precision(2);
    out << fixed;
    out << timeCursor;
    font.drawString(out.str() + " s", textX, textY);
    out.str("");
    out.clear();
    out.precision(2);
    out << timeScale;
    int numDigits = out.str().size();
    // int scaleX = ofClamp(cursorX-(fontSize*(float(numDigits)/2.)), 0, WIDTH-(fontSize*numDigits)); // TODO: center on end of timeline
    font.drawString(out.str() + " x", textX, textY + textHeight);
  }
  
  void setCursor(uint64_t cur) {
    timeCursor = cur;
  }
  
  void togglePlay() {
    if(!playing) {
      // send all background info to SuperCollider again
      // This allows you to restart the SC code without having to restart the OF program
      startPlaying();
    } else {
      stopPlaying();
    }
    playing = !playing;
  }

  void startPlaying() {
      sendTimeScale();
      sendBackgroundInfoOSC();
      TimelineMessage mess;
      mess.type = "startPlaying";
      lock();
      messageFIFO.push_back(mess);
      unlock();
      sendViaOsc(mess);
      resetLastTime = true;
  }
  
  void stopPlaying() {
      TimelineMessage mess;
      mess.type = "stopPlaying";
      lock();
      messageFIFO.push_back(mess);
      unlock();
      sendViaOsc(mess);
      resetLastTime = true;
  }
  
  void setLoop(bool b) {
    doLoop = b;
  }
  
  bool isPlaying() {
    return playing;
  }
  
  float getTimeScale() {
    return timeScale;
  }
  
  void reduceSpeed() {
    timeScale *= 0.9;
    sendTimeScale();
  }
  
  void increaseSpeed() {
    timeScale *= 1.11;
    sendTimeScale();
  }

  void setTimeScale(float t, bool propagate = true) {
    timeScale = t;
    // if the timeScale value was received from SC there is no point in sending it back
    sendTimeScale(propagate);
  }
  
  void sendTimeScale(bool viaOsc = true) {
    TimelineMessage mess;
    mess.type = "changeSpeed";
    mess.parameters.insert({"speed", timeScale});
    lock();
      messageFIFO.push_back(mess);
    unlock();
    
    if(viaOsc) sendViaOsc(mess);
  }
  
  void click(int x, int y) {
    // if(y > HEIGHT - timelineHeight) {
      // move the time cursor to where you clicked on the timeline
      timeCursor = (double(timeWidth_d)/double(ofGetWidth())) * x;
      progressScoreIndexToCursor();
      // clear the timeline fbo
      // timelineFbo.begin();
      // ofBackground(0, 0);
      // timelineFbo.end();
    // }
  }

  void progressScoreIndexToCursor() {
      // set next event
      int newNextEvent = 0;
      while(score[newNextEvent].ts - timeCursor < 0) {
        newNextEvent++;
      }
      nextEvent = newNextEvent;
  }

  double getTimeCursor() {
    return timeCursor;
  }

  double getTimeWidth() {
    return timeWidth_d;
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
    timeCursor += (1./frameRate) * timeScale;
    progressQueue(timeCursor);
  }
  
  void startRendering() {
    rendering = true;
    playing = true;
  }
  void stopRendering() {
    rendering = false;
  }
  
  uint64_t getFirstts() {
    return firstts;
  }

  void receiveOSC() {
    // check for waiting messages
    while(receiver.hasWaitingMessages()){

      // get the next message
      ofxOscMessage m;
      receiver.getNextMessage(m);

      // check for mouse moved message
      if(m.getAddress() == "/timeScale"){
        float t = m.getArgAsFloat(0);
        setTimeScale(t, false);
      }
    }
  }
  
};