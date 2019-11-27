#pragma once

#include "ofMain.h"

class Camera2D {
public:
  glm::vec2 offsetPos;
  glm::vec2 offsetPosLow;
  glm::vec2 currentPos;
  glm::vec2 targetPos;
  
  void update(float dt) {
    glm::vec2 moveVector = targetPos - currentPos;
    currentPos += moveVector*dt;
    offsetPos = glm::vec2(ofGetWidth()/2, ofGetHeight()/2) - currentPos;
    offsetPosLow = offsetPos * 0.95;
  }
};