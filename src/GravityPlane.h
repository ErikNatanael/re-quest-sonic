#pragma once
#include "ofMain.h"
#include "Script.h"
#include "Function.h"
#include <unordered_set>

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

enum class HoleRelation {HOLE = 0, WALL = 1, NONE = 2};
class HolePoint {
public:
  glm::vec2 pos;
  float wallRadius;
  float holeRadius;
  HolePoint(glm::vec2 p, float wr, float hr): pos(p), wallRadius(wr), holeRadius(hr) {}

  // returns -1 if the 
  HoleRelation insideHole(glm::vec2 p) {
    float distance = glm::distance(pos, p);
    if(distance < holeRadius) return HoleRelation::HOLE;
    else if (distance < wallRadius) return HoleRelation::WALL;
    return HoleRelation::NONE;
  }
};

class GravityPlane {
public:
  vector<AttractionPoint> points;
  vector<HolePoint> holePoints;
  int maxScriptId;
  int size = 6000;
  float scale = 0.4; // used to scale things down so they don't overshoot the edge
  bool addBasePlate = true;
  bool functionsAsHoles = true;
  int holeYPos = -4000;
  glm::vec2 pointOffset;

  GravityPlane() {
    points.push_back(AttractionPoint(
      glm::vec2(0, 0),
      200,
      size*0.6
    ));
    pointOffset = glm::vec2(0, -0.25); // compensate for an off center mid point
  }

  ofMesh generateMesh() {
    int stepSize = 5;
    int width = size/stepSize, height = size/stepSize;
    return generateMesh(-width/2, width/2, -height/2, height/2, stepSize);
  }
  
  ofMesh generateMesh(int minW, int maxW, int minH, int maxH, int stepSize) {
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);

    int baseY = -2;
    
    // plane vertexes
    for (int y = minH; y < maxH; y++){
      for (int x = minW; x<maxW; x++){
        float offset = getGravityAtPoint(glm::vec2(x*stepSize, y*stepSize));
        // if(offset < 0) offset = 0; // avoid function holes extending under the model
        HoleRelation hr = getHole(glm::vec2(x*stepSize, y*stepSize));
        if(hr == HoleRelation::HOLE) {
          offset = holeYPos;
        }
        else if(hr == HoleRelation::WALL) offset = baseY;
        mesh.addVertex(ofPoint(x*stepSize,offset,y*stepSize)); // make a new vertex
        mesh.addColor(ofFloatColor(1,.5,1));  // add a color at that vertex
      }
    }

    // add wall vertices on the sides to make a box
    // top
    int y = minH;
    int x = minW;
    for (x = minW; x<maxW; x++){
      mesh.addVertex(ofPoint(x*stepSize, baseY, y*stepSize)); // make a new vertex at the base
      mesh.addColor(ofFloatColor(1,.5,0));  // add a color at that vertex
    }
    // right
    x = maxW-1;
    for (int y = minH; y < maxH; y++){
      mesh.addVertex(ofPoint(x*stepSize, baseY, y*stepSize)); // make a new vertex at the base
      mesh.addColor(ofFloatColor(1,.5,0));  // add a color at that vertex
    }
    // bottom
    y = maxH-1;
    for (x = minW; x<maxW; x++){
      mesh.addVertex(ofPoint(x*stepSize, baseY, y*stepSize)); // make a new vertex at the base
      mesh.addColor(ofFloatColor(1,.5,0));  // add a color at that vertex
    }
    // left
    x = minW;
    for (int y = minH; y < maxH; y++){
      mesh.addVertex(ofPoint(x*stepSize, baseY, y*stepSize)); // make a new vertex at the base
      mesh.addColor(ofFloatColor(1,.5,0));  // add a color at that vertex
    }

    if(addBasePlate && functionsAsHoles) {
      // base plate vertexes
      for (int y = minH; y < maxH; y++){
        for (int x = minW; x<maxW; x++){
          float offset = baseY;
          HoleRelation hr = getHole(glm::vec2(x*stepSize, y*stepSize));
          if(hr == HoleRelation::HOLE) {
            offset = holeYPos;
          }
          else if(hr == HoleRelation::WALL) offset = holeYPos;
          mesh.addVertex(ofPoint(x*stepSize,offset,y*stepSize)); // make a new vertex
          mesh.addColor(ofFloatColor(1,0.,.5));  // add a color at that vertex
        }
      }
    }


