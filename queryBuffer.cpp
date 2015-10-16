#include "queryBuffer.h"

#include <iostream>

//scope checks?
///different initialization options for different types of queries?
//have initialization method before first usage/swapping like initToZero which does suitable empty operation for that glquery

//something else..
//make retrieve result method.. loads result into memory.. and another method uses that value..     ?
///////remove array of queries.. one query per object.. keep it simple.. there is no reason to not create vector of buffered queries..


/*
Constructs front and back buffers for number of glqueries and generates them.
Incomplete implementation of  GL_ANY_SAMPLES_PASSED_CONSERVATIVE​​, GL_PRIMITIVES_GENERATED​ and
GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN type queries in generate_queries. No support checking and have no current use for them.

Query vs glquery - query buffer can have multiple queries (as in timestamp at index 0 and samples passed at index 1),
each of which consists of 2 glqueries in front and back buffer which are swapped after getting result of query.
*/
bufferedQuery::bufferedQuery(GLuint query_type) {

	
	backBuffer = 0, frontBuffer = 1;
		
	generateQueries(query_type);
};


/*
Fills back and fron buffer arrays with generated glqueries.
+ puts dummy timestamp glqueries into front bufers to prevent error when running for the first time.
*/
void bufferedQuery::generateQueries(GLuint query_type) {
 	

	
	glGenQueries(1, &queryID[frontBuffer]);
	glGenQueries(1, &queryID[backBuffer]);


    //populate front buffer to have some sort of result to read..
	//probably not strictly necessary to do it for both buffers, but lets be defensive..
	//glQueryCounter — record the GL time into a query object after all previous commands have reached the GL server but have not yet necessarily executed.
	
	switch(query_type){

	case GL_SAMPLES_PASSED:
	case GL_ANY_SAMPLES_PASSED:
//	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE​​:
//	case GL_PRIMITIVES_GENERATED​:
//	case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
	case GL_TIME_ELAPSED:
		glBeginQuery(query_type, queryID[frontBuffer]);
		glEndQuery(query_type);

		glBeginQuery(query_type, queryID[backBuffer]);
		glEndQuery(query_type);
		break;

	case GL_TIMESTAMP:
		glQueryCounter(queryID[frontBuffer], query_type);   //OMG it has arguments in opposite order...
		glQueryCounter(queryID[backBuffer], query_type);
		break;

	default:
		break;
	}


	


}

/*
Calls glDeleteQueries over buffer arrays.
*/
bufferedQuery::~bufferedQuery(void)
{
		glDeleteQueries(1, &queryID[0]);
		glDeleteQueries(1, &queryID[1]);
	
}


/*
Returns queryID of free glquery in back buffer.
Back buffer always has glquery for next use.
*/
unsigned int bufferedQuery::query(){
	return queryID[backBuffer];
}

/*
Returns queryID of finished glquery in front buffer.
Front buffer always has glquery with result.
*/
unsigned int bufferedQuery::result_of_query(){
	return queryID[frontBuffer];
}

/*
Gets result of glquery and swaps buffers.
Important to call only once per query.
*/
void bufferedQuery::getResult(GLuint64 &result){
	
	GLuint64 done = GL_FALSE;
	
	// wait until the query results are available
	while (done == GL_FALSE) {
		glGetQueryObjectui64v(result_of_query(), GL_QUERY_RESULT_AVAILABLE, &done);
	}
		
	glGetQueryObjectui64v(result_of_query(), GL_QUERY_RESULT, &result);

	swapBuffers();
}

 
/*
Swaps front and back buffer of query at index query_number.
*/
void bufferedQuery::swapBuffers() {
 
	std::swap(backBuffer, frontBuffer);
  
}





///////////////////////////////////////////////////////////////////////////////
		
Query::Query()
{
	glGenQueries(1, &id);
}

Query::~Query(void)
{
	
	glDeleteQueries(1, &id);
	
}

/*
polls result availability and then retrieves result.
*/
void Query::getResult(GLuint64 &result){
	
	GLuint64 done = GL_FALSE;

	// wait until the query results are available
	while (done == GL_FALSE) {
		glGetQueryObjectui64v(id, GL_QUERY_RESULT_AVAILABLE, &done);
	}
	
glGetQueryObjectui64v(id, GL_QUERY_RESULT, &result);

}