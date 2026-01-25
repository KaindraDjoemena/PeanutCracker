#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include "object.h"

#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>


void drawSelectionOutline(std::) {
	glEnable(GL_DEPTH_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);
	DrawTwoContainers();

	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glDisable(GL_DEPTH_TEST);
	shaderSingleColor.use();
	DrawTwoScaledUpContainers();
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glEnable(GL_DEPTH_TEST);
}


#endif