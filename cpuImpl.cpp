#include "cpuImpl.h"

glm::vec3 position(const SimpleVertex& vert)
{
	return glm::vec3(vert._x, vert._y, vert._z);
}

bool isFrontFacing(const glm::vec3& lightDir, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
	auto ab = b - a;
	auto ac = c - a;
	auto n = glm::cross(ab, ac);
	return glm::dot(glm::normalize(n), glm::normalize(lightDir)) < 0;
}

void emitTriangle(std::vector<ShadowVolumeVertex>& outVertices, const glm::vec3 a, const glm::vec3 b, const glm::vec3 c, int multiplicity, unsigned isCap)
{
	outVertices.push_back(ShadowVolumeVertex(a.x, a.y, a.z, multiplicity, isCap));
	outVertices.push_back(ShadowVolumeVertex(b.x, b.y, b.z, multiplicity, isCap));
	outVertices.push_back(ShadowVolumeVertex(c.x, c.y, c.z, multiplicity, isCap));
}

// zjisti, zda je bod point pred nebo za rovinou definovanou body a, b a c
bool isInFront(const glm::vec3& point, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
	auto ab = b - a;
	auto ac = c - a;

	auto normal = glm::normalize(glm::cross(ab, ac));
	auto pointvec = glm::normalize(point - a);
	return glm::dot(pointvec, normal) > 0.0;
}
int edgeLookupNodeCompare(const EdgeLookupNode& node, unsigned edge0, unsigned edge1)
{
	if (node.idx0 < edge0) return -1;
	else if (node.idx0 > edge0) return 1;
	else if (node.idx1 < edge1) return -1;
	else if (node.idx1 > edge1) return 1;
	else return 0;
}

bool hasEdge(const EdgeLookupNode& node, unsigned edge0, unsigned edge1)
{
	return (node.idx0 == edge0 && node.idx1 == edge1)
		|| (node.idx0 == edge1 && node.idx1 == edge0);
}

// vrati id prvniho prvku v poli edgeLookup, ktery je na stejne hrane
unsigned doEdgeLookup(const std::vector<EdgeLookupNode>& edgeLookup, unsigned edge0, unsigned edge1)
{
	if (edge0 < edge1)
	{
		std::swap(edge0, edge1);
	}

	unsigned loIdx = 0;
	unsigned hiIdx = edgeLookup.size();
	unsigned midIdx;
	EdgeLookupNode node;

	// binarne vyhledame shodny prvek
	while (true)
	{
		midIdx = (loIdx + hiIdx) / 2.0f;
		node = edgeLookup.at(midIdx);
		int comparison = edgeLookupNodeCompare(node, edge0, edge1);
		if (comparison == -1) loIdx = midIdx;
		else if (comparison == 1) hiIdx = midIdx;
		else break;
	}

	while (midIdx > 0)
	{
		// pokusime se najit nejnizsi shodny prvek
		node = edgeLookup.at(midIdx - 1);
		if (node.idx0 == edge0 && node.idx1 == edge1)
		{
			midIdx--;
		}
		else break;
	}

	return midIdx;
}

ShadowVolumeComputationInfo compute(
	unsigned indexOffset,
	unsigned indexCount,
	//const glm::vec3& lightDir,
	const glm::vec3& lightPos,
	float extrusionDistance,
	const std::vector<SimpleVertex>& inVertices, 
	const std::vector<GLuint>& inIndices, 
	const std::vector<EdgeLookupNode>& edgeLookup,
	std::vector<ShadowVolumeVertex>& outVertices)
{
	unsigned triCount = 0;
	
	for (unsigned i = 0; i < indexCount / 3; i++){
		auto triangleId = i;

		if (triangleId * 3 < indexCount)
		{
			unsigned firstIdx = indexOffset + triangleId * 3;

			unsigned aidx[3];
			aidx[0] = inIndices[firstIdx];
			aidx[1] = inIndices[firstIdx + 1];
			aidx[2] = inIndices[firstIdx + 2];
			glm::vec3 a0 = position(inVertices[aidx[0]]);
			glm::vec3 a1 = position(inVertices[aidx[1]]);
			glm::vec3 a2 = position(inVertices[aidx[2]]);

			
			glm::vec3 lightDir = -glm::normalize(lightPos);

			//je to privracena cast modelu - frontcap
			if (isFrontFacing(lightDir, a0, a1, a2))
			{
				emitTriangle(outVertices, a0, a1, a2, -2, 1);		

				//duplikace jako backcap
				emitTriangle(outVertices, a0 + glm::normalize(a0 - lightPos) * extrusionDistance, a2 + glm::normalize(a2 - lightPos) * extrusionDistance, a1 + glm::normalize(a1 - lightPos) * extrusionDistance, -2, 1);
				triCount += 2;
			}

			unsigned edgeIndices[] = { aidx[0], aidx[1], aidx[1], aidx[2], aidx[2], aidx[0] };

			for (unsigned edgeIdx = 0; edgeIdx < 3; edgeIdx++)
			{
				unsigned thisEdge[2];
				thisEdge[0] = edgeIndices[edgeIdx * 2];
				thisEdge[1] = edgeIndices[edgeIdx * 2 + 1];
				int edgeMultiplicity = 0;
				bool ignore = false;
				unsigned edgeNode;
				for (edgeNode = doEdgeLookup(edgeLookup, thisEdge[0], thisEdge[1]);
					edgeNode < indexCount && hasEdge(edgeLookup[edgeNode], thisEdge[0], thisEdge[1]);
					edgeNode++)
				{
					auto node = edgeLookup[edgeNode];

					// kazdou hranu zpracovava trojuhelnik s nejnizsim indexem
					if (node.triangleIdx < triangleId)
					{
						ignore = true;
						break;
					}

					glm::vec3 edge0 = position(inVertices[thisEdge[0]]);
					glm::vec3 edge1 = position(inVertices[thisEdge[1]]);
					glm::vec3 thirdVert = position(inVertices[node.idx2]);

					edgeMultiplicity += isInFront(thirdVert, edge0, edge1, edge1 + lightDir) ? -1 : 1;
				}

				if (!ignore && edgeMultiplicity != 0)
				{
					glm::vec3 edge0 = position(inVertices[edgeIndices[edgeIdx * 2]]);
					glm::vec3 edge1 = position(inVertices[edgeIndices[edgeIdx * 2 + 1]]);

					//vytahnuti stran
					emitTriangle(outVertices, edge0, edge1, edge0 + glm::normalize(edge0 - lightPos) * extrusionDistance, edgeMultiplicity, 0);
					emitTriangle(outVertices, edge1, edge1 + glm::normalize(edge1 - lightPos) * extrusionDistance, edge0 + glm::normalize(edge0 - lightPos) * extrusionDistance, edgeMultiplicity, 0);
					triCount += 2;
				}
			}
		}
	} //for
	
	return ShadowVolumeComputationInfo {triCount};
}
