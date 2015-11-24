#include "Animated.h"


Animated::Animated() {}


Animated::~Animated() {}

/*
Does NOT translate by object position.
*/
void Animated::animateTranslate(glm::mat4 &transformMatrix, glm::vec3 &position) {

	
	for (int i = 0; i < moveAnimation.size(); i++) {

		if (moveAnimation[i].isPlaying()) {

			vec3 tmp = moveAnimation[i].getValue();
			
			//std::cout << tmp.x << std::endl;
			if (!moveAnimation[i].shifting) {

				position = position + tmp;
			}
			else
				transformMatrix = glm::translate(transformMatrix, tmp);
		}
	}


}


/*
Rotation should be called last as it needs to be applied first and matrices are multiplied from the back.
Does NOT rotate by object rotation.


okay.. hmm animation is mostly 0 to 1.. this is converted to 0 - 360 an then to radians.. nice..
This takes degrees.. glm uses radians.. conversion happens
*/
void Animated::animateRotate(glm::mat4 &transformMatrix, glm::vec3 &rotation) {



	for (int i = 0; i < rotateAnimation.size(); i++) {

		if (rotateAnimation[i].isPlaying()) {
			vec3 anim = rotateAnimation[i].getValue();
			
			transformMatrix = glm::rotate(transformMatrix, glm::radians(anim.x * 360), glm::vec3(1.0f, 0.0f, 0.0f));
			transformMatrix = glm::rotate(transformMatrix, glm::radians(anim.y * 360), glm::vec3(0.0f, 1.0f, 0.0f));
			transformMatrix = glm::rotate(transformMatrix, glm::radians(anim.z * 360), glm::vec3(0.0f, 0.0f, 1.0f));
			
		}
	}

	
}

//does both.. INCLUDING position translation and rotation
void Animated::animateAll(glm::mat4 &transformMatrix, glm::vec3 &position, glm::vec3 &rotation) {

	animateTranslate(transformMatrix, position);
	
	transformMatrix = glm::translate(transformMatrix, position);


	animateRotate(transformMatrix, rotation);

	transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation.x * 360), glm::vec3(1.0f, 0.0f, 0.0f));
	transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation.y * 360), glm::vec3(0.0f, 1.0f, 0.0f));
	transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation.z * 360), glm::vec3(0.0f, 0.0f, 1.0f));
}

