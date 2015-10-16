#include "edgeLookup.h"

#include <algorithm>
#include <utility>

EdgeLookupNode::EdgeLookupNode(GLuint idx0, GLuint idx1, GLuint idx2, GLuint triangleIdx):
	idx0(idx0),
	idx1(idx1),
	idx2(idx2),
	triangleIdx(triangleIdx)
{
	if (idx0 < idx1)
	{
		std::swap(this->idx0, this->idx1);
	}
}

bool edgeLookupNodeCompare(EdgeLookupNode a, EdgeLookupNode b)
{
	if (a.idx0 < b.idx0) return true;
	else if (a.idx0 > b.idx0) return false;
	else if (a.idx1 < b.idx1) return true;
	else if (a.idx1 > b.idx1) return false;
	else if (a.triangleIdx < b.triangleIdx) return true;
	else return false;
}

//creates array of edges consisting of one edge, one non-edge vertex and triangle id to which it belongs
//finally it sorts them by idx0, then idx2 and eventually by triangle id
void generateEdgeLookup(
	const ModelInfo& modelInfo,
	const std::vector<GLuint>& indices,
	std::vector<EdgeLookupNode>& edgeLookup)
{
	edgeLookup.reserve(indices.size());
	
	//j is triangle id
	for (unsigned i = modelInfo.baseIndex, j = 0; i < modelInfo.baseIndex + modelInfo.indexCount; i += 3, j++)
	{
		auto idx0 = indices.at(i);
		auto idx1 = indices.at(i + 1);
		auto idx2 = indices.at(i + 2);
		
		edgeLookup.push_back(EdgeLookupNode(idx0, idx1, idx2, j));
		edgeLookup.push_back(EdgeLookupNode(idx0, idx2, idx1, j));
		edgeLookup.push_back(EdgeLookupNode(idx2, idx1, idx0, j));
	}
	
	std::sort(edgeLookup.begin(), edgeLookup.end(), edgeLookupNodeCompare);
}
