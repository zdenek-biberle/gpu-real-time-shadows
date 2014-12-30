#include "simplifyModel.h"

#include <map>

struct SimpleVertexComparer
{
	bool operator()(const SimpleVertex& a, const SimpleVertex& b) const
	{
		if (a._x < b._x)
		{
			return true;
		}
		else if (a._x > b._x)
		{
			return false;
		}

		if (a._y < b._y)
		{
			return true;
		}
		else if (a._y > b._y)
		{
			return false;
		}
		
		if (a._z < b._z)
		{
			return true;
		}
		else if (a._z > b._z)
		{
			return false;
		}
		
		return false;
	}
};

ModelInfo simplifyModel(
	ModelInfo inModel,
	const std::vector<Vertex>& inVertices, 
	const std::vector<GLuint>& inIndices,
	std::vector<SimpleVertex>& outVertices,
	std::vector<GLuint>& outIndices)
{
	unsigned baseIndex = outIndices.size();
	
	std::map<SimpleVertex, GLuint, SimpleVertexComparer> verticesLookup;
	
	for (unsigned i = inModel.baseIndex; i < inModel.baseIndex + inModel.indexCount; i++)
	{
		auto originalIndex = inIndices.at(i);
		auto originalVertex = inVertices.at(originalIndex);
		auto newVertex = SimpleVertex(originalVertex._x, originalVertex._y, originalVertex._z);
		
		auto findVertex = verticesLookup.find(newVertex);
		if (findVertex == verticesLookup.end())
		{
			auto newIdx = static_cast<GLuint>(outVertices.size());
			outVertices.push_back(newVertex);
			verticesLookup.insert(std::make_pair(newVertex, newIdx));
			outIndices.push_back(newIdx);
		}
		else
		{
			outIndices.push_back(findVertex->second);
		}
	}
	
	return ModelInfo {baseIndex, inModel.indexCount};
}
