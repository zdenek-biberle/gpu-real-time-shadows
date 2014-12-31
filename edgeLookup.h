#ifndef EDGE_LOOKUP_H
#define EDGE_LOOKUP_H

#include "modelLoader.h"

// idx0 a idx1 definuji indexy vertexu hrany. Plati, ze idx0 >= idx1
// idx2 je index tretiho vertexu trojuhelniku, ktery neni na hrane
// triangleIdx je index trojuhelniku
struct EdgeLookupNode
{
	EdgeLookupNode(GLuint idx0, GLuint idx1, GLuint idx2, GLuint triangleIdx);
	
	GLuint idx0;
	GLuint idx1;
	GLuint idx2;
	GLuint triangleIdx;
};

void generateEdgeLookup(
	const ModelInfo& modelInfo,
	const std::vector<GLuint>& indices,
	std::vector<EdgeLookupNode>& edgeLookup);

#endif
