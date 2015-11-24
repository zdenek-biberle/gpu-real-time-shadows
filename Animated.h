#pragma once

#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#include "Animation.h"

class Animated {
public:
	Animated();
	~Animated();

	std::vector<Animation> moveAnimation;  //it's vector to enable adding multiple animations together
	std::vector<Animation> rotateAnimation;

	void animateTranslate(glm::mat4 &transformMatrix, glm::vec3 &position);
	void animateRotate(glm::mat4 &transformMatrix, glm::vec3 &rotation);
	//animate scale


	//before rotating translation of object position must be made
	void animateAll(glm::mat4 &transformMatrix, glm::vec3 &position, glm::vec3 &rotation);

};

