#include "Animation.h"


Animation::Animation(void)
{
	start_time = time_item();


}

void Animation::start(){
	if(! playing){
		playing = true;	
		start_time = time_item();
		current_index = 0;

		if (! positions.looping)
			previous_value = vec3(0.0, 0.0, 0.0);
	}
}

void Animation::stop(){

	playing = false;
}

float Animation::getDuration(){

	return times.back();
}

Animation::~Animation(void)
{
}

void Animation::addPoint(glm::vec3 point, float time){

	positions.addSplinePoint(point);
	times.push_back(time);

}

bool Animation::isPlaying(){

return playing;
}

void Animation::setLooping(bool looping){

	positions.looping = looping;
	
}	

bool Animation::isLooping(){

	return positions.looping;

}

//gets time from start on its own..
//if shifting is set to true, returns vector from the start of motion
//if it is not set the difference from previous interpolated value is returned instead
glm::vec3 Animation::getValue(time_item time){
	if (! playing)
		return glm::vec3();

	float valueToInterpolate = time.seconds(start_time);

	//if next index is not out of bounds, adjust current index
	while(current_index + 1 != times.size() && valueToInterpolate > times[current_index + 1])	//could have skipped one or more nodes..
		current_index++;


	if(current_index + 1 != times.size()){
	
		float segment_duration =  times[current_index + 1] - times[current_index];
		float percentage_of_current_segment_travelled = (valueToInterpolate - times[current_index]) / segment_duration; ///0.5 = 45 / 90     - means im at half of current segment - use it to compute normalized distance for interpolation


		float delta = positions.delta();   //is number between 0.0 and 1.0

		float normalized_distance = delta * current_index + delta * percentage_of_current_segment_travelled;
		
		if (shifting)
			return positions.getInterpolatedSplinePoint(normalized_distance);
		else {
			//delta vector of animation
			vec3 value = positions.getInterpolatedSplinePoint(normalized_distance);
			vec3 diff = value - previous_value;
			previous_value = value;
			previous_diff = diff;
			return diff;
		}

	}
	else {
		playing = false;

		if (positions.looping){
			this->start();

			return this->getValue(time);
		}
		else {
			
			vec3 value = positions.getLastPoint();
			vec3 diff = value - previous_value;
			previous_value = value;
			previous_diff = diff;
			return diff;  //if animation ends here make sure to return diff to the last value
		}
	}

return glm::vec3();
}

//with ome posibly non time value
glm::vec3 Animation::getValue(float valueToInterpolate){

	if (!playing)
		return glm::vec3();

	//if next index is not out of bounds, adjust current index
	while(current_index + 1 != times.size() && valueToInterpolate > times[current_index + 1])	//could have skipped one or more nodes..
		current_index++;


	if(current_index + 1 != times.size()){
	
		float segment_duration =  times[current_index + 1] - times[current_index];
		float percentage_of_current_segment_travelled = (valueToInterpolate - times[current_index]) / segment_duration; ///0.5 = 45 / 90     - means im at half of current segment - use it to compute normalized distance for interpolation


		float delta = positions.delta();   //is number between 0.0 and 1.0

		float normalized_distance = delta * current_index + delta * percentage_of_current_segment_travelled;
		
		if (shifting)
			return positions.getInterpolatedSplinePoint(normalized_distance);
		else {
			//delta vector of animation
			vec3 value = positions.getInterpolatedSplinePoint(normalized_distance);
			vec3 diff = value - previous_value;
			previous_value = value;
			previous_diff = diff;
			return diff;
		}
	
	} else {
		playing = false;

		if (positions.looping){
			this->start();

			return this->getValue(valueToInterpolate);
		}
		else { 
			vec3 value = positions.getLastPoint();
			vec3 diff = value - previous_value;
			previous_value = value;
			previous_diff = diff;
			return diff;	
		}
	}

return glm::vec3();
}

glm::vec3 Animation::getLastValue(){

	if (shifting)
		return previous_value;
	else
		return previous_diff;
}


/*
Converts loaded times in movement.times to frame numbers
To enable deterministic position at set frame..
*/
void Animation::convertToFrames(unsigned int frame_count_at_end){

	if(times.empty())
		return;

	float end_time = times.back();

	for(unsigned int i = 0; i < times.size(); i++){
	
		times[i] = times[i] / end_time * frame_count_at_end;
	
	}
	
}