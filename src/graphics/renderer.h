/* Start Header -------------------------------------------------------
Copyright (C) 2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior written consent of
DigiPen Institute of Technology is prohibited.
File Name:	renderer.h
Purpose: OpenGl renderer
Author: Gabriel Ma�eru - gabriel.m
- End Header --------------------------------------------------------*/

#pragma once
#include "shader_program.h"
#include "vectorial_camera.h"
#include "ortho_camera.h"
#include "model.h"
#include "framebuffer.h"
#include "curve.h"
#include "ocean.h"
#include <glm/glm.h>

class c_renderer
{
public:
	// Shaders
	Shader_Program* g_buffer_shader;
	Shader_Program* decal_shader;
	Shader_Program* ocean_shader;
	Shader_Program* light_shader;
	Shader_Program* blur_shader;
	Shader_Program* texture_shader;
	Shader_Program* color_shader;
	Shader_Program* skybox_shader;

	// Cameras
	vectorial_camera scene_cam{};
	ortho_camera ortho_cam{};

	// Meshes
	std::vector<Model*> m_models;

	//Curves
	std::vector<curve_base*> m_curves;

	// Framebuffer
	framebuffer g_buffer;
	framebuffer selection_buffer;
	framebuffer light_buffer;
	framebuffer blur_control_buffer;
	framebuffer bloom_buffer;
	framebuffer blur_buffer;

	enum e_texture {
		DIFFUSE,
		POSITION,
		METALLIC,
		NORMAL,
		SELECTION,
		LIN_DEPTH,
		DEPTH,
		COPY_DEPTH,
		LIGHT,
		BLUR_CONTROL,
		BLOOM,
		BLOOM2,
		BLUR,
		BLUR2
	}m_txt_cur{ BLUR };

	Texture skybox;
	Ocean m_ocean;

	std::pair<size_t,size_t> m_selection_calls{0u,0u};
	void update_max_draw_call_count();
	void recompile_shaders();

	bool init();
	void update();
	void shutdown();
	void drawGUI();
	void set_debug_color(vec3 c);
	void reset_debug_color();
	GLuint get_texture(e_texture ref);
	void set_texture(e_texture ref) { m_txt_cur = ref; }
	const Model* get_model(std::string s);
	vec3 compute_selection_color();

	struct Options
	{
		bool render_lights{ false };
		bool render_curves{ false };

		bool  do_antialiasing{ true };
		float aa_coef_normal{ 0.05f };
		float aa_coef_depth{ 0.25f };

		bool  do_depth_of_field{ false };
		float df_plane_focus{ 45.f };
		float df_aperture{ 45.f };
		bool  df_auto_focus{ false };

		bool  do_motion_blur{ false };
		bool  mb_camera_blur{ false };

		bool  do_bloom{ true };
		float bl_coef{ 1.f };

		int   blur_bloom_iterations{ 1 };
		int   blur_general_iterations{ 1 };
		int   blur_mode{ 0 };
		float bilat_threshold{ 0.01f };

		float dc_angle{ 0.8f };
		int   dc_mode{ 0 };
		bool  dc_active{ true };
	}m_render_options;

	friend class c_editor;
	friend class scene_object;
	friend struct curve_interpolator;
	friend class c_scene;
	friend class point_light;
};
extern c_renderer* renderer;