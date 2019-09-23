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

bool c_renderer::init()
{
	if (gl3wInit())
		return false;
	
	if (!gl3wIsSupported(4, 0))
		return false;
	setup_gl_debug();
	// GL Options
	GL_CALL(glEnable(GL_DEPTH_TEST));

	// Load Programs
	try {
		g_buffer_shader = new Shader_Program("./data/shaders/basic.vert", "./data/shaders/g_buffer.frag");
		light_shader = new Shader_Program("./data/shaders/basic.vert", "./data/shaders/light.frag");
		texture_shader = new Shader_Program("./data/shaders/basic.vert", "./data/shaders/texture.frag");
	}
	catch (const std::string & log) { std::cout << log; return false; }

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

	// Setup Framebuffers
	g_buffer.setup(window_manager->get_width(), window_manager->get_height(),{
		GL_RGBA16F, GL_RGBA,
		GL_RGB16F, GL_RGB,
		GL_RGBA16F, GL_RGBA,
		GL_RGB16F, GL_RGB,
		GL_RGBA16F, GL_RGBA,
		GL_RGB16F, GL_RGB
		});
	light_buffer.setup(window_manager->get_width(), window_manager->get_height(), {
		GL_RGBA16F, GL_RGBA
		});
	return true;
}

void c_renderer::update()
{
	// Camera Update
	scene_cam.update();

	// G_Buffer Pass	///////////////////////////////////////////////////////
	/**/GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.m_fbo));
	/**/GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	/**/GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	/**/GL_CALL(glViewport(0, 0, g_buffer.m_width, g_buffer.m_height));
	/**/
	/**/g_buffer_shader->use();
	/**/g_buffer_shader->set_uniform("P", scene_cam.m_proj);
	/**/g_buffer_shader->set_uniform("V", scene_cam.m_view);
	/**/scene->draw(g_buffer_shader);
	///////////////////////////////////////////////////////////////////////////



	// Light Pass	///////////////////////////////////////////////////////////
	/**/GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, light_buffer.m_fbo));
	/**/GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	/**/GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	/**/GL_CALL(glViewport(0, 0, light_buffer.m_width, light_buffer.m_height));
	/**/
	/**/light_shader->use();
	/**/light_shader->set_uniform("P", mat4(1.0f));
	/**/light_shader->set_uniform("V", mat4(1.0f));
	/**/light_shader->set_uniform("M", mat4(1.0f));
	/**/
	/**/light_shader->set_uniform("light_position", vec3(scene_cam.m_view * vec4(scene->m_objects[1]->m_transform.get_pos(), 1.0f)));
	/**/light_shader->set_uniform("la", la);
	/**/light_shader->set_uniform("ld", ld);
	/**/light_shader->set_uniform("ls", ls);
	/**/light_shader->set_uniform("att_factor", att_factor);
	/**/
	/**/glActiveTexture(GL_TEXTURE0);
	/**/light_shader->set_uniform_sampler(0);
	/**/glBindTexture(GL_TEXTURE_2D, get_texture(DIFFUSE));
	/**/
	/**/glActiveTexture(GL_TEXTURE1);
	/**/light_shader->set_uniform_sampler(1);
	/**/glBindTexture(GL_TEXTURE_2D, get_texture(POSITION));
	/**/
	/**/glActiveTexture(GL_TEXTURE2);
	/**/light_shader->set_uniform_sampler(2);
	/**/glBindTexture(GL_TEXTURE_2D, get_texture(NORMAL));
	/**/
	/**/m_models[2]->m_meshes[0]->draw(light_shader);
	///////////////////////////////////////////////////////////////////////////



	// Last Render Pass	///////////////////////////////////////////////////////
	/**/GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	/**/GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	/**/GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	/**/GL_CALL(glViewport(0, 0, window_manager->get_width(), window_manager->get_height()));
	/**/
	/**/texture_shader->use();
	/**/texture_shader->set_uniform("P", mat4(1.0f));
	/**/texture_shader->set_uniform("V", mat4(1.0f));
	/**/texture_shader->set_uniform("M", mat4(1.0f));
	/**/
	/**/texture_shader->set_uniform_sampler(0);
	/**/glActiveTexture(GL_TEXTURE0);
	/**/glBindTexture(GL_TEXTURE_2D, get_texture(m_txt_cur));
	/**/
	/**/m_models[2]->m_meshes[0]->draw(texture_shader);
	///////////////////////////////////////////////////////////////////////////
}

void c_renderer::shutdown()
{
	delete color_shader;
	delete g_buffer_shader;

	// Clean Meshes
	for (auto m : m_models)
		delete m;
	m_models.clear();
}

void c_renderer::drawGUI()
{

	if (ImGui::TreeNode("Camera"))
	{
		ImGui::SliderFloat("Near", &renderer->scene_cam.m_near, 0.001f, renderer->scene_cam.m_far);
		ImGui::SliderFloat("Far", &renderer->scene_cam.m_far, renderer->scene_cam.m_near, 1000.f);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Light"))
	{
		ImGui::DragFloat("LA", &la, 0.1f, 0.0f, 1.0f);
		ImGui::DragFloat3("LD", &ld.x, 0.1f, 0.0f, 1.0f);
		ImGui::DragFloat3("LS", &ls.x, 0.1f, 0.0f, 1.0f);
		ImGui::DragFloat3("Att", &att_factor.x, 0.1f, 0.0f, 1.0f);
		ImGui::TreePop();
	}
	
	std::function<void(c_renderer::e_texture)> show_image = [&](c_renderer::e_texture txt)
	{
		const float scale = 2.0f;
		const ImVec2 rect{ scale*192.f, scale*108.f };
		GLuint id = renderer->get_texture(txt);
		ImGui::Image(*reinterpret_cast<ImTextureID*>(&id), rect, ImVec2{ 0.f, 1.f }, ImVec2{ 1.f, 0.f });
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
			renderer->set_texture(txt);
	};
	show_image(c_renderer::DIFFUSE_rgb);
	show_image(c_renderer::POSITION_rgb);
	show_image(c_renderer::NORMAL_rgb);
	show_image(c_renderer::DEPTH);
	show_image(c_renderer::LIGHT);
}

GLuint c_renderer::get_texture(e_texture ref)
{
	switch (ref)
	{
	case c_renderer::e_texture::DIFFUSE:
		return g_buffer.m_color_texture[0];
	case c_renderer::e_texture::DIFFUSE_rgb:
		return g_buffer.m_color_texture[1];
	case c_renderer::e_texture::POSITION:
		return g_buffer.m_color_texture[2];
	case c_renderer::e_texture::POSITION_rgb:
		return g_buffer.m_color_texture[3];
	case c_renderer::e_texture::NORMAL:
		return g_buffer.m_color_texture[4];
	case c_renderer::e_texture::NORMAL_rgb:
		return g_buffer.m_color_texture[5];
	case c_renderer::e_texture::DEPTH:
		return g_buffer.m_depth_texture;
	case c_renderer::e_texture::LIGHT:
		return light_buffer.m_color_texture[0];
	}
}

const Model * c_renderer::get_model(std::string s)
{
	for (auto& model : m_models)
		if (model->m_name == s)
			return model;
	return nullptr;
}
