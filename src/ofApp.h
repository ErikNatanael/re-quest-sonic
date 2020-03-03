#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include "Timeline.h"
#include "FunctionCall.h"
#include "Function.h"
#include "Script.h"
#include "UserEvent.h"
#include "ModelExport.h"
#include "Shapes.h"
#include "SmoothVideoPlayer.h"

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

		void receiveOSC();
		ofxOscReceiver receiver;
		
		// GUI related functions
		void setupGui();
		void saveSVGButtonPressed();
		void saveTrianglesSVG();
		void sendActivityDataOSC();
		void doLoopToggleFunc(bool &b);
		void toggleDoDrawGraphics(bool &b);
		void updateScaling(float& scaling);
		
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
		void exportMeshGridPiece();
		void generateMesh();
		void regenerateMesh();
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
		
		ofParameter<int> graphX = 0;
		ofParameter<int> graphY = 0;
		ofParameter<float> graphScaling = 1;
		vector<ofIcoSpherePrimitive> scriptSpheres;
		vector<ofIcoSpherePrimitive> funcSpheres;
		ofCamera cam;
		ofEasyCam easyCam;
		ofMesh mesh;
		ofParameter<float> functionPointOffsetRatio;
		bool doSpiralPositions = false;
		bool doTrianglePositions = true;

		SmoothVideoPlayer pauseVideo;
		float pauseVideoPosition;
		SmoothVideoPlayer traceVideo;
		ofShader flipShader;
		ofParameter<float> videoOffset;

		ofTrueTypeFont font;
		
		list<TimelineMessage> messageFIFOLocal;
		
		vector<Screenshot> screenshots;
		size_t currentScreen = 0;
		
		ofFbo functionCallFbo;
		vector<FunctionCall> functionCallsToDraw;
		
		ofShader invertShader;

		Triangle triangle;
		vector<Triangle> triangles;
		vector<Circle> circles;

		ofParameter<bool> drawFunctionCallsOneAtATime = false;
		ofParameter<size_t> currentFunctionCall = 0; // for drawing function calls one at a time
		
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
		ofxButton regenerateMeshButton;
		ofxButton exportMeshButton;
		ofxButton exportMeshGridButton;
		ofxButton exportMeshGridPieceButton;
		ofxButton exportTrianglesSVGButton;
		bool doDrawGraphics = true;
		ofParameter<bool> doDrawScreenshots;
		ofParameter<int> numScriptsToDraw;
		ofParameter<bool> showTriangle;
		ofParameter<float> triangleScale;
		ofParameter<bool> showMesh;
		ofParameter<int> meshGridPieceX, meshGridPieceY;
};
