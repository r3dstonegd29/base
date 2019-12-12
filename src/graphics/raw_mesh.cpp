/* Start Header -------------------------------------------------------
Copyright (C) 2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior written consent of
DigiPen Institute of Technology is prohibited.
File Name:	raw_mesh.cpp
Purpose: Basic mesh handler
Author: Gabriel Ma�eru - gabriel.m
- End Header --------------------------------------------------------*/

#include "raw_mesh.h"
#include "gl_error.h"
#include <utils/math_utils.h>
#include <GL/gl3w.h>
void raw_mesh::draw()
{
	GL_CALL(glBindVertexArray(m_vao));
	GL_CALL(glDrawElements(GL_TRIANGLES, (GLsizei)faces.size(), GL_UNSIGNED_INT, 0));
	GL_CALL(glBindVertexArray(0));
}
void raw_mesh::load()
{
	if (m_vao == 0)
	{
		GL_CALL(glGenVertexArrays(1, &m_vao));
		GL_CALL(glBindVertexArray(m_vao));

		GL_CALL(glGenBuffers(1, &m_vtx));
		GL_CALL(glGenBuffers(1, &m_idx));
		GL_CALL(glGenBuffers(1, &m_uvs));
		GL_CALL(glGenBuffers(1, &m_norm));
		GL_CALL(glGenBuffers(1, &m_tan));
		GL_CALL(glGenBuffers(1, &m_bit));
	}

	GL_CALL(glBindVertexArray(m_vao));

	GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, m_vtx));
	GL_CALL(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), &vertices[0], GL_DYNAMIC_DRAW));
	GL_CALL(glEnableVertexAttribArray(0));
	GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr));

	GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, m_norm));
	GL_CALL(glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_DYNAMIC_DRAW));
	GL_CALL(glEnableVertexAttribArray(1));
	GL_CALL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr));

	GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, m_uvs));
	GL_CALL(glBufferData(GL_ARRAY_BUFFER, uv_coord.size() * sizeof(vec2), &uv_coord[0], GL_DYNAMIC_DRAW));
	GL_CALL(glEnableVertexAttribArray(2));
	GL_CALL(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr));

	GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, m_tan));
	GL_CALL(glBufferData(GL_ARRAY_BUFFER, tan.size() * sizeof(vec3), tan.data(), GL_DYNAMIC_DRAW));
	GL_CALL(glEnableVertexAttribArray(3));
	GL_CALL(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr));

	GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, m_bit));
	GL_CALL(glBufferData(GL_ARRAY_BUFFER, bit.size() * sizeof(vec3), bit.data(), GL_DYNAMIC_DRAW));
	GL_CALL(glEnableVertexAttribArray(4));
	GL_CALL(glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, nullptr));

	GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_idx));
	GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(unsigned), &faces[0], GL_DYNAMIC_DRAW));
	
	GL_CALL(glBindVertexArray(0));
}

void raw_mesh::free()
{
	if (m_vao > 0)
	{
		GL_CALL(glDeleteBuffers(1, &m_vtx));
		GL_CALL(glDeleteBuffers(1, &m_idx));
		if(m_uvs != 0)
			GL_CALL(glDeleteBuffers(1, &m_uvs));
		if (m_norm != 0)
			GL_CALL(glDeleteBuffers(1, &m_norm));
		GL_CALL(glDeleteVertexArrays(1, &m_vao));
	}
}

void raw_mesh::compute_normals()
{
	size_t scale = static_cast<size_t>(sqrt(vertices.size()));
	normals.resize(vertices.size());
	tan.resize(vertices.size());
	bit.resize(vertices.size());
	for (size_t y = 0; y < scale; y++)
	{
		for (size_t x = 0; x < scale; x++)
		{
			size_t x_prev = (x==0)		? x : x - 1;
			size_t x_post = (x==scale-1)? x : x + 1;
			size_t y_prev = (y==0)		? y : y - 1;
			size_t y_post = (y==scale-1)? y : y + 1;

			vec3 dx = glm::normalize(vertices[y*scale + x_post] - vertices[y*scale + x_prev]);
			vec3 dy = glm::normalize(vertices[y_post*scale + x] - vertices[y_prev*scale + x]);
			
			tan[y*scale + x] = dx;
			bit[y*scale + x] = dy;
			normals[y*scale + x] = glm::normalize(glm::cross(dy, dx));
		}
	}
}

void raw_mesh::build_plane(const int scale, float size)
{
	vertices.resize(scale*scale);
	uv_coord.resize(scale*scale);
	faces.resize((scale - 1)*(scale - 1) * 6);
	int tri_index = 0;
	auto add_tri = [&](int a, int b, int c)
	{
		faces[tri_index] = (unsigned)a;
		faces[tri_index + 1u] = (unsigned)b;
		faces[tri_index + 2u] = (unsigned)c;
		tri_index += 3;
	};
	int vtx_index = 0;
	for (int y = 0; y < scale; ++y)
	{
		for (int x = 0; x < scale; ++x)
		{
			vertices[vtx_index] = vec3{
				map<int, float>(x, 0, scale - 1, -scale * size / 2.0f, scale * size / 2.0f),
				0.0f,
				map<int, float>(y, 0, scale - 1, -scale * size / 2.0f, scale * size / 2.0f)
			};
			uv_coord[vtx_index] = {
				coef<int>(0, scale - 1, x),
				coef<int>(0, scale - 1, y),
			};
			if (x < scale - 1 && y < scale - 1)
			{
				add_tri(vtx_index, vtx_index + scale + 1, vtx_index + scale);
				add_tri(vtx_index + scale + 1, vtx_index, vtx_index + 1);
			}
			vtx_index++;
		}
	}
}