    int width = maxW - minW;
    int height = maxH - minH;

    // the offset into the base plate vertices
    uint32_t basePlateVerticesOffset = width*height + width*2 + height*2;

    // now it's important to make sure that each vertex is correctly connected with the
    // other vertices around it. This is done using indices, which you can set up like so:
    // indices for the plane
    for (int y = 0; y<height-1; y++){
      for (int x=0; x<width-1; x++){
        // only add the triangles if we are not inside a hole
        glm::vec3 p0 = mesh.getVertex(x+y*width);
        glm::vec3 p1 = mesh.getVertex((x+1)+y*width);
        glm::vec3 p10 = mesh.getVertex(x+(y+1)*width);
        glm::vec3 p11 = mesh.getVertex((x+1)+(y+1)*width);
        bool isNotAHole = p0.y != holeYPos && p1.y != holeYPos && p10.y != holeYPos && p11.y != holeYPos;
        float groundLimit = (1.0 / (glm::distance(p0, glm::vec3(0, 0, 0))/(size*0.04)) * -500) + 40;
        groundLimit = -500;

        bool isNotFlatGround = p0.y > groundLimit || p1.y > groundLimit || p10.y > groundLimit || p11.y > groundLimit;
        if(isNotAHole && isNotFlatGround) {
          mesh.addIndex(x+y*width);               // 0
          mesh.addIndex((x+1)+y*width);           // 1
          mesh.addIndex(x+(y+1)*width);           // 10

          mesh.addIndex((x+1)+y*width);           // 1
          mesh.addIndex((x+1)+(y+1)*width);       // 11
          mesh.addIndex(x+(y+1)*width);           // 10
        }
      }
    }
    int planeVertexLength = width*height; // the offset for the walls, gets incremented after each wall
    // add wall indices
    // top
    y = 0;
    for (int i = 0; i < width-1; i++){
      // only add the triangles if we are not inside a hole
      glm::vec3 p0 = mesh.getVertex(i);
      glm::vec3 p1 = mesh.getVertex(i+1);
      glm::vec3 p10 = mesh.getVertex(planeVertexLength + i);
      glm::vec3 p11 = mesh.getVertex(planeVertexLength + i + 1);
      bool isNotAHole = p0.y != holeYPos && p1.y != holeYPos && p10.y != holeYPos && p11.y != holeYPos;
      bool isNotFlatGround = p0.y > 0 || p1.y > 0 || p10.y > 0 || p11.y > 0;
      if(isNotAHole) {
        // also don't add 
        mesh.addIndex(i);               // 0
        mesh.addIndex(i+1);           // 1
        mesh.addIndex(planeVertexLength + i);           // 10

        mesh.addIndex(i+1);           // 1
        mesh.addIndex(planeVertexLength + i + 1);       // 11
        mesh.addIndex(planeVertexLength + i);           // 10
      }
    }
    planeVertexLength += width;
    // right
    x = width-1;
    for (int i = 0; i < height-1; i++){
      // only add the triangles if we are not inside a hole
      glm::vec3 p0 = mesh.getVertex(x+i*width);
      glm::vec3 p1 = mesh.getVertex(x + (i+1)*width);
      glm::vec3 p10 = mesh.getVertex(planeVertexLength + i);
      glm::vec3 p11 = mesh.getVertex(planeVertexLength + i + 1);
      bool isNotAHole = p0.y != holeYPos && p1.y != holeYPos && p10.y != holeYPos && p11.y != holeYPos;
      bool isNotFlatGround = p0.y > 0 || p1.y > 0 || p10.y > 0 || p11.y > 0;
      if(isNotAHole) {
        mesh.addIndex(x+i*width);               // 0
        mesh.addIndex(x + (i+1)*width);           // 1
        mesh.addIndex(planeVertexLength + i);           // 10

        mesh.addIndex(x + (i+1)*width);           // 1
        mesh.addIndex(planeVertexLength + i + 1);       // 11
        mesh.addIndex(planeVertexLength + i);           // 10
      }
    }
    planeVertexLength += height;
    // bottom
    y = height-1;
    for (int i = 0; i < width-1; i++){
      // only add the triangles if we are not inside a hole
      glm::vec3 p0 = mesh.getVertex(i+y*width);
      glm::vec3 p1 = mesh.getVertex((i+1) + y*width);
      glm::vec3 p10 = mesh.getVertex(planeVertexLength + i);
      glm::vec3 p11 = mesh.getVertex(planeVertexLength + i + 1);
      bool isNotAHole = p0.y != holeYPos && p1.y != holeYPos && p10.y != holeYPos && p11.y != holeYPos;
      bool isNotFlatGround = p0.y > 0 || p1.y > 0 || p10.y > 0 || p11.y > 0;
      if(isNotAHole) {
        mesh.addIndex(i+y*width);               // 0
        mesh.addIndex((i+1) + y*width);           // 1
        mesh.addIndex(planeVertexLength + i);           // 10

        mesh.addIndex((i+1) + y*width);           // 1
        mesh.addIndex(planeVertexLength + i + 1);       // 11
        mesh.addIndex(planeVertexLength + i);           // 10
      }
    }
    planeVertexLength += width;
    // left
    x = 0;
    for (int i = 0; i < height-1; i++){
      // only add the triangles if we are not inside a hole
      glm::vec3 p0 = mesh.getVertex(x+i*width);
      glm::vec3 p1 = mesh.getVertex(x + (i+1)*width);
      glm::vec3 p10 = mesh.getVertex(planeVertexLength + i);
      glm::vec3 p11 = mesh.getVertex(planeVertexLength + i + 1);
      bool isNotAHole = p0.y != holeYPos && p1.y != holeYPos && p10.y != holeYPos && p11.y != holeYPos;
      bool isNotFlatGround = p0.y > 0 || p1.y > 0 || p10.y > 0 || p11.y > 0;
      if(isNotAHole) {
        mesh.addIndex(x+i*width);               // 0
        mesh.addIndex(x + (i+1)*width);           // 1
        mesh.addIndex(planeVertexLength + i);           // 10

        mesh.addIndex(x + (i+1)*width);           // 1
        mesh.addIndex(planeVertexLength + i + 1);       // 11
        mesh.addIndex(planeVertexLength + i);           // 10
      }
    }

