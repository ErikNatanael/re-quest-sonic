#pragma once
#include "ofMain.h"
#include "Script.h"
#include "Function.h"
#include <unordered_set>
#include "ofxCv.h"

using namespace cv;
using namespace ofxCv;

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

  vector<glm::vec2> getPoints() {
    vector<glm::vec2> points;
    points.push_back(pos);
    int numCircles = 10;
    for(int i = 1; i <= numCircles; i++) {
      int numPointsPerCircle = max(4.0, (radius/numCircles) * 0.1 * i);
      for(int k = 0; k < numPointsPerCircle; k++) {
        float angle = (TWO_PI/numPointsPerCircle) * k;
        float r = (radius/numCircles) * i;
        glm::vec2 tp = pos + glm::vec2(cos(angle)*r, sin(angle)*r);
        // round to nearest integer because OpenCV Points are using integers
        tp = glm::round(tp);
        points.push_back(tp);
      }
    }
    return points;
  }
  // these functions assume a value 0-1
  float smoothcurve1(float x) {
    return x * x * (3 - 2 * x);
  }
  float smoothcurve2(float x) {
    return x * x * x * (x * (x * 6 - 15) + 10);
  }
};

enum class HoleRelation {HOLE = 0, CLEARANCE = 1, NONE = 2};
enum class Plane {TOP, BOTTOM, NONE};

class IndexedPoint {
  // a way to keep track of the index in the mesh of a certain vertex
public:
  glm::vec2 pos;
  float angle = 0; // use polar coordinates to sort the points in circular order
  size_t index;

  IndexedPoint(glm::vec2 p, size_t i): pos(p), index(i) {
    if(index < 0) {
      ofLogError("IndexedPoint()") << "index out of bounds " << index;
    }
  }
  void calculateAngle(glm::vec2 offset) {
    angle = atan2(pos.y-offset.y, pos.x-offset.x);
  }
  bool operator<(const IndexedPoint& s) {
    return this->angle < s.angle;
  }
  bool operator==(const IndexedPoint& ip) {
    return this->pos == ip.pos;
  }
};

class HolePoint {
public:
  glm::vec2 pos;
  // if points are added too close to a hole the Delaunay triangulation can create triangles
  // covering the hole and creating non-manifold edges. Therefore a clearence radius is needed.
  float clearRadius;
  float holeRadius;
  vector<IndexedPoint> topPlaneHoleIndices;
  vector<IndexedPoint> bottomPlaneHoleIndices;
  vector<glm::vec2> cirPoints; // contains cylinder points and wall points if there are any
  vector<glm::vec2> wallPoints; // only contains the wall points

  HolePoint(glm::vec2 p, float cr, float hr): pos(p), clearRadius(cr), holeRadius(hr) {}

  HoleRelation insideHole(glm::vec2 p) {
    float distance = glm::distance(pos, p);
    if(distance < holeRadius) return HoleRelation::HOLE;
    else if (distance < clearRadius) return HoleRelation::CLEARANCE;
    return HoleRelation::NONE;
  }

  void generateCircumferencePoints() {
    cirPoints.clear();
    int numPoints = 6;
    for(int i = 0; i < numPoints; i++) {
      float angle = (TWO_PI/numPoints) * i;
      glm::vec2 tp = glm::vec2(pos.x + cos(angle) * holeRadius, pos.y + sin(angle) * holeRadius);
      // round to nearest integer because OpenCV Points are using integers
      tp = glm::round(tp);
      cirPoints.push_back(tp);
    }
  }

  vector<glm::vec2> getHoleCircumferencePoints() {
    return cirPoints;
  }

