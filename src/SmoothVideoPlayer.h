#pragma once

#include "ofMain.h"
#include "ofxHapPlayer.h"

/* ofVideoPlayer has the following useful functions:
   - setFrame(int)
   - setPosition(float) // in percent
   - firstFrame()
   - getTotalNumFrames()
   - play
   */ 

class SmoothVideoPlayer {
  ofxHapPlayer video;
	ofTexture texture;
  ofFbo videoFbo;
  float speed = 0.1;

  ofParameter<float> frameAlpha;

  
public:

  void init(string path, int WIDTH, int HEIGHT) {
    video.load(path);
    video.setVolume(0.0);
    videoFbo.allocate(WIDTH, HEIGHT, GL_RGB);
    videoFbo.begin();
    ofBackground(0, 255);
    videoFbo.end();
    frameAlpha = 30;
    video.setLoopState(OF_LOOP_NONE);
    video.play();
    video.setPaused(true);
  }
  void update() {
    // unnecessary for ofxHapPlayer?
    video.update();
  }

  void draw(int WIDTH, int HEIGHT) {
    videoFbo.begin();
    ofSetColor(255, frameAlpha);
    video.draw(0, 0, WIDTH, HEIGHT);
    videoFbo.end();
    videoFbo.draw(0, 0);
  }

  void setSpeed(float s) {
    speed = s;
    video.setSpeed(speed);
    frameAlpha = 255*speed;
  }

  void setPosition(float pos, float timeOffset) {
    // convert seconds to frames
    // use a position offset to 
    // int frame = (pos + timeOffset) * 29.97;
    // if(frame > video.getTotalNumFrames() - 1) frame = video.getTotalNumFrames() - 1;
    // video.setFrame(frame);
    // ofLogNotice("setPosition") << "video frame: " << frame;

    float videoPosition = (pos + timeOffset)/video.getDuration();
    videoPosition = ofClamp(videoPosition, 0.0, 1.0);
    video.setPosition(videoPosition);
  }

  void play() {
    video.setPaused(false);
  }

  void stop() {
    video.setPaused(true);
  }
};