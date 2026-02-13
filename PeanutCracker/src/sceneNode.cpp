#include "headers/sceneNode.h"
#include "headers/transform.h"
#include "headers/object.h"
#include "headers/sphereColliderComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <memory>
#include <vector>


SceneNode::SceneNode(std::string i_name) : name(i_name) {
	sphereColliderComponent = std::make_unique<SphereColliderComponent>(localTransform.position, 1.0f);
}

// --Getters
glm::vec3 SceneNode::getPosition() const { return localTransform.position; }
glm::vec3 SceneNode::getScale() const { return localTransform.scale; }
glm::vec3 SceneNode::getEulerRotation() const { return glm::degrees(glm::eulerAngles(localTransform.quatRotation)); }

// --Setters
void SceneNode::setPosition(const glm::vec3& pos) {
	localTransform.position = pos;


	// *** FLAGS ***
	isDirty = true;
}
void SceneNode::setScale(const glm::vec3& scl, bool uniform) {
	glm::vec3 finalScale = scl;

	if (uniform) {
		glm::vec3 currScale = localTransform.scale;
		float ratio = 1.0f;
		const float EPS = 1e-5f;

		if (std::abs(scl.x - currScale.x) > EPS) {
			ratio = (std::abs(currScale.x) > EPS) ? (scl.x / currScale.x) : scl.x;
			finalScale.y = currScale.y * ratio;
			finalScale.z = currScale.z * ratio;
		}
		else if (std::abs(scl.y - currScale.y) > EPS) {
			ratio = (std::abs(currScale.y) > EPS) ? (scl.y / currScale.y) : scl.y;
			finalScale.x = currScale.x * ratio;
			finalScale.z = currScale.z * ratio;
		}
		else if (std::abs(scl.z - currScale.z) > EPS) {
			ratio = (std::abs(currScale.z) > EPS) ? (scl.z / currScale.z) : scl.z;
			finalScale.x = currScale.x * ratio;
			finalScale.y = currScale.y * ratio;
		}
	}

	localTransform.scale.x = std::max(finalScale.x, epsilon);
	localTransform.scale.y = std::max(finalScale.y, epsilon);
	localTransform.scale.z = std::max(finalScale.z, epsilon);


	// *** FLAGS ***
	isDirty = true;
}
void SceneNode::setEulerRotation(const glm::vec3& eulerRotDegrees) {
	localTransform.setRotDeg(eulerRotDegrees);

	// *** FLAGS ***
	isDirty = true;
}


void SceneNode::setSphereComponentRadius() const {
	if (object && object->modelPtr) {
		glm::vec3 maxAABB = object->modelPtr->aabb.max;
		glm::vec3 minAABB = object->modelPtr->aabb.min;

		float maxExtent = std::max(std::fabs(maxAABB.x), std::fabs(minAABB.x));
		maxExtent = std::max(maxExtent, std::max(std::fabs(maxAABB.y), std::fabs(minAABB.y)));
		maxExtent = std::max(maxExtent, std::max(std::fabs(maxAABB.z), std::fabs(minAABB.z)));

		sphereColliderComponent->localRadius = maxExtent;
	}

}

void SceneNode::updateFromMatrix(const glm::mat4& newLocalMatrix) {
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

void SceneNode::update(const glm::mat4& parentWorldMatrix, bool isParentDirty) {
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

void SceneNode::addChild(std::unique_ptr<SceneNode> child) {
	child->parent = this;

	child->isDirty = true;

	children.push_back(std::move(child));
}

std::unique_ptr<SceneNode> SceneNode::clone() const {
	auto newNode = std::make_unique<SceneNode>(name + "_copy");

	// Copy spatial data
	newNode->localTransform = this->localTransform;
	newNode->isDirty = true;

	// Copy the visual object (assuming Object has a simple constructor)
	if (this->object) {
		newNode->object = std::make_unique<Object>(this->object->modelPtr);
	}

	// Recursively clone all children (The "Deep Copy")
	for (const auto& child : this->children) {
		newNode->addChild(child->clone());
	}

	return newNode;
}