  vector<glm::vec2> getHoleWallIntersectionPoints(int minW, int maxW, int minH, int maxH) {
    vector<glm::vec2> holeWallPoints;
    // corner holes intersect with multiple walls so don't do `else if`
    if(abs(pos.x - minW) < holeRadius && pos.y + holeRadius > minH && pos.y - holeRadius < maxH) {
      // the hole intersects with the right wall
      // calculate points on the wall
      float xdiff = minW - pos.x;
      // use Pythagorean theorem to find the other point on the hole radius
      float ydiff = sqrt(pow(holeRadius, 2) - pow(xdiff, 2));
      // add points +- ydiff
      holeWallPoints.push_back(glm::round(pos + glm::vec2(xdiff, ydiff)));
      holeWallPoints.push_back(glm::round(pos + glm::vec2(xdiff, -ydiff)));
    } 
    if(abs(pos.x - maxW) < holeRadius && pos.y + holeRadius > minH && pos.y - holeRadius < maxH) {
      // the hole intersects with the left wall
      // calculate points on the wall
      float xdiff = maxW-1 - pos.x;
      // use Pythagorean theorem to find the other point on the hole radius
      float ydiff = sqrt(pow(holeRadius, 2) - pow(xdiff, 2));
      // add points +- ydiff
      holeWallPoints.push_back(glm::round(pos + glm::vec2(xdiff, ydiff)));
      holeWallPoints.push_back(glm::round(pos + glm::vec2(xdiff, -ydiff)));
    } 
    if(abs(pos.y - minH) < holeRadius && pos.x + holeRadius > minW && pos.x - holeRadius < maxW) {
      // the hole intersects with the top wall
      // calculate points on the wall
      float ydiff = minH - pos.y;
      // use Pythagorean theorem to find the other point on the hole radius
      float xdiff = sqrt(pow(holeRadius, 2) - pow(ydiff, 2));
      // add points +- ydiff
      holeWallPoints.push_back(glm::round(pos + glm::vec2(xdiff, ydiff)));
      holeWallPoints.push_back(glm::round(pos + glm::vec2(-xdiff, ydiff)));
    } 
    if(abs(pos.y - maxH) < holeRadius && pos.x + holeRadius > minW && pos.x - holeRadius < maxW) {
      // the hole intersects with the top wall
      // calculate points on the wall
      float ydiff = maxH-1 - pos.y;
      // use Pythagorean theorem to find the other point on the hole radius
      float xdiff = sqrt(pow(holeRadius, 2) - pow(ydiff, 2));
      // add points +- ydiff
      holeWallPoints.push_back(glm::round(pos + glm::vec2(xdiff, ydiff)));
      holeWallPoints.push_back(glm::round(pos + glm::vec2(-xdiff, ydiff)));
    }
    for(int i = 0; i < holeWallPoints.size(); i++) {
      auto& p = holeWallPoints[i];
      ofLogError("HolePoint::addCylinderToMesh") << "point added: " << p.x << ", " << p.y;
      if(p.x >= minW && p.x < maxW && p.y >= minH && p.y < maxH) {
        wallPoints.push_back(p);
        cirPoints.push_back(p);
      } else {
        ofLogError("HolePoint::addCylinderToMesh") << "point out of bounds: " << p.x << ", " << p.y;
        holeWallPoints.erase(holeWallPoints.begin()+i);
        i--;
      }
    }
    
    return holeWallPoints;
  }

  bool isPointInList(glm::vec2 p) {
    if(find(cirPoints.begin(), cirPoints.end(), p) != cirPoints.end()) {
      return true;
    } else {
      return false; 
    }
  }

  void addIndex(glm::vec2 p, Plane plane, size_t index) {
    IndexedPoint ip = IndexedPoint(p, index);
    if(plane == Plane::TOP) {
      if(find(topPlaneHoleIndices.begin(), topPlaneHoleIndices.end(), ip) == topPlaneHoleIndices.end())
        topPlaneHoleIndices.push_back(ip);
    } else if (plane ==Plane::BOTTOM) {
      if(find(bottomPlaneHoleIndices.begin(), bottomPlaneHoleIndices.end(), ip) == bottomPlaneHoleIndices.end())
        bottomPlaneHoleIndices.push_back(ip);
    }
  }

