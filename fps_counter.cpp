#include "fps_counter.h"

/*
reevaluates current_time to last_update_time difference and computes avg_render_time and fps accordingly.
INCREMENTS FRAME COUNTER - call it only once per frame
*/
void fps_counter::update(){

	frame++;
	frames_count++;

	GLuint elapsed_time = current_time - last_frame_time;
	put_frame_into_time_slot(elapsed_time);

	//if it's time to update values
	if (current_time - last_update_time > update_interval) { 

		fps = frame / ((float)(current_time - last_update_time) / 1000000000.0f);   //extrapolate to whole second  //frame count / elapsed time converted from nanoseconds to seconds 
		render_time_avg = 1.0f / fps * 1000;     //now this I want in ms
		last_update_time = current_time;
		frame = 0;



		if(render_time_avg < 1000){  //prevent distortion of values at start.. 33ms =~ 30fps; 66 cca 15fps; 100ms - 10fps
			update_count++;

				avg_fps = avg_fps + fps;
				avg_render_time = avg_render_time + render_time_avg;
			
			//update text only if it is being currently displayed
			if(control->stats){
				std::string msg = "FPS:        "+std::to_string(fps)+
				"\nAvg FPS: "+std::to_string(avg_fps/update_count)+
				"\nRender time:       "+std::to_string(render_time_avg)+"ms per frame"+
				"\nAvg render time: "+std::to_string(avg_render_time/update_count)+"ms\n"+
				"Frames rendered: "+std::to_string(frames_count);
				
				stats->addTextData(msg, control->windowHeight, 10, 20, text_size);
				stats->uploadData();
			}
		}
	}

	last_frame_time = current_time;  //save this time for the next call of this function

}

/* time slots - can be interpreted as fps targets..
    lower  - greater than 100ms
	10fps  - smaller than 100ms
	15fps  - smaller than 66.6ms
	20fps  - smaller than 50ms
	30fps  - smaller than 33.3ms
	60fps  - smaller than 16.7ms
	120fps - smaller than 8.3ms
	240fps - smaller than 4.16ms
	*/
void fps_counter::put_frame_into_time_slot(GLuint time){

	float time_ms = time / 1000000.0f;

	if (time_ms < 4.16f){
		frame_time_slots[0].value++;

	} else if(time_ms < 8.3f){
		frame_time_slots[1].value++;
	
	} else if (time_ms < 16.7f) {
		frame_time_slots[2].value++;
	
	} else if (time_ms < 33.3f) {
		frame_time_slots[3].value++;

	} else if (time_ms < 50.0f) {
		frame_time_slots[4].value++;

	} else if (time_ms < 66.6f) {
		frame_time_slots[5].value++;

	} else if (time_ms < 100.0f) {
		frame_time_slots[6].value++;

	} else if (time_ms > 100.0f) {
		frame_time_slots[7].value++;
	}

}

void fps_counter::setTextSize(unsigned int new_size) {

	text_size = new_size;

}

/*
again in nanoseconds
*/
void fps_counter::setUpdateInterval(unsigned int ns){

	update_interval = ns;
}

GLuint fps_counter::getFrameCount(){
	return frames_count;
}

/*
Returns added render time divided by update count.
*/
float fps_counter::getAvgRenderTime(){

return avg_render_time / update_count;
}

/*
Returns added fps divided by update count.
*/
float fps_counter::getAvgFps(){

return avg_fps / update_count;
}