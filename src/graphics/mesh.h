#pragma once
#include "shader_program.h"
#include "texture.h"
#include <glm/glm.h>
#include <GL/gl3w.h>
#include <vector>

struct VertexBuffer
{
	VertexBuffer(size_t s);
	VertexBuffer(const VertexBuffer&) = default;
	std::vector<vec3> position;
	std::vector<vec3> normal;
	std::vector<vec2> uv;
	std::vector<vec3> tangent;
	std::vector<vec3> bitangent;
};

struct Mesh
{
public:
	Mesh(const VertexBuffer& vertices, const std::vector<GLuint>& indices, int material_idx);
	~Mesh();

	void draw(Shader_Program* shader)const;
	int m_material_idx;

private:
	void load();

	GLuint m_VAO{ 0 };
	GLuint m_vertexbuffer{ 0 };
	GLuint m_normalbuffer{ 0 };
	GLuint m_uvbuffer{ 0 };
	GLuint m_tangentbuffer{ 0 };
	GLuint m_bitangentbuffer{ 0 };
	GLuint m_indexbuffer{ 0 };

	VertexBuffer m_vertices;
	std::vector<GLuint> m_indices;
	std::vector<Texture> m_textures;
};