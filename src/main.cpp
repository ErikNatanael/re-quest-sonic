#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofGLWindowSettings settings;
	settings.setGLVersion(3,3); // required for OpenGL 3.3 shaders to work
	// settings.setSize(1920, 1080);
	settings.setSize(3840, 2160);
	//settings.windowMode = OF_GAME_MODE;
	settings.windowMode = OF_WINDOW;
	ofCreateWindow(settings);
	
	// ofSetupOpenGL(1920,1080,OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