  void addCylinderToMesh(ofMesh& mesh) {
    if(topPlaneHoleIndices.size() != bottomPlaneHoleIndices.size()) {
      ofLogError("HolePoint::addCylinderToMesh") << "top and bottom hole index vectors have different sizes";
      ofLogError("HolePoint::addCylinderToMesh") << "top: " << topPlaneHoleIndices.size();
      for(auto& ip : topPlaneHoleIndices) {
        ofLogError("HolePoint::addCylinderToMesh") << ip.index << " " << ip.pos.x << ", " << ip.pos.y;
      }
      ofLogError("HolePoint::addCylinderToMesh") << "bottom: " << bottomPlaneHoleIndices.size();
      for(auto& ip : bottomPlaneHoleIndices) {
        ofLogError("HolePoint::addCylinderToMesh") << ip.index << " " << ip.pos.x << ", " << ip.pos.y;
      }
    }
    if(topPlaneHoleIndices.size() < 3) {
      // ofLogError("HolePoint::addCylinderToMesh") << "fewer than 3 indices";
    }
    // sort the points in circular order
    // 1. calculate the angles of the points in polar coordinates relative to the center
    for(auto& i : topPlaneHoleIndices) {
      i.calculateAngle(pos);
    }
    for(auto& i : bottomPlaneHoleIndices) {
      i.calculateAngle(pos);
    }
    // 2. sort the points
    std::sort(topPlaneHoleIndices.begin(), topPlaneHoleIndices.end());
    std::sort(bottomPlaneHoleIndices.begin(), bottomPlaneHoleIndices.end());

    // create the cylinder
    for (int i = 0; i < topPlaneHoleIndices.size(); i++){
      // for the last wall we want to wrap around to the first point
      int nextIndex = (i+1)%(topPlaneHoleIndices.size());
      // filter out pieces belonging to a wall
      bool isWallPiece = false;
      // if both positions exist in the wallPoints vector the segment is a wall segment that should not be
      if(find(wallPoints.begin(), wallPoints.end(), topPlaneHoleIndices[i].pos) != wallPoints.end()
        && find(wallPoints.begin(), wallPoints.end(), topPlaneHoleIndices[nextIndex].pos) != wallPoints.end()) 
      {
        isWallPiece = true;
      }
      if(!isWallPiece) {
        mesh.addIndex(bottomPlaneHoleIndices[i].index);           // 10
        mesh.addIndex(topPlaneHoleIndices[nextIndex].index);           // 1
        mesh.addIndex(topPlaneHoleIndices[i].index);               // 0
        

        mesh.addIndex(bottomPlaneHoleIndices[i].index);           // 10
        mesh.addIndex(bottomPlaneHoleIndices[nextIndex].index);       // 11
        mesh.addIndex(topPlaneHoleIndices[nextIndex].index);           // 1
      }
    }
  }

  void resetForMeshGeneration() {
    // if many meshes are generated from the same GravityPlane we need to reset a few things in between
    topPlaneHoleIndices.clear();
    bottomPlaneHoleIndices.clear();
    wallPoints.clear();
  }
};

class GravityPlane {
public:
  vector<AttractionPoint> points;
  vector<HolePoint> holePoints;
  int maxScriptId;
  int size = 8000;
  float scale = 0.35; // used to scale things down so they don't overshoot the edge
  bool addBasePlate = true;
  bool functionsAsHoles = true;
  int holeYPos = -4000;
  glm::vec2 pointOffset;
  glm::vec2 centerOffset;

  GravityPlane() {
    centerOffset = glm::vec2(size/2, size/2); // don't use negative coordinates in the mesh (for OpenCV triangulation algo)
    pointOffset = glm::vec2(0, -0.3); // compensate for an off center mid point
    points.push_back(AttractionPoint(
      glm::vec2(0, 0) + centerOffset + (glm::vec2(0, -0.1)*size),
      size*0.1,
      size*0.6
    ));
  }

  ofMesh generateMesh() {
    int stepSize = 1;
    int width = size/stepSize, height = size/stepSize;
    // return generateMesh(-width/2, width/2, -height/2, height/2, stepSize);
    return generateMesh(0, width, 0, height, stepSize);
  }
  
  ofMesh generateMesh(int minW, int maxW, int minH, int maxH, int stepSize) {
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);

    vector<glm::vec2> topPoints;
    vector<glm::vec2> bottomPoints;
    // use these to be able to reuse vertices 
    vector<IndexedPoint> topIndexedPoints;
    vector<IndexedPoint> bottomIndexedPoints;
    // for the wall points, keeping them in a separate list to sort them and go through them all
    vector<IndexedPoint> topCornerIndices;
    vector<IndexedPoint> bottomCornerIndices;
    int currentIndex = 0;

    int baseY = -200;

    int width = maxW - minW;
    int height = maxH - minH;

    // reset from last time generateMesh was run
    for(auto& h : holePoints) {
      h.resetForMeshGeneration();
      h.generateCircumferencePoints();
    }

    // triangulation/OpenCV stuff
    vector<Point> pt(3); 
    vector<Vec6f> triangleList;
    Rect rect(minW, minH, width, height);

    ofLogNotice("GravityPlane::generateMesh") << "Mesh generation started";
    // plane vertexes
    // add surrounding plane points
    
