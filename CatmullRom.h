#pragma once

#include <vector>
#include<glm/glm.hpp>
#include <iostream>
//source adapted from http://www.codeproject.com/Articles/30838/Overhauser-Catmull-Rom-Splines-for-Camera-Animatio - CPOL

using namespace glm;

class CRSpline
{
public:

	CRSpline();
    CRSpline(const CRSpline&);
    ~CRSpline();


    void addSplinePoint(const vec3 &point);
	vec3 getInterpolatedSplinePoint(float time);   // t = 0...1; 0=vp[0] ... 1=vp[max]
	int getNumPoints();
	vec3 getNthPoint(unsigned int n);
	vec3 getLastPoint();
	float delta();
	bool looping = false;	//when looping, loopingBoounds is used

    // Static method for computing the Catmull-Rom parametric equation
    // given a time (t) and a vector quadruple (p1,p2,p3,p4).
    static vec3 equation(float t, const vec3 &p1, const vec3 &p2, const vec3 &p3, const vec3 &p4);

private:
	int bounds(int index);
	int loopingBounds(int index);
    std::vector<vec3> waypoints;
    float delta_t = 0.0f;
};

