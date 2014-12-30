#ifndef SIMPLIFY_MODEL_H
#define SIMPLIFY_MODEL_H

#include <vector>
#include "modelLoader.h"

// zjednodussi model pro ucely generovani shadow volume
ModelInfo simplifyModel(
	ModelInfo inModel,
	const std::vector<Vertex>& inVertices, 
	const std::vector<GLuint>& inIndices,
	std::vector<SimpleVertex>& outVertices,
	std::vector<GLuint>& outIndices);

#endif
