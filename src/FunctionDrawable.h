#pragma once

#include "ofMain.h"
#include "Camera2D.h"

class FunctionDrawable {
public:
  int id;
  string name;
  int scriptId;
  int calledTimes = 0;
  int lineNumber;
  int columnNumber;
  
  float radius = 5;
  bool activated = false; // activated if it has been called at least once
  glm::vec2 pos;
  glm::vec2 vel;
  glm::vec2 acc;
  float alpha = 0;
  float alphaVel = 100;
  float maxAlpha = 200;
  float minAlpha = 30;
  
  // boundaries
  glm::vec2 boundCenter;
  float boundRadius;
  
  FunctionDrawable() {
    //vel = glm::vec2(ofRandom(-10, 10), ofRandom(-10, 10));
  }
  
  void activate() {
    activated = true;
    alphaVel = ofRandom(900, 1300);
  }
  
  void reset() {
    activated = false;
    vel = glm::vec2(0, 0);
    acc = glm::vec2(0, 0);
  }
  
  void update(float dt) {
    alpha += alphaVel*dt;
    if(alpha > maxAlpha) {
      alpha = maxAlpha;
      alphaVel *= -1;
    } else if (alpha < minAlpha) {
      alpha = minAlpha;
      alphaVel *= -1;
    }
    alphaVel -= alphaVel*0.1*dt;
    
    vel += acc*dt;
    acc -= acc*dt;
    pos += vel*dt;
    glm::vec2 distanceVec = boundCenter - pos;
    float distance = glm::distance(pos, boundCenter);
    
    if(distance + radius > boundRadius) {
      
      // first move the function ball back into bounds
      glm::vec2 correction = glm::normalize(distanceVec)*(boundRadius - distance - radius);
      pos = pos - correction;
      
      // bounce the ball against the wall of the boundary circle with a randomised direction
      float theta = atan2(distanceVec.y, distanceVec.x);
      // add some randomness
      theta += ofRandom(TWO_PI*-0.25, TWO_PI*0.25);
      float sine = sin(theta);
      float cosine = cos(theta);
      // apply rotation matrix to velocity
      // glm::vec2 tempVel = glm::vec2(
      //   cosine * vel.x - sine * vel.y,
      //   cosine * vel.y + sine * vel.x
      // );
      // vel = tempVel;
      
      // TODO: use rotation matric instead of the expensive sqrt
      float speed = sqrt(vel.x*vel.x + vel.y*vel.y);
      vel = glm::vec2(cosine*speed, sine*speed);

    }
  }
  
  void drawForeground(FunctionDrawable& parent, Camera2D& camera2d) {
    ofSetColor(30, 255, 70, 150);
    ofDrawLine(pos, parent.pos);
    ofDrawCircle(pos, radius*1.2);
  }
  
  void drawLineBackground(FunctionDrawable& parent, Camera2D& camera2d) {
    ofSetColor(30, 70, 150, 200);
    ofDrawLine(pos, parent.pos);
  }
  
  void drawShapeBackground(Camera2D& camera2d) {
    if(activated) {
      ofSetColor(123, 123, 255, alpha);
    }
    else {
      ofSetColor(80, 80, 250, 50);
    }
    ofDrawCircle(pos, radius);
  }
};