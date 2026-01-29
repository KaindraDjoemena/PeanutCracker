#ifndef SCENE_NODE_H
#define SCENE_NODE_H


#define GLM_ENABLE_EXPERIMENTAL 

#include "transform.h"
#include "object.h"
#include "sphereColliderComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <memory>
#include <vector>


class SceneNode {
public:
	std::string	name;
	Transform	localTransform;
	glm::mat4	worldMatrix;
	bool		isDirty	= true;
	bool		isSelected = false;
	SceneNode*								parent = nullptr;
	std::vector<std::unique_ptr<SceneNode>>	children;
	std::unique_ptr<Object>	object = nullptr;

	std::unique_ptr<SphereColliderComponent> sphereColliderComponent;

	SceneNode(std::string i_name) : name(i_name) {
		sphereColliderComponent = std::make_unique<SphereColliderComponent>(localTransform.position, 1.0f);
	}

	glm::vec3 getPosition() const { return localTransform.position; }
	glm::vec3 getScale() const { return localTransform.scale; }
	glm::vec3 getEulerRotation() const { return glm::degrees(glm::eulerAngles(localTransform.quatRotation)); }

	// --Setters
	void setPosition(const glm::vec3& pos) {
		localTransform.position = pos;


		// *** FLAGS ***
		isDirty = true;
	}
	void setScale(const glm::vec3& scl) {
		localTransform.scale.x = scl.x < epsilon ? epsilon : scl.x;
		localTransform.scale.y = scl.y < epsilon ? epsilon : scl.y;
		localTransform.scale.z = scl.z < epsilon ? epsilon : scl.z;
		
		
		// *** FLAGS ***
		isDirty = true;
	}
	void setEulerRotation(const glm::vec3& eulerRotDegrees) {
		glm::vec3 radians = glm::radians(eulerRotDegrees);
		localTransform.quatRotation = glm::quat(radians);
		
		
		// *** FLAGS ***
		isDirty = true;
	}


	void setSphereComponentRadius() {
		if (object && object->modelPtr) {
			glm::vec3 maxAABB = object->modelPtr->aabb.max;
			glm::vec3 minAABB = object->modelPtr->aabb.min;

			float maxExtent = std::max(std::fabs(maxAABB.x), std::fabs(minAABB.x));
			maxExtent = std::max(maxExtent, std::max(std::fabs(maxAABB.y), std::fabs(minAABB.y)));
			maxExtent = std::max(maxExtent, std::max(std::fabs(maxAABB.z), std::fabs(minAABB.z)));

			sphereColliderComponent->localRadius = maxExtent;
		}

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


		// *** FLAGS ***
		this->isDirty = true;
	}

	void update(const glm::mat4& parentWorldMatrix, bool isParentDirty) {
		bool shouldUpdate = isDirty || isParentDirty;

		if (shouldUpdate) {
			worldMatrix = parentWorldMatrix * localTransform.getModelMatrix();
			float scaleX = glm::length(glm::vec3(worldMatrix[0]));
			float scaleY = glm::length(glm::vec3(worldMatrix[1]));
			float scaleZ = glm::length(glm::vec3(worldMatrix[2]));

			float maxScale = glm::max(scaleX, glm::max(scaleY, scaleZ));

			sphereColliderComponent->worldRadius = sphereColliderComponent->localRadius * maxScale;
			sphereColliderComponent->worldCenter = worldMatrix * glm::vec4(sphereColliderComponent->localCenter, 1.0f);

			isDirty = false;
		}

		if (object) {
			object->setScale(localTransform.scale);
			object->setPosition(localTransform.position);
			object->setQuatRotation(localTransform.quatRotation);
		}

		// Propagate update to children nodes
		for (auto& child : children) {
			child->update(worldMatrix, shouldUpdate);
		}
	}

	void addChild(std::unique_ptr<SceneNode> child) {
		child->parent = this;

		child->isDirty = true;

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