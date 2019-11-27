#pragma once

#include "ofMain.h"
#include "ofShader.h"

// this code is based on the ZoomBlurPass from ofxPostProcessing by satcy, http://satcy.net
// there is a license that might apply

class FocusShader {
public:
  
  ofShader shader;
  float centerX = 0;
  float centerY = 0;
  float exposure = 0.48;
  float decay = 0.9;
  float density = .01;
  float weight = 0.5;
  float clamp = 1;
  
  void init() {
    shader.load("shaders/focusShader/shader");
  }
  
  void render(ofFbo& readFbo, ofFbo& writeFbo)
  {
      writeFbo.begin();
      shader.begin();
      
      shader.setUniformTexture("tDiffuse", readFbo.getTextureReference(), 1);
      shader.setUniform1f("fX", centerX);
      shader.setUniform1f("fY", centerY);
      shader.setUniform1f("fExposure", exposure);
      shader.setUniform1f("fDecay", decay);
      shader.setUniform1f("fDensity", density);
      shader.setUniform1f("fWeight", weight);
      shader.setUniform1f("fClamp", clamp);
      shader.setUniform2f("resolution", ofGetWidth(), ofGetHeight());
      
      ofSetColor(255, 255);
      ofRect(0, 0, writeFbo.getWidth(), writeFbo.getHeight());
      
      shader.end();
      writeFbo.end();
  }
  
};