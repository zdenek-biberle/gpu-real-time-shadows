#pragma once

#include "CatmullRom.h"
#include "time_item.h"

///////possible TODO resume/pause..
////saving time in pause mode and subtracting it when resumed..
//
//TODO - loading animation from file and saving it
//TODO - merging non looping animations??
//TODO - investigate not looping in some cases  - copy/move constructor?
class Animation
 {
 public:
	 Animation();
	 ~Animation();

	
	
	//flag to check if this animation will be used to modify position or not..	true by default and
	//causes to getValue function to return offset from the start of motion instead of delta from previous position
	//so shifting vs moving  - shifting is temporary, snaps back after finish, returns vector to be added to position; moving is meant to be permanent, returns delta
	bool shifting = true;	
	void start();		//resets start time too..
	void stop();

	void convertToFrames(unsigned int frame_count_at_end);
	glm::vec3 getValue(float valueToInterpolate);		//some other number than time.. must be of same kind as times
	glm::vec3 getValue(time_item time = time_item());
	glm::vec3 getLastValue();							//euther previous value or diff

	void setLooping(bool looping);	//necessary because it sets attribute in another class
	bool isLooping();	

	float getDuration();
	void addPoint(glm::vec3 point, float time);
	bool isPlaying();

 private:

	CRSpline positions;
	std::vector<float> times;	//time values paired with spline points in positions

	//should be set when starting movement else it contains time at object creation
	//set by start()
	time_item start_time;
	int current_index = 0;		// index into movement
	vec3 previous_value = vec3(0.0, 0.0, 0.0);				//for return of incremental values
	vec3 previous_diff = vec3(0.0, 0.0, 0.0);				//for return of previous value
	bool playing = false;

};

