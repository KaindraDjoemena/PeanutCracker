#pragma once
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

	SceneNode(std::string i_name);

	glm::vec3 getPosition() const;
	glm::vec3 getScale() const;
	glm::vec3 getEulerRotation() const;

	// --Setters
	void setPosition(const glm::vec3& pos);
	void setScale(const glm::vec3& scl);
	void setEulerRotation(const glm::vec3& eulerRotDegrees);
	void setSphereComponentRadius();

	void updateFromMatrix(const glm::mat4& newLocalMatrix);

	void update(const glm::mat4& parentWorldMatrix, bool isParentDirty);

	void addChild(std::unique_ptr<SceneNode> child);

	std::unique_ptr<SceneNode> clone() const;
};