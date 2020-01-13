#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include "Timeline.h"
#include "FunctionCall.h"
#include "Function.h"
#include "Script.h"
#include "UserEvent.h"

class Screenshot {
public:
	ofImage img;
	double ts;
	
	bool operator<(const Screenshot& s) {
    return this->ts < s.ts;
  }
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void mouseScrolled(int x, int y, float scrollX, float scrollY);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void exit();
		
		// GUI related functions
		void setupGui();
		void saveSVGButtonPressed();
		void sendActivityDataOSC();
		
		void drawStaticPointsOfFunctions(); // draws a circle for each function
		void drawStaticPointsOfScripts(); // draws a circle for each script
		void drawStaticFunctionCallLines();
		void drawSingleStaticFunctionCallLine(string function_id, int parent, int scriptId);
		void drawStaticRepresentation(); // draws a static representation of the call graph data
		void drawThickPolyline(ofPolyline line, float width);
		// data structures containing a copy of the data used in Timeline in order to draw a static representation
		vector<FunctionCall> functionCalls;
	  map<string, Function> functionMap;
	  vector<Script> scripts;
	  vector<UserEvent> userEvents;
		uint32_t maxScriptId = 0;
		void drawSpiral(); // function to test spiral shape
		
		Timeline timeline;

		int WIDTH = 1920;
		int HEIGHT = 1080;

		ofTrueTypeFont font;
		
		list<TimelineMessage> messageFIFOLocal;
		
		vector<Screenshot> screenshots;
		size_t currentScreen = 0;
		
		ofFbo functionCallFbo;
		vector<FunctionCall> functionCallsToDraw;
		
		// GUI
		bool showGui = true;
		ofxPanel gui;
		ofxButton saveSVGButton;
		ofxButton sendActivityEnvelopeToSCButton;
};
