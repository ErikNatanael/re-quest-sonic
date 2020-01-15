#pragma once
#include "ofMain.h"
#include "Script.h"
#include "Function.h"

// export function and script positions as a 3d plane where points are 
// displaced based on their distance to scripts and points



// float clamp(float x, float lowerlimit, float upperlimit) {
//   if (x < lowerlimit)
//     x = lowerlimit;
//   if (x > upperlimit)
//     x = upperlimit;
//   return x;
// }
// 
// float smoothstep(float edge0, float edge1, float x) {
//   // Scale, bias and saturate x to 0..1 range
//   x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0); 
//   // Evaluate polynomial
//   return x * x * (3 - 2 * x);
// }

class AttractionPoint {
public:
  glm::vec2 pos;
  float gravity; // the strength of the gravity pull
  float radius; // the maximum radius of influence over which the gravity strength is interpolated
  AttractionPoint(glm::vec2 p, float g, float r): pos(p), gravity(g), radius(r) {}
  
  float getGravity(glm::vec2 p) {
    float distance = glm::distance(pos, p);
    if(distance > radius) return 0.0f;
    float amount = 1. - (distance/radius);
    amount = pow(amount, 3.);
    amount = smoothcurve2(amount);
    return gravity * amount;
  }
  // these functions assume a value 0-1
  float smoothcurve1(float x) {
    return x * x * (3 - 2 * x);
  }
  float smoothcurve2(float x) {
    return x * x * x * (x * (x * 6 - 15) + 10);
  }
};

class GravityPlane {
public:
  vector<AttractionPoint> points;
  int maxScriptId;
  int size = 1500;
  
  ofMesh generateMesh() {
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);
    int stepSize = 5;
    int width = size/stepSize, height = size/stepSize;
    for (int y = 0; y < height; y++){
      for (int x = 0; x<width; x++){
        float offset = getGravityAtPoint(glm::vec2(x*stepSize, y*stepSize));
        mesh.addVertex(ofPoint(x*stepSize,offset,y*stepSize)); // make a new vertex
        mesh.addColor(ofFloatColor(1,.5,1));  // add a color at that vertex
      }
    }

    // now it's important to make sure that each vertex is correctly connected with the
    // other vertices around it. This is done using indices, which you can set up like so:
    for (int y = 0; y<height-1; y++){
      for (int x=0; x<width-1; x++){
        mesh.addIndex(x+y*width);               // 0
        mesh.addIndex((x+1)+y*width);           // 1
        mesh.addIndex(x+(y+1)*width);           // 10

        mesh.addIndex((x+1)+y*width);           // 1
        mesh.addIndex((x+1)+(y+1)*width);       // 11
        mesh.addIndex(x+(y+1)*width);           // 10
      }
    }
    return mesh;
  }
  
  void addScriptPoint(Script& s) {
    float scale = 0.7;
    float offset = (1.-pow(1.-s.getSize(), 2.0)) * size * scale * 0.15;
    glm::vec2 pos = s.getSpiralCoordinate(maxScriptId, size*scale) + glm::vec2(size*0.5, size*.5);
    float radius = (1.-pow(1.-s.getSize(), 2.0)) * size * scale * .5;
    points.push_back(AttractionPoint(
      pos,
      offset,
      radius
    ));
    s.meshRadius = radius * 0.25;
    s.meshPos = pos;
  }
  
  void addFunctionPoint(Function& f, Script& s) {
    float offset = size * -0.04;
    // recalculate the pos relative to the mesh size instead of reusing the one meant for visual display
    glm::vec2 scriptPos = s.meshPos;
    float scriptSize = s.meshRadius;
    glm::vec2 funcRelativePos = f.getRelativeSpiralPos();
    glm::vec2 pos = scriptPos + (funcRelativePos * scriptSize);
    points.push_back(AttractionPoint(
      pos,
      offset,
      size * .005
    ));
  }
  
  float getGravityAtPoint(glm::vec2 p) {
    float gravity = 0;
    for(auto& a : points) {
      gravity += a.getGravity(p);
    }
    return gravity;
  }
};