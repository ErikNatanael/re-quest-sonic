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

class Circle {
public:
	glm::vec2 p;
	float r = 1;
	void draw() {
		ofDrawCircle(p.x, p.y, r);
	}
	bool circleOverlaps(Circle& c) {
		return glm::distance(p, c.p) < (r + c.r);
	}
};

class Triangle {
public:
	glm::vec2 p1, p2, p3;

	// https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
	float sign (glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
	{
		// signed area of the triangle formed by 3 points
		return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
	}
	float pointToLineDistance(glm::vec2 p, glm::vec2 l1, glm::vec2 l2) {
		// https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
		return abs((l2.y-l1.y)*p.x - (l2.x-l1.x)*p.y + l2.x*l1.y - l2.y*l1.x) / sqrt(pow(l2.y-l1.y, 2) + pow(l2.x - l1.x, 2));
	}
	bool isCircleInside (glm::vec2 pt, float r)
	{
		// distance between center of circle to each line in the triangle
		// if the distance if less than the radius of the circle and the 
		// circle center is inside the triangle we are good
		if(
			// test if either of the sides are closer to the point than r
			pointToLineDistance(pt, p1, p2) < r ||
			pointToLineDistance(pt, p2, p3) < r ||
			pointToLineDistance(pt, p1, p3) < r
		) {
			return false;
		}

			float d1, d2, d3;
			bool has_neg, has_pos;

			// get the signed area of every edge and the point tested
			d1 = sign(pt, p1, p2);
			d2 = sign(pt, p2, p3);
			d3 = sign(pt, p3, p1);

			// if all the areas are either negative or positive the point is in the triangle
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
		void regenerateMesh(float& f);
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
		ofParameter<float> functionPointOffsetRatio;
		bool doSpiralPositions = true;
		bool doTrianglePositions = false;

		ofTrueTypeFont font;
		
		list<TimelineMessage> messageFIFOLocal;
		
		vector<Screenshot> screenshots;
		size_t currentScreen = 0;
		
		ofFbo functionCallFbo;
		vector<FunctionCall> functionCallsToDraw;
		
		ofShader invertShader;

		Triangle triangle;
		vector<Circle> circles;
		
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
		ofParameter<bool> doDrawScreenshots;
		ofParameter<int> numScriptsToDraw;
};
