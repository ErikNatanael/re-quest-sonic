#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include "Timeline.h"
#include "FunctionCall.h"
#include "Function.h"
#include "Script.h"
#include "UserEvent.h"
#include "GravityPlane.h"

class Screenshot {
public:
	ofImage img;
	double ts;
	
	bool operator<(const Screenshot& s) {
    return this->ts < s.ts;
  }
};

class Triangle {
	glm::vec2 p1, p2, p3;
	float sign (glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
	{
			return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
	}
	bool isCircleInside (glm::vec2 pt, float r)
	{
		// distance between center of circle to each line in the triangle
		// if the distance if less than the radius of the circle and the 
		// circle center is inside the triangle we are good
		if(
			// test if either of the sides are closer to the point than r
			false
		) {
			return false;
		}

			float d1, d2, d3;
			bool has_neg, has_pos;

			d1 = sign(pt, p1, p2);
			d2 = sign(pt, p2, p3);
			d3 = sign(pt, p3, p1);

			has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
			has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

			return !(has_neg && has_pos);
	}
	void draw() {
		ofDrawTriangle(p1, p2, p3);
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
		void doLoopToggleFunc(bool &b);
		void toggleDoDrawGraphics(bool &b);
		
		void drawStaticPointsOfFunctions(); // draws a circle for each function
		void drawStaticPointsOfScripts(bool drawCenters = false); // draws a circle for each script
		void drawStaticFunctionCallLines();
		void drawSingleStaticFunctionCallLine(string function_id, int parent, int scriptId);
		void drawStaticRepresentation(); // draws a static representation of the call graph data
		void drawThickPolyline(ofPolyline line, float width);
		ofColor getColorFromScriptId(int scriptId, int alpha);
		ofParameter<uint16_t> hueOffset;
		ofParameter<uint16_t> hueRotation;
		ofParameter<uint8_t> saturation;
		ofParameter<uint8_t> brightness;
		
		void drawMesh();
		void exportMesh();
		void exportMeshGrid();
		void generateMesh();
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
		
		int graphX = 0;
		int graphY = 0;
		vector<ofIcoSpherePrimitive> scriptSpheres;
		vector<ofIcoSpherePrimitive> funcSpheres;
		ofCamera cam;
		ofEasyCam easyCam;
		ofMesh mesh;

		ofTrueTypeFont font;
		
		list<TimelineMessage> messageFIFOLocal;
		
		vector<Screenshot> screenshots;
		size_t currentScreen = 0;
		
		ofFbo functionCallFbo;
		vector<FunctionCall> functionCallsToDraw;
		
		ofShader invertShader;
		
		/// RENDERING
		bool rendering = false;
		int frameNumber = 0;
		string renderDirectory;
		ofFbo renderFbo;
		ofPixels renderPixels;
		ofImage grabImg;
		
		// GUI
		bool showGui = true;
		ofxPanel gui;
		ofxButton saveSVGButton;
		ofxButton sendActivityEnvelopeToSCButton;
		ofxToggle doLoopToggle;
		ofxToggle doGraphicsToggle;
		ofxButton exportMeshButton;
		ofxButton exportMeshGridButton;
		bool doDrawGraphics = true;
		ofParameter<int> numScriptsToDraw;
};
