#ifndef SCENE_NODE_H
#define SCENE_NODE_h


#define GLM_ENABLE_EXPERIMENTAL 

#include "transform.h"
#include "object.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <memory>
#include <vector>


class SceneNode {
public:
	std::string		name;
	Transform		localTransform;
	glm::mat4		worldMatrix;
	bool			isDirty	= true;
	bool			isSelected = false;

	SceneNode*		parent = nullptr;
	std::vector<std::unique_ptr<SceneNode>>	children;

	std::unique_ptr<Object> object = nullptr;

	SceneNode(std::string i_name) : name(i_name) {}

	glm::vec3 getPosition() const { return localTransform.position; }
	glm::vec3 getScale() const { return localTransform.scale; }
	glm::vec3 getEulerRotation() const { return glm::degrees(glm::eulerAngles(localTransform.quatRotation)); }

	void setPosition(const glm::vec3& pos) {
		localTransform.position = pos;
		isDirty = true;
	}
	void setScale(const glm::vec3& scl) {
		localTransform.scale.x = scl.x < epsilon ? epsilon : scl.x;
		localTransform.scale.y = scl.y < epsilon ? epsilon : scl.y;
		localTransform.scale.z = scl.z < epsilon ? epsilon : scl.z;
		isDirty = true;
	}
	void setEulerRotation(const glm::vec3& eulerRotDegrees) {
		glm::vec3 radians = glm::radians(eulerRotDegrees);
		localTransform.quatRotation = glm::quat(radians);
		isDirty = true;
	}


	void updateFromMatrix(const glm::mat4& newLocalMatrix) {
		glm::vec3 skew;
		glm::vec4 perspective;

		glm::decompose(
			newLocalMatrix,
			localTransform.scale,
			localTransform.quatRotation,
			localTransform.position,
			skew,
			perspective
		);

		this->isDirty = true;
	}

	void update(const glm::mat4& parentWorldMatrix, bool isParentDirty) {
		bool shouldUpdate = isDirty || isParentDirty;

		if (shouldUpdate) {
			worldMatrix = parentWorldMatrix * localTransform.getModelMatrix();
			isDirty = false;
		}

		// Propagate update to children nodes
		for (auto& child : children) {
			child->update(worldMatrix, shouldUpdate);
		}
	}

	void addChild(std::unique_ptr<SceneNode> child) {
		// 1. Set the backlink so the child knows who its parent is
		child->parent = this;

		// 2. Mark the child as dirty so it inherits this node's world transform
		child->isDirty = true;

		// 3. Move ownership into our vector
		children.push_back(std::move(child));
	}

	std::unique_ptr<SceneNode> clone() const {
		auto newNode = std::make_unique<SceneNode>(name + "_copy");

		// Copy spatial data
		newNode->localTransform = this->localTransform;
		newNode->isDirty = true;

		// Copy the visual object (assuming Object has a simple constructor)
		if (this->object) {
			newNode->object = std::make_unique<Object>(this->object->modelPtr, this->object->shaderPtr);
		}

		// Recursively clone all children (The "Deep Copy")
		for (const auto& child : this->children) {
			newNode->addChild(child->clone());
		}

		return newNode;
	}
};

#endif