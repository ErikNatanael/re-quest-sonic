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
    bool paused = true;

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
        video.setPaused(paused);
    }

    void draw(int WIDTH, int HEIGHT) {
        videoFbo.begin();
        ofSetColor(255, frameAlpha);
        video.draw(0, 0, WIDTH, HEIGHT);
        videoFbo.end();
        videoFbo.draw(0, 0);
    }

    // frequently changing speed seems to lead to crashes so
    // it is safer to just use the Timeline clock and set the position
    // of the video every frame.
    // Only use this to set the frame alpha
    void setSpeed(float s) {
        speed = s;
        frameAlpha = ofClamp(150*s, 10, 255);
    }

    void setPosition(float pos, float timeOffset) {
        float videoPosition = (pos + timeOffset)/video.getDuration();
        videoPosition = ofClamp(videoPosition, 0.0, 1.0 - (1./30.));
        video.setPosition(videoPosition);
    }

    float getDuration() {
        return video.getDuration();
    }

    void setAlpha(float alpha) {
        frameAlpha = alpha;
    }
};