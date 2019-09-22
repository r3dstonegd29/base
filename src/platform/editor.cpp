
#include "editor.h"
#include "window_manager.h"
#include "window.h"
#include <functional>
#include <graphics/renderer.h>
#include <scene/scene.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

c_editor* editor = new c_editor;

bool c_editor::init()
{
	// Start ImGui
	ImGui::CreateContext();
	if (!ImGui_ImplGlfw_InitForOpenGL(window_manager->m_window->m_window, true))
		return false;
	const char* glsl_version = "#version 440";
	if(!ImGui_ImplOpenGL3_Init(glsl_version))
		return false;

	// Set Own Style
	ImGui::StyleColorsLight();
	ImGui::GetStyle().FrameRounding = 12;
	ImGui::GetStyle().Colors[2] = ImVec4{ 1.0f, 1.0f, 1.0f, 0.6f };
	ImGui::GetStyle().Colors[10] = ImVec4{ 0.8f, 0.0f, 0.2f, 1.0f };
	ImGui::GetStyle().Colors[11] = ImVec4{ 0.8f, 0.0f, 0.2f, 1.0f };

	return true;
}

void c_editor::update()
{
	// Draw ImGui
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	//ImGui::ShowDemoWindow();
	draw_main_window();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void c_editor::shutdown()
{
	// Exit ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void c_editor::draw_main_window()
{
	ImGui::Begin("Options", nullptr);
	renderer->drawGUI();
	ImGui::End();

	ImGui::Begin("Scene", nullptr);
	scene->drawGUI();
	ImGui::End();
}