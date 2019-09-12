#pragma once
#include <vector>
#include <GL/gl3w.h>
struct framebuffer
{
	void setup(GLsizei width, GLsizei height, size_t txt_count);

	GLsizei m_width;
	GLsizei m_height;
	GLuint m_fbo;
	std::vector<GLuint> m_color_texture;
	GLuint m_depth_texture;
};