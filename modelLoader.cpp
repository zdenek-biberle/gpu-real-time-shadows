#include "modelLoader.h"

#include <cassert>
#include <stdexcept>
#include <sstream>

#include <glm/glm.hpp>

ModelInfo::ModelInfo():
	baseIndex(0u),
	indexCount(0u),
	transform(glm::mat4(1.0f))
{}

ModelInfo::ModelInfo(unsigned int baseIndex, unsigned int indexCount):
	baseIndex(baseIndex),
	indexCount(indexCount),
	transform(glm::mat4(1.0f))
{}

ModelInfo::ModelInfo(unsigned int baseIndex, unsigned int indexCount, const glm::mat4& transform):
	baseIndex(baseIndex),
	indexCount(indexCount),
	transform(transform)
{}

/*
Loads .obj file into modelGroups.  Barebone version..
*/
ModelInfo loadModel(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<GLuint>& outIndices){

	using namespace std;


	std::cout << "\rLoading model: " << filename << std::endl;

	ModelInfo info;
	info.baseIndex = outVertices.size();
	info.indexCount = 0;

	int old_index_count = outIndices.size();

	//open file
	std::ifstream file(filename);


	if (!file.is_open())
		throw new std::runtime_error("Could not open model file " + filename);

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	vector<vector<string>> faces;

	std::istringstream ss(filename);
	std::string token;
	glm::vec3 indices;

	string path, last;

	//extract path without last filename from filename
	while (std::getline(ss, token, '/')) {

		if (!last.empty() && !path.empty())
			path = path + "/" + last;

		if (!last.empty() && path.empty())
			path = last;

		last = token;
	}




	string line;

	while (getline(file, line))
	{
		std::istringstream iLineStream(line);
		string mark;

		iLineStream >> mark;

		//line with vertex
		if (mark.compare("v") == 0){
			
			glm::vec3 point;

			iLineStream >> point.x;
			iLineStream >> point.y;
			iLineStream >> point.z;

			vertices.push_back(point);

			//line with face
		}
		else if (mark.compare("f") == 0){
			// v/t/n      * n
			// v/n
			// v//n
			// v

			string tmp;
			vector<string> tmp_vector;

			while (iLineStream >> tmp){

				if (tmp == "")  //! sure if neccesssary
					break;

				tmp_vector.push_back(tmp);	//just save the line for future use in untangle..
			}

			faces.push_back(tmp_vector);   //should be only 3 in this case

		}
		else if (mark.compare("vn") == 0){

			glm::vec3 normal;

			iLineStream >> normal.x;
			iLineStream >> normal.y;
			iLineStream >> normal.z;

			normals.push_back(normal);

		}
		else if (mark.compare("vt") == 0){  //ignore


			glm::vec2 coordinates;

			iLineStream >> coordinates.x;
			iLineStream >> coordinates.y;

			//textureCoordinates.push_back(coordinates);

		}
		else if (mark.compare("mtllib") == 0){  //ignore

			std::string materialFilename;

			iLineStream >> materialFilename;

			//getMaterials(materials, path + "/" + materialFilename);

		}
		else if (mark.compare("g") == 0){  //group can have material.. //ignore

			string name;

			iLineStream >> name;

			cout << "  g " << name << endl;




		}
		else if (mark.compare("usemtl") == 0){	//ignore

			string name;

			iLineStream >> name;

			//auto it = find_if(materials.begin(), materials.end(), [&name](const Material &mat) { return mat.name == name; });

			//if material is found assign it to modelGroup
			//if (it != materials.end())
			//	modelGroups.back().material = *it;


		}
		else if (mark.compare("o") == 0){
			////////////////////////////////////////////////probably useless
			string name;

			iLineStream >> name;

			cout << "o " << name << endl;


		}


	}



		//appends new vertex / normal pairs as needed, fills out indices and vertices
		untangle(vertices, normals, faces, outVertices, outIndices);

	

		cout << outIndices.size() / 3 << " triangles.\n";

		
		info.indexCount = outIndices.size() - old_index_count;
	

	cout << filename << " loaded. \n";

	return info; 
}
 

void untangle(std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals, std::vector<std::vector<std::string>> &faces, std::vector<Vertex>& outVertices, std::vector<GLuint> &outIndices){

	//! sure if can there be duplicates, but i dont care now..

	/*
	ok.. i got vertices, normals and texture coordinates - their size is NOT the same, indexes are into global vectors
	I must create new vectors for them and put there coresponding values at common index

	and faces in format x/x/x or generally x/y/z
	I need to generate correct indexes for element buffer

	x/x/x are fine.. element is simply x

	x/y/z append vertex x, normal y and texturecoord z
	and set element as size


	for faces with more elements than 3 treat it as a triangle fan

	now it should be ok.. can draw with this..
	*/


	for (auto const face : faces){


		for (auto const item : face){     // 3 - n items of format x/y/z

			std::istringstream ss(item);
			std::string token;
			glm::vec3 indices;

			std::vector<int> tmp;	//v/t/n		3
			//v/n		2
			//v/t		2
			//v			1
			//v//n		3

			while (std::getline(ss, token, '/')) {
				if (token != "")
					tmp.push_back(std::stoi(token) - 1);	//in the file it starts from 1

			}


			int vertex_index;
			int tex_index;
			int normal_index;


			vertex_index = tmp[0];
			tex_index = tmp[1];
			normal_index = tmp[2];

			//create new vertex with its attributes on common index
			Vertex vertex(vertices[vertex_index], normals[normal_index], glm::vec2());

			outVertices.push_back(vertex); //save it
			outIndices.push_back(outVertices.size() - 1);	//note its index


		}
	}
}