    // add bottom plate
    if(addBasePlate) {
      if(!functionsAsHoles) {
        // simple base plate 
        int topleft = width*height;
        int topright = width*height + width;
        int bottomleft = width*height + width + height;
        int bottomright = width*height + width + height + width-1; // last element of bottom row
        mesh.addIndex(topleft);
        mesh.addIndex(topright);
        mesh.addIndex(bottomleft);

        mesh.addIndex(topright);
        mesh.addIndex(bottomright);
        mesh.addIndex(bottomleft);
      } else {
        // fine grained function holes in the bottom plate
        // this will increase the complexity and file size by a lot!
        for (int y = 0; y<height-1; y++){
          for (int x=0; x<width-1; x++){
            // only add the triangles if we are not inside a hole
            glm::vec3 p0 = mesh.getVertex(x+y*width + basePlateVerticesOffset);
            glm::vec3 p1 = mesh.getVertex((x+1)+y*width + basePlateVerticesOffset);
            glm::vec3 p10 = mesh.getVertex(x+(y+1)*width + basePlateVerticesOffset);
            glm::vec3 p11 = mesh.getVertex((x+1)+(y+1)*width + basePlateVerticesOffset);
            bool isNotAHole = p0.y != holeYPos && p1.y != holeYPos && p10.y != holeYPos && p11.y != holeYPos;
            if(isNotAHole) {
              mesh.addIndex(x+(y+1)*width + basePlateVerticesOffset);           // 10
              mesh.addIndex((x+1)+y*width + basePlateVerticesOffset);           // 1
              mesh.addIndex(x+y*width + basePlateVerticesOffset);               // 0

              mesh.addIndex((x+1)+(y+1)*width + basePlateVerticesOffset);       // 11
              mesh.addIndex(x+(y+1)*width + basePlateVerticesOffset);           // 10
              mesh.addIndex((x+1)+y*width + basePlateVerticesOffset);           // 1
            }
          }
        }
      }
    }

