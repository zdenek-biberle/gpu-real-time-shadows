#ifndef CPUIMPL_H
#define CPUIMPL_H

#include <glm/glm.hpp>
#include "vertexTypes.h"
#include "edgeLookup.h"
#include "shadowComputationTypes.h"

ShadowVolumeComputationInfo compute(
	unsigned indexOffset,
	unsigned indexCount,
	const glm::vec3& lightPos,
	float extrusionDistance,
	const std::vector<SimpleVertex>& inVertices, 
	const std::vector<GLuint>& inIndices, 
	const std::vector<EdgeLookupNode>& edgeLookup,
	std::vector<ShadowVolumeVertex>& outVertices);

#endif
