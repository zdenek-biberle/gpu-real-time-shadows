#pragma once


#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <iostream>

/*
Small class which allows for sampling current time with comment and returning difference from another time point in seconds.
*/
class time_item {

public:

	time_item(){ 
		time = clock(); 
		comment = ""; 
	}

	time_item(clock_t t, std::string comm) : time(t), comment(comm){}  //clock_t to be supplied by clock()

	clock_t time;
	std::string comment;

	//returns difference from  this - reference  as seconds
	//precision looks like is in ms
	float seconds(time_item &reference){
		return (float) (this->time - reference.time) / CLOCKS_PER_SEC;
		//return difftime(time, reference.time);
	}

};


//call only clock() internally?

