#pragma once

#include "ofMain.h"

class Circle {
public:
    glm::vec2 p;
    float r = 1;
    ofColor col;
    Circle() {}
    Circle(glm::vec2 p_, float r_): p(p_), r(r_) {}
    void draw() {
        ofSetColor(col);
        ofDrawCircle(p.x, p.y, r);
    }
    void draw(float scale) {
        ofSetColor(col);
        ofDrawCircle(p.x * scale, p.y * scale, r*scale);
    }
    bool circleOverlaps(Circle& c) {
        return glm::distance(p, c.p) < (r + c.r);
    }
    bool isCircleInside(Circle& c) {
        return glm::distance(p, c.p) < (r - c.r);
    }
};

class Triangle {
public:
    glm::vec2 p1, p2, p3;
    Triangle() {}
    Triangle(glm::vec2 p1_, glm::vec2 p2_, glm::vec2 p3_): p1(p1_), p2(p2_), p3(p3_) {}
    // https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
    float sign (glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
    {
        // signed area of the triangle formed by 3 points
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    }
    float pointToLineDistance(glm::vec2 p, glm::vec2 l1, glm::vec2 l2) {
        // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
        return abs((l2.y-l1.y)*p.x - (l2.x-l1.x)*p.y + l2.x*l1.y - l2.y*l1.x) / sqrt(pow(l2.y-l1.y, 2) + pow(l2.x - l1.x, 2));
    }
    bool isCircleInside (glm::vec2 pt, float r)
    {
        // distance between center of circle to each line in the triangle
        // if the distance if less than the radius of the circle and the
        // circle center is inside the triangle we are good
        if(
            // test if either of the sides are closer to the point than r
            pointToLineDistance(pt, p1, p2) < r ||
            pointToLineDistance(pt, p2, p3) < r ||
            pointToLineDistance(pt, p1, p3) < r
        ) {
            return false;
        }

        float d1, d2, d3;
        bool has_neg, has_pos;

        // get the signed area of every edge and the point tested
        d1 = sign(pt, p1, p2);
        d2 = sign(pt, p2, p3);
        d3 = sign(pt, p3, p1);

        // if all the areas are either negative or positive the point is in the triangle
        has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

        return !(has_neg && has_pos);
    }
    glm::vec2 getCenter() {
        return glm::vec2(
                   (p1.x+p2.x+p3.x)/3.0,
                   (p1.y+p2.y+p3.y)/3.0
               );
    }
    void draw() {
        ofDrawTriangle(p1, p2, p3);
    }
    void draw(float scale) {
        ofDrawTriangle(p1*scale, p2*scale, p3*scale);
    }
};