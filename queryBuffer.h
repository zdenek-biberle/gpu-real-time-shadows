#pragma once

#include <GL/glew.h>
#include <vector>
#include <iostream>
// two query buffers: front and back
#define QUERY_BUFFERS 2

//has two buffers for number of queries. 
//After retrieving results for these queries the buffers are swapped.
class bufferedQuery
{
public:

	bufferedQuery(GLuint query_type);
	~bufferedQuery(void);

	//returns queryID for issuing queries.
	unsigned int query();

	//returns result of query into result and swaps buffers.
	void getResult(GLuint64 &result);

private:

	// the array to store the sets of queries.
	unsigned int queryID[QUERY_BUFFERS];
	unsigned int backBuffer;
	unsigned int frontBuffer;


	void swapBuffers();
	void generateQueries(GLuint query_type);

	unsigned int result_of_query();


};
/*
class for singluar query without buffer. Waiting is expected.
*/
class Query {

public:
	Query();
	~Query();

	GLuint id;

	void getResult(GLuint64 &result);
};