    // start with the four corners
    vector<glm::vec2> wallPoints = {glm::vec2(minW, minH), 
      glm::vec2(maxW-1, minH), 
      glm::vec2(maxW-1, maxH-1),
      glm::vec2(minW, maxH-1)};

    int pointsPerWall = 8;
    for(int i = 0; i < pointsPerWall; i++) {
      wallPoints.push_back(glm::vec2(minW + ((width/(pointsPerWall+1))*i+1), minH));
      wallPoints.push_back(glm::vec2(maxW-1, minH + ((height/(pointsPerWall+1))*i+1)));
      wallPoints.push_back(glm::vec2(minW + ((width/(pointsPerWall+1))*i+1), maxH-1));
      wallPoints.push_back(glm::vec2(minW, minH + ((height/(pointsPerWall+1))*i+1)));
    }
    // remove points that fall within a hole
    for(int i = 0; i < wallPoints.size(); i++) {
      for(auto& h : holePoints) {
        if(h.insideHole(wallPoints[i]) == HoleRelation::HOLE) {
          wallPoints.erase(wallPoints.begin()+i);
          i--;
          break;
        }
      }
    }
    // add points from holes intersecting the wall
    for(auto& h : holePoints) {
      vector<glm::vec2> holeWallPoints = h.getHoleWallIntersectionPoints(minW, maxW, minH, maxH);
      for(auto& p : holeWallPoints) {
        wallPoints.push_back(p);
      }
    }

    int numWallPoints = wallPoints.size();

    for(int i = 0; i < numWallPoints; i++) {
      topPoints.push_back(wallPoints[i]);
    }
    
    // add all attraction point points
    for(auto& a : points) {
      vector<glm::vec2> ap = a.getPoints();
      for(auto& p : ap) {
        // points too close to an edge can cause non manifold edges because of the triangulation
        // filter out points that are too close
        float edgeFilter = size*0.01;
        if(p.x > minW + edgeFilter && p.x < maxW - edgeFilter && p.y > minH + edgeFilter && p.y < maxH - edgeFilter) {
          // only add point if it is not inside a hole
        if(getHole(p) == HoleRelation::NONE)
          topPoints.push_back(p);
        }
      }
    }
    // add all hole points
    for(auto& hole : holePoints) {
      vector<glm::vec2> hp = hole.getHoleCircumferencePoints();
      for(auto& p : hp) {
        topPoints.push_back(p);
      }
    }

    // triangulate using Delaunay triangulation from OpenCV
    
    Subdiv2D subdivTop = Subdiv2D(rect);
    for(auto& p : topPoints) {
      if(p.x >= minW && p.x < minW + width && p.y >= minH && p.y < minH + height) {
        subdivTop.insert(toCv(p));
      }
    }
    subdivTop.getTriangleList(triangleList);
    
