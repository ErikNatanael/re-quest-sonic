#pragma once

#include "ofMain.h"

#include "Timeline.h"
#include "FunctionCall.h"
#include "Function.h"
#include "Script.h"

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
		
		Timeline timeline;

		int WIDTH = 1920;
		int HEIGHT = 1080;

		ofTrueTypeFont font;
		
		list<TimelineMessage> messageFIFOLocal;
		
};