    return mesh;
  }

  vector<ofMesh> generateMeshGrid(int gridSize = 10, int stepSize = 5) {
    int width = size/stepSize, height = size/stepSize;
    int minHeight = -height/2;
    int minWidth = -width/2;
    int partWidth = width/gridSize;
    int partHeight = height/gridSize;
    vector<ofMesh> meshes;

    for(int y = 0; y < gridSize; y++) {
      for(int x = 0; x < gridSize; x++) {
        meshes.push_back(generateMesh(
          minWidth + partWidth*x,
          minWidth + partWidth*(x+1),
          minHeight + partHeight*y,
          minHeight + partHeight*(y+1),
          stepSize
        ));
      }
    }
    return meshes;
  }

  ofMesh generateMeshGridPiece(int gridSize = 10, int pieceX = 0, int pieceY = 0, int stepSize = 5) {
    int width = size/stepSize, height = size/stepSize;
    int minHeight = -height/2;
    int minWidth = -width/2;
    int partWidth = width/gridSize;
    int partHeight = height/gridSize;

    return generateMesh(
      minWidth + partWidth*pieceX,
      minWidth + partWidth*(pieceX+1),
      minHeight + partHeight*pieceY,
      minHeight + partHeight*(pieceY+1),
      stepSize
    );
  }
  
  void addScriptPoint(Script& s) {
    // SPIRAL:
    // float scale = 0.7;
    // float offset = (1.-pow(1.-s.getSize(), 2.0)) * size * scale * 0.15;
    // glm::vec2 pos = s.getSpiralCoordinate(maxScriptId, size*scale); // + glm::vec2(size*0.5, size*.5);
    // float radius = (1.-pow(1.-s.getSize(), 2.0)) * size * scale * .5;
    // points.push_back(AttractionPoint(
    //   pos,
    //   offset,
    //   radius
    // ));
    // s.meshRadius = radius * 0.25;
    // s.meshPos = pos;
    // TRIANGLE:
    float offset = (1.-pow(1.-s.scriptCircle.r, .5)) * size * scale * 3.0;
    offset *= 0.2; // shallower mountains for shorter print time
    if(s.scriptType == "built-in") {
      offset *= -.5;
    }
    glm::vec2 pos = (s.scriptCircle.p + pointOffset) * size*scale;
    float radius = (1.-pow(1.-s.scriptCircle.r, 2.0)) * size * scale * 3.0;
    points.push_back(AttractionPoint(
      pos,
      offset,
      radius
    ));
    s.meshRadius = radius * 0.25;
    s.meshPos = pos;
  }
  
  void addFunctionPoint(Function& f, Script& s, float offsetRatio = -0.005) {
    

    // float offset = size * -0.04; // negative bump
    float offset = size * offsetRatio; // bump
    // float radius = size * .003;
    // float radius = size * .005;
    float radius = f.functionCircle.r * size;
    // recalculate the pos relative to the mesh size instead of reusing the one meant for visual display
    glm::vec2 scriptPos = s.meshPos;
    float scriptSize = s.meshRadius;
    // glm::vec2 funcRelativePos = f.getRelativeSpiralPos();
    // glm::vec2 pos = scriptPos + (funcRelativePos * scriptSize);

    // TRIANGLE pos:
    glm::vec2 pos = (f.functionCircle.p + pointOffset) * size*scale;

    
    if(functionsAsHoles) {
      // add as a hole point
      holePoints.push_back(HolePoint(
        pos, 
        radius*0.35,
        radius*0.32
      ));
    } else {
      // add as an attraction point
      points.push_back(AttractionPoint(
        pos,
        offset,
        radius
      ));
    }
    
  }
  
  float getGravityAtPoint(glm::vec2 p) {
    float gravity = 0;
    for(auto& a : points) {
      gravity += a.getGravity(p);
    }
    return gravity;
  }

  HoleRelation getHole(glm::vec2 p) {
    HoleRelation hr = HoleRelation::NONE;
    for(auto& h : holePoints) {
      HoleRelation temphr = h.insideHole(p);
      if(static_cast<int>(temphr) < static_cast<int>(hr)) hr = temphr;
    }
    return hr;
  }
};