    for( size_t i = 0; i < triangleList.size(); i++ )
    {
      Vec6f t = triangleList[i];
      pt[0] = Point(t[0], t[1]);
      pt[1] = Point(t[2], t[3]);
      pt[2] = Point(t[4], t[5]);
        
      // add triangles to the mesh
      if ( rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2]))
      {
        int pointIndices[3];
        
        // check if the point is in a hole's point list. if it is, add it as an index
        int numPointsInHole = 0;
        size_t lastHoleIndex = -1;
        bool differentHoles = false; // true if points belong to holes, but different ones
        for(int i = 0; i < 3; i++) {
          auto& p = pt[i];
          int holeMeshIndex = currentIndex;
          // check if the point is already in the topIndexedPoints list of previous points
          IndexedPoint ip = IndexedPoint(toOf(p), currentIndex);
          auto pointIt = std::find(topIndexedPoints.begin(), topIndexedPoints.end(), ip);
          if(pointIt != topIndexedPoints.end()) {
            // point already exists, use it
            pointIndices[i] = pointIt->index;
            holeMeshIndex = pointIt->index;
          } else {
            // point is new, create it
            float offset = getGravityAtPoint(toOf(p));
            mesh.addVertex(ofPoint(p.x, offset, p.y));
            
            // check if the point is a corner, in that case we save the index for the walls
            for(int i = 0; i < numWallPoints; i++) {
              if(toOf(p) == wallPoints[i]) topCornerIndices.push_back(IndexedPoint(toOf(p), currentIndex));
            }
            pointIndices[i] = currentIndex;
            topIndexedPoints.push_back(ip);
            currentIndex++;
          }

          size_t holeIndex = isPointInHoleThenAddIndex(toOf(p), Plane::TOP, holeMeshIndex);
          if(holeIndex != -1) {
            numPointsInHole++;
            if(lastHoleIndex == -1) lastHoleIndex = holeIndex;
            else if(holeIndex != lastHoleIndex) {
              differentHoles = true;
            }
          }
        }
        // don't add the triangle if all points are hole points
        if(numPointsInHole < 3 || differentHoles) {
          mesh.addIndex(pointIndices[2]);
          mesh.addIndex(pointIndices[1]);
          mesh.addIndex(pointIndices[0]);
        }
      }
    }

    ofLogNotice("GravityPlane::generateMesh") << "Plane constructed";

    // bottom vertices
    // add surrounding plane points
    for(int i = 0; i < numWallPoints; i++) {
      bottomPoints.push_back(wallPoints[i]);
    }
    // add all hole points
    for(auto& hole : holePoints) {
      vector<glm::vec2> hp = hole.getHoleCircumferencePoints();
      for(auto& p : hp) {
          bottomPoints.push_back(p); // make a new vertex
      }
    }
    // triangulate using Delaunay triangulation from OpenCV
    Subdiv2D subdivBottom = Subdiv2D(rect);
    for(auto& p : bottomPoints) {
      if(p.x >= minW && p.x < minW + width && p.y >= minH && p.y < minH + height) {
        subdivBottom.insert(toCv(p));
      }
    }
    triangleList.clear();
    subdivBottom.getTriangleList(triangleList);
    for( size_t i = 0; i < triangleList.size(); i++ )
    {
      Vec6f t = triangleList[i];
      pt[0] = Point(t[0], t[1]);
      pt[1] = Point(t[2], t[3]);
      pt[2] = Point(t[4], t[5]);
        
      // add triangles to the mesh
      if ( rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2]))
      {
        int pointIndices[3];
        // check if the point is in a hole's point list. if it is, add it as an index
        int numPointsInHole = 0;
        size_t lastHoleIndex = -1;
        bool differentHoles = false; // true if points belong to holes, but different ones
        for(int i = 0; i < 3; i++) {
          auto& p = pt[i];
          // check if the point is already in the topIndexedPoints list of previous points
          IndexedPoint ip = IndexedPoint(toOf(p), currentIndex);
          auto pointIt = std::find(bottomIndexedPoints.begin(), bottomIndexedPoints.end(), ip);
          int holeMeshIndex = currentIndex;
          if(pointIt != bottomIndexedPoints.end()) {
            // point already exists, use it
            pointIndices[i] = pointIt->index;
            holeMeshIndex = pointIt->index;
          } else {
            // make a new point
            float offset = baseY;
            mesh.addVertex(ofPoint(p.x, offset, p.y));
            pointIndices[i] = currentIndex;
            bottomIndexedPoints.push_back(ip);
            // check if the point is a corner, in that case we save the index for the walls
            for(int i = 0; i < numWallPoints; i++) {
              if(toOf(p) == wallPoints[i]) bottomCornerIndices.push_back(IndexedPoint(toOf(p), currentIndex));
            }
            currentIndex++;
          }
          
          size_t holeIndex = isPointInHoleThenAddIndex(toOf(p), Plane::BOTTOM, holeMeshIndex);
          if(holeIndex != -1) {
            numPointsInHole++;
            if(lastHoleIndex == -1) lastHoleIndex = holeIndex;
            else if(holeIndex != lastHoleIndex) {
              differentHoles = true;
            }
          }
        }
        // don't add the triangle if all points are hole points
        if(numPointsInHole < 3 || differentHoles) {
          mesh.addIndex(pointIndices[0]);
          mesh.addIndex(pointIndices[1]);
          mesh.addIndex(pointIndices[2]);
        }
      }
    }

    ofLogNotice("GravityPlane::generateMesh") << "Bottom constructed";

    // walls
    // sort the corner indexed points in circular order
    // 1. calculate the angles of the points in polar coordinates relative to the center
    glm::vec2 pieceCenterOffset = glm::vec2(minW + (width/2), minH + (height/2));
    for(auto& i : topCornerIndices) {
      i.calculateAngle(pieceCenterOffset);
    }
    for(auto& i : bottomCornerIndices) {
      i.calculateAngle(pieceCenterOffset);
    }
    ofLogNotice("GravityPlane::generateMesh") << "Constructing walls from " << topCornerIndices.size() 
      << " indices, pieceCenterOffset: " << pieceCenterOffset.x << ", " << pieceCenterOffset.y;
    // 2. sort the points
    std::sort(topCornerIndices.begin(), topCornerIndices.end());
    std::sort(bottomCornerIndices.begin(), bottomCornerIndices.end());

    if(topCornerIndices.size() != wallPoints.size() && bottomCornerIndices.size() != wallPoints.size()) {
      ofLogError("GravityPlane::generateMesh") << "Not all corner indices have been set";
    }

    for(int wall = 0; wall < numWallPoints; wall++) {
      int nextIndex = (wall+1)%numWallPoints;
      // filter out wall segments where the middle of the segment falls within a hole
      bool isInsideHole = false;
      glm::vec2 segmentCenter = (topCornerIndices[wall].pos + topCornerIndices[nextIndex].pos)/2;
      for(auto& h : holePoints) {
        if(h.insideHole(segmentCenter) == HoleRelation::HOLE) {
          isInsideHole = true;
          break;
        }
      }
      if(!isInsideHole) {
        mesh.addIndex(topCornerIndices[wall].index);               // 0
        mesh.addIndex(topCornerIndices[nextIndex].index);           // 1
        mesh.addIndex(bottomCornerIndices[wall].index);           // 10

        mesh.addIndex(topCornerIndices[nextIndex].index);           // 1
        mesh.addIndex(bottomCornerIndices[nextIndex].index);       // 11
        mesh.addIndex(bottomCornerIndices[wall].index);         // 10
      }
    }

    ofLogNotice("GravityPlane::generateMesh") << "Walls constructed";

    // add cylinders for all the holes
    for(auto& h : holePoints) {
      h.addCylinderToMesh(mesh);
    }

    ofLogNotice("GravityPlane::generateMesh") << "Hole indices added";

    return mesh;
  }

  vector<ofMesh> generateMeshGrid(int gridSize = 10, int stepSize = 1) {
    int width = size, height = size;
    int minHeight = 0;
    int minWidth = 0;
    int partWidth = width/gridSize;
    int partHeight = height/gridSize;
    vector<ofMesh> meshes;

    for(int y = 0; y < gridSize; y++) {
      for(int x = 0; x < gridSize; x++) {
        ofLogNotice("GravityPlane::generateMeshGrid") << "Generating piece " << x << ", " << y << " with dimensions "
          << minWidth + partWidth*x << ", " << minWidth + partWidth*(x+1)-1 << ", " << minHeight + partHeight*y << ", " << minHeight + partHeight*(y+1)-1;
        meshes.push_back(generateMesh(
          minWidth + partWidth*x,
          minWidth + partWidth*(x+1) - 1,
          minHeight + partHeight*y,
          minHeight + partHeight*(y+1) - 1,
          stepSize
        ));
      }
    }
    return meshes;
  }

  ofMesh generateMeshGridPiece(int gridSize = 10, int pieceX = 0, int pieceY = 0, int stepSize = 1) {
    int width = size/stepSize, height = size/stepSize;
    int minHeight = 0;
    int minWidth = 0;
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
    // TRIANGLE:
    float offset = (1.-pow(1.-s.scriptCircle.r, .5)) * size * scale * 3.0;
    // offset *= 0.2; // shallower mountains for shorter print time
    if(s.scriptType == "built-in") {
      offset *= -1.;
    }
    glm::vec2 pos = (s.scriptCircle.p + pointOffset) * size*scale;
    pos += centerOffset;
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
    pos += centerOffset;

    
    if(functionsAsHoles) {
      // add as a hole point
      float holeRadius = radius*0.32;
      float clearRadius = holeRadius * 1.3;
      holePoints.push_back(HolePoint(
        pos, 
        clearRadius,
        holeRadius
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

  size_t isPointInHoleThenAddIndex(glm::vec2 p, Plane plane, size_t index) {
    int holesMatched = 0;
    int holeIndex = -1;
    for(int i = 0; i < holePoints.size(); i++) {
      auto& h = holePoints[i];
      bool isPointInHoleList = h.isPointInList(p);
      if(isPointInHoleList) {
        holesMatched++;
        // add the index of this vertex to the hole to create a cylinder later
        h.addIndex(p, plane, index);
        holeIndex = i;
      }
    }
    if(holesMatched > 1) {
      ofLogError("GravityPlane::isPointInHoleThenAddIndex") << "Point matched with more than 1 hole. Maybe the holes are too close together?";
    }
    return holeIndex;
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