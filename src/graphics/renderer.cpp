/* Start Header -------------------------------------------------------
Copyright (C) 2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior written consent of
DigiPen Institute of Technology is prohibited.
File Name:	renderer.cpp
Purpose: OpenGl renderer
Author: Gabriel Ma�eru - gabriel.m
- End Header --------------------------------------------------------*/

#include "renderer.h"
#include "gl_error.h"
#include <platform/window_manager.h>
#include <platform/window.h>
#include <platform/editor.h>
#include <scene/scene.h>
#include <utils/generate_noise.h>
#include <GL/gl3w.h>
#include <imgui/imgui.h>
#include <iostream>
#include <algorithm>

c_renderer* renderer = new c_renderer;

void c_renderer::update_max_draw_call_count()
{
	m_selection_calls = { 0u, scene->m_objects.size() };
	if (m_render_options.render_lights)
		m_selection_calls.second += scene->m_lights.size();
}

vec3 c_renderer::compute_selection_color()
{
	float scalar = static_cast<float>(m_selection_calls.first++) / static_cast<float>(m_selection_calls.second)*360.f;
	float val = fmod(scalar, 60.f)/60.f;

	vec3 color;
	if		(scalar < 60.f)
		color = { 1.0f, val, 0.0f };
	else if (scalar < 120.f)
		color = { 1.f-val, 1.0f, 0.0f };
	else if (scalar < 180.f)
		color = { 0.0f, 1.f, val };
	else if (scalar < 240.f)
		color = { 0.0f, 1.f-val, 1.0f};
	else if (scalar < 300.f)
		color = { val, 0.0f, 1.0f };
	else
		color = { 1.0f, 0.0f, 1.f-val };
	return color;
}

bool c_renderer::init()
{
	if (gl3wInit())
		return false;
	
	if (!gl3wIsSupported(4, 6))
		return false;

	// GL Options
	//setup_gl_debug();
	glCullFace(GL_FRONT);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	// Load Programs
	try {
		g_buffer_shader	= new Shader_Program("./data/shaders/basic.vert", "./data/shaders/g_buffer.frag");
		light_shader	= new Shader_Program("./data/shaders/basic.vert", "./data/shaders/light.frag");
		blur_shader	= new Shader_Program("./data/shaders/basic.vert", "./data/shaders/blur.frag");
		texture_shader	= new Shader_Program("./data/shaders/basic.vert", "./data/shaders/texture.frag");
	}
	catch (const std::string & log) { std::cout << log; }

	// Load Resources
	try
	{
		// Basic
		m_models.push_back(new Model("./data/meshes/cube.obj"));
		m_models.push_back(new Model("./data/meshes/octohedron.obj"));
		m_models.push_back(new Model("./data/meshes/quad.obj"));
		m_models.push_back(new Model("./data/meshes/sphere.obj"));

		// Complex
		m_models.push_back(new Model("./data/meshes/sponza.obj"));
		m_models.push_back(new Model("./data/meshes/phoenix.fbx"));
	}
	catch (const std::string & log) { std::cout << log; return false; }
	
	// Setup Cameras
	scene_cam.m_eye = { 4,16,44 };
	scene_cam.update();
	return true;
}

void c_renderer::update()
{
	// Setup Framebuffers
	if (window_manager->get_width() != g_buffer.m_width
	||  window_manager->get_height() != g_buffer.m_height)
	{
		g_buffer.setup(window_manager->get_width(), window_manager->get_height(), {
			GL_RGBA16F, GL_RGBA, GL_FLOAT,
			GL_RGBA16F, GL_RGBA, GL_FLOAT,
			GL_RGBA16F, GL_RGBA, GL_FLOAT,
			GL_RGBA16F, GL_RGBA, GL_FLOAT
			});
		light_buffer.setup(window_manager->get_width(), window_manager->get_height(), {
			GL_RGB16F, GL_RGB, GL_FLOAT
			}, g_buffer.m_depth_texture);
		blur_buffer.setup(window_manager->get_width(), window_manager->get_height(), {
			GL_R16F, GL_RED, GL_FLOAT,
			GL_RGB16F, GL_RGB, GL_FLOAT
			});
	}

	// Camera Update
	scene_cam.update();

	if (g_buffer_shader->is_valid())
	{
		// G_Buffer Pass	///////////////////////////////////////////////////////
		/**/GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.m_fbo));
		/**/GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		/**/GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		/**/GL_CALL(glViewport(0, 0, g_buffer.m_width, g_buffer.m_height));
		/**/
		/**/g_buffer_shader->use();
		/**/scene_cam.set_uniforms(g_buffer_shader);
		/**/GL_CALL(glEnable(GL_DEPTH_TEST));
		/**/update_max_draw_call_count();
		/**/scene->draw_objs(g_buffer_shader);
		/**/if (m_render_options.render_lights)
			/**/	scene->draw_debug_lights(g_buffer_shader);
		/**/GL_CALL(glDisable(GL_DEPTH_TEST));
		///////////////////////////////////////////////////////////////////////////
	}



	if (light_shader->is_valid())
	{
		// Light Pass	///////////////////////////////////////////////////////////
		/**/GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, light_buffer.m_fbo));
		/**/GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		/**/GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
		/**/GL_CALL(glViewport(0, 0, light_buffer.m_width, light_buffer.m_height));
		/**/glDepthMask(GL_FALSE);
		/**/light_shader->use();
		/**/
		/**/// Render Ambient
		/**/light_shader->set_uniform_subroutine(GL_FRAGMENT_SHADER, "render_ambient");
		/**/ortho_cam.set_uniforms(light_shader);
		/**/glActiveTexture(GL_TEXTURE0);
		/**/glBindTexture(GL_TEXTURE_2D, get_texture(DIFFUSE));
		/**/glActiveTexture(GL_TEXTURE1);
		/**/glBindTexture(GL_TEXTURE_2D, get_texture(POSITION));
		/**/glActiveTexture(GL_TEXTURE2);
		/**/glBindTexture(GL_TEXTURE_2D, get_texture(NORMAL));
		/**/m_models[2]->m_meshes[0]->draw(light_shader);
		/**/
		/**/// Render Lights
		/**/light_shader->set_uniform_subroutine(GL_FRAGMENT_SHADER, "render_diffuse_specular");
		/**/scene_cam.set_uniforms(light_shader);
		/**/light_shader->set_uniform("window_width", window_manager->get_width());
		/**/light_shader->set_uniform("window_height", window_manager->get_height());
		/**/GL_CALL(glEnable(GL_BLEND));
		/**/GL_CALL(glEnable(GL_DEPTH_TEST));
		/**/glDepthFunc(GL_GREATER);
		/**/glEnable(GL_CULL_FACE);
		/**/scene->draw_lights(light_shader);
		/**/glDisable(GL_CULL_FACE);
		/**/glDepthFunc(GL_LESS);
		/**/GL_CALL(glDisable(GL_DEPTH_TEST));
		/**/GL_CALL(glDepthMask(GL_TRUE));
		/**/GL_CALL(glDisable(GL_BLEND));
		///////////////////////////////////////////////////////////////////////////
	}

	if (blur_shader->is_valid())
	{
		// Blur Pass	///////////////////////////////////////////////////////////
		/**/GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, blur_buffer.m_fbo));
		/**/GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
		/**/GL_CALL(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
		/**/GL_CALL(glViewport(0, 0, blur_buffer.m_width, blur_buffer.m_height));
		/**/blur_shader->use();
		/**/GL_CALL(glEnable(GL_BLEND));
		/**/
		/**/// Sobel Edge Detection
		/**/if (m_render_options.do_antialiasing)
		/**/{
		/**/	blur_shader->set_uniform_subroutine(GL_FRAGMENT_SHADER, "do_sobel_edge_detection");
		/**/	ortho_cam.set_uniforms(blur_shader);
		/**/	blur_shader->set_uniform("width", (float)window_manager->get_width());
		/**/	blur_shader->set_uniform("height", (float)window_manager->get_height());
		/**/	blur_shader->set_uniform("coef_normal", m_render_options.aa_coef_normal);
		/**/	blur_shader->set_uniform("coef_depth", m_render_options.aa_coef_depth);
		/**/	blur_shader->set_uniform("depth_power", m_render_options.aa_depth_power);
		/**/	glActiveTexture(GL_TEXTURE0);
		/**/	glBindTexture(GL_TEXTURE_2D, get_texture(NORMAL));
		/**/	glActiveTexture(GL_TEXTURE1);
		/**/	glBindTexture(GL_TEXTURE_2D, get_texture(DEPTH));
		/**/	m_models[2]->m_meshes[0]->draw(blur_shader);
		/**/}
		/**/
		/**/// Blur
		/**/glFlush();
		/**/glFinish();
		/**/blur_shader->set_uniform_subroutine(GL_FRAGMENT_SHADER, "do_blur");
		/**/ortho_cam.set_uniforms(blur_shader);
		/**/glActiveTexture(GL_TEXTURE0);
		/**/glBindTexture(GL_TEXTURE_2D, get_texture(BLUR_FACTOR));
		/**/m_models[2]->m_meshes[0]->draw(blur_shader);
		/**/GL_CALL(glDisable(GL_BLEND));
		///////////////////////////////////////////////////////////////////////////
	}



	if (texture_shader->is_valid())
	{
		// Last Render Pass	///////////////////////////////////////////////////////
		/**/GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		/**/GL_CALL(glClearColor(0.50f, 0.75f, 0.93f, 1.0f));
		/**/GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		/**/GL_CALL(glViewport(0, 0, window_manager->get_width(), window_manager->get_height()));
		/**/
		/**/texture_shader->use();
		/**/ortho_cam.set_uniforms(texture_shader);
		/**/
		/**/glActiveTexture(GL_TEXTURE0);
		/**/glBindTexture(GL_TEXTURE_2D, get_texture(m_txt_cur));
		/**/
		/**/GL_CALL(glEnable(GL_DEPTH_TEST));
		/**/m_models[2]->m_meshes[0]->draw(texture_shader);
		/**/GL_CALL(glDisable(GL_DEPTH_TEST));
		///////////////////////////////////////////////////////////////////////////
	}
}

void c_renderer::shutdown()
{
	delete g_buffer_shader;
	delete light_shader;
	delete texture_shader;

	// Clean Meshes
	for (auto m : m_models)
		delete m;
	m_models.clear();
}

void c_renderer::drawGUI()
{
	if (ImGui::TreeNode("Camera"))
	{
		ImGui::SliderFloat("Near", &renderer->scene_cam.m_near, 0.1f, renderer->scene_cam.m_far);
		ImGui::SliderFloat("Far", &renderer->scene_cam.m_far, renderer->scene_cam.m_near, 1000.f);
		ImGui::SliderFloat("Fov", &renderer->scene_cam.m_fov, 30.0f, 150.f);
		if (auto target = scene_cam.get_target())
		{
			std::string name = target->m_model
				? target->m_model->m_name
				: "Unknown";
			ImGui::Text(("Target: "+name).c_str());
			ImGui::SameLine();
			if (ImGui::Button("Release"))
				scene_cam.release_target();
		}
		else
		{
			if (ImGui::BeginCombo("Target", "Select Obj"))
			{
				for (size_t n = 0; n < scene->m_objects.size(); n++)
				{
					if (ImGui::Selectable(scene->m_objects[n]->m_model->m_name.c_str(), false))
						scene_cam.use_target(scene->m_objects[n]);
				}
				ImGui::EndCombo();
			}
		}
		ImGui::TreePop();
	}
	
	if (ImGui::TreeNode("Buffers"))
	{
		std::function<void(c_renderer::e_texture)> show_image = [&](c_renderer::e_texture txt)
		{
			const float scale = 2.0f;
			const ImVec2 rect{ scale*192.f, scale*108.f };
			GLuint id = renderer->get_texture(txt);
			ImGui::Image(*reinterpret_cast<ImTextureID*>(&id), rect, ImVec2{ 0.f, 1.f }, ImVec2{ 1.f, 0.f });
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
				renderer->set_texture(txt);
		};
		if (ImGui::TreeNode("G-Buffer"))
		{
			show_image(c_renderer::DIFFUSE);
			show_image(c_renderer::POSITION);
			show_image(c_renderer::NORMAL);
			show_image(c_renderer::DEPTH);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Selection"))
		{
			show_image(c_renderer::SELECTION);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Light"))
		{
			show_image(c_renderer::LIGHT);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Blur"))
		{
			show_image(c_renderer::BLUR_FACTOR);
			show_image(c_renderer::BLUR_RESULT);
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("RenderOptions"))
	{
		if (ImGui::Button("Recompile Shaders"))
		{
			Shader_Program ** sh[]{ &g_buffer_shader, &light_shader, &texture_shader, &blur_shader };
			for (Shader_Program ** s : sh)
				*s = new Shader_Program((*s)->paths[0], (*s)->paths[1], (*s)->paths[2]);
		}
		ImGui::Checkbox("Render Lights", &m_render_options.render_lights);
		if (ImGui::TreeNode("Antialiasing"))
		{
			ImGui::Checkbox("Do Antialiasing", &m_render_options.do_antialiasing);
			if (m_render_options.do_antialiasing)
			{
				ImGui::SliderFloat("Normal Coefficient", &m_render_options.aa_coef_normal, 0.0f, 1.0f);
				ImGui::SliderFloat("Depth Coefficient", &m_render_options.aa_coef_depth, 0.0f, 1.0f);
				ImGui::InputFloat("Depth Power", &m_render_options.aa_depth_power);
			}
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
}

GLuint c_renderer::get_texture(e_texture ref)
{
	switch (ref)
	{
	case c_renderer::e_texture::SELECTION:
		return g_buffer.m_color_texture[0];
	case c_renderer::e_texture::DIFFUSE:
		return g_buffer.m_color_texture[1];
	case c_renderer::e_texture::POSITION:
		return g_buffer.m_color_texture[2]; 
	case c_renderer::e_texture::NORMAL:
		return g_buffer.m_color_texture[3];
	case c_renderer::e_texture::DEPTH:
		return g_buffer.m_depth_texture;
	case c_renderer::e_texture::LIGHT:
		return light_buffer.m_color_texture[0];
	case c_renderer::e_texture::BLUR_FACTOR:
		return blur_buffer.m_color_texture[0];
	case c_renderer::e_texture::BLUR_RESULT:
		return blur_buffer.m_color_texture[1];
	}
	return 0;
}

const Model * c_renderer::get_model(std::string s)
{
	for (auto& model : m_models)
		if (model->m_name == s)
			return model;
	return nullptr;
}
