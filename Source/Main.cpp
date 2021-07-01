#include "Application.h"
#include "Logger.h"
#include "FirsrPersonCamera.h"
#include "ThirdPersonCamera.h"
#include "Shader.h"
#include "MatrixStack.h"
#include "Texture2D.h"
#include "Light.h"

#include "Rectangle.h"
#include "Cube.h"
#include "Sphere.h"
#include "ViewVolume.h"


class NexusDemo final : public Nexus::Application {
public:
	NexusDemo() {
		Settings.Width = 800;
		Settings.Height = 600;
		Settings.WindowTitle = "Scientific Visualization | Project #5: High Dimensional Data Visualization";
		Settings.EnableDebugCallback = true;
		Settings.EnableFullScreen = false;

		Settings.EnableGhostMode = true;
		Settings.EnableFaceCulling = true;
		Settings.CullingTypeStr = "Back Face";
		Settings.ShowOriginAnd3Axes = false;

		// Projection Settings Initalize
		ProjectionSettings.IsPerspective = true;
		ProjectionSettings.OrthogonalHeight = 5.0f;
		ProjectionSettings.ClippingNear = 0.1f;
		ProjectionSettings.ClippingFar = 500.0f;
		ProjectionSettings.Aspect = (float)Settings.Width / (float)Settings.Height;
	}

	void Initialize() override {
		// Setting OpenGL
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Create shader program
		// myShader = std::make_unique<Nexus::Shader>("Shaders/lighting.vert", "Shaders/lighting.frag");
		myShader = std::make_unique<Nexus::Shader>("Shaders/simple_lighting.vert", "Shaders/simple_lighting.frag");
		// simpleDepthShader = std::make_unique<Nexus::Shader>("Shaders/simple_depth_shader.vert", "Shaders/simple_depth_shader.frag");
		// debugDepthQuad = std::make_unique<Nexus::Shader>("Shaders/debug_quad.vert", "Shaders/debug_quad_depth.frag");
		// normalShader = std::make_unique<Nexus::Shader>("Shaders/normal_visualization.vs", "Shaders/normal_visualization.fs", "Shaders/normal_visualization.gs");
		
		// Create Camera
		first_camera = std::make_unique<Nexus::FirstPersonCamera>(glm::vec3(0.0f, 2.0f, 5.0f));
		third_camera = std::make_unique<Nexus::ThirdPersonCamera>(glm::vec3(0.0f, 0.0f, 5.0f));
		// first_camera->SetRestrict(true);
		// first_camera->SetRestrictValue(glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(10.0f, 20.0f, 10.0f));
		// third_camera->SetRestrict(true);
		// third_camera->SetRestrictValue(glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(10.0f, 20.0f, 10.0f));
		
		// Create Matrix Stack
		model = std::make_unique<Nexus::MatrixStack>();

		// Create object data
		floor = std::make_unique<Nexus::Rectangle>(20.0f, 20.0f, 10.0f, Nexus::POS_Y);
		cube = std::make_unique<Nexus::Cube>();
		sphere = std::make_unique<Nexus::Sphere>();

		view_volume = std::make_unique<Nexus::ViewVolume>();

		// Loading textures
		texture_checkerboard = Nexus::Texture2D::CreateFromFile("Resource/Textures/chessboard-metal.png", true);
		texture_checkerboard->SetWrappingParams(GL_REPEAT, GL_REPEAT);

        // Create a point light
        point_light = std::make_unique<Nexus::PointLight>(glm::vec3(0.0f, 0.0f, 0.0f), true);
	}

	void Update() override {
        // Lighting Setting
        point_light->SetPosition(Settings.EnableGhostMode ? first_camera->GetPosition() : third_camera->GetPosition());
	}
	
	void Render(Nexus::DisplayMode monitor_type) override {

		if (Settings.EnableFaceCulling) {
			// CW => Clockwise is the front face
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CW);
			if (Settings.CullingTypeStr == "Back Face") {
				glCullFace(GL_BACK);
			} else {
				glCullFace(GL_FRONT);
			}
		} else {
			glDisable(GL_CULL_FACE);
		}

        SetViewMatrix(Nexus::DISPLAY_MODE_ORTHOGONAL_Z);
        SetProjectionMatrix(Nexus::DISPLAY_MODE_ORTHOGONAL_Z);
        SetViewport(Nexus::DISPLAY_MODE_ORTHOGONAL_Z);

		myShader->Use();
        myShader->SetMat4("view", view);
        myShader->SetMat4("projection", projection);
        myShader->SetVec3("viewPos", Settings.EnableGhostMode ? first_camera->GetPosition() : third_camera->GetPosition());
        myShader->SetVec3("lightPos", point_light->GetPosition());
        myShader->SetVec3("lightColor", point_light->GetDiffuse());

		// ==================== Draw origin and 3 axes ====================
		if (Settings.ShowOriginAnd3Axes) {
			this->DrawOriginAnd3Axes(myShader.get());
		}

		// ==================== Draw a room ====================
		/*
		myShader->SetBool("material.enableDiffuseTexture", true);
		myShader->SetBool("material.enableSpecularTexture", false);
		myShader->SetBool("material.enableEmission", false);
		myShader->SetBool("material.enableEmissionTexture", false);
		myShader->SetFloat("material.shininess", 64.0f);
		*/
        myShader->SetVec3("objectColor", glm::vec3(0.482352941, 0.68627451, 0.929411765));
		model->Push();
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 20.0f, 0.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 10.0f, -10.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 10.0f, 10.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();
		
		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(10.0f, 10.0f, 0.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(-10.0f, 10.0f, 0.0f)));
		model->Save(glm::rotate(model->Top(), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
		floor->BindTexture(0, texture_checkerboard.get());
		floor->Draw(myShader.get(), model->Top());
		model->Pop();
		model->Pop();

		// third_camera->SetTarget(balls[0].GetPosition());
		// ImGui::ShowDemoWindow();
	}

	void ShowDebugUI() override {
		ImGui::Begin("Control Panel");
		ImGuiTabBarFlags tab_bar_flags = ImGuiBackendFlags_None;

		if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {

			if (ImGui::BeginTabItem("Camera")) {
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				if (Settings.EnableGhostMode) {
					first_camera->ShowDebugUI("Person Person Camera");
				} else {
					third_camera->ShowDebugUI("Third Person Camera");
					ImGui::BulletText("Distance: %.2f", third_camera->GetDistance());
				}
				if (ImGui::Button("Reset Position")) {
					first_camera->SetPosition(glm::vec3(0.0f, 3.0f, 5.0f));
					first_camera->SetPitch(0.0f);
					first_camera->SetYaw(0.0f);
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Projection")) {

				ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), (ProjectionSettings.IsPerspective) ? "Perspective Projection" : "Orthogonal Projection");
				ImGui::Text("Parameters");
				ImGui::BulletText("FoV = %.2f deg, Aspect = %.2f", Settings.EnableGhostMode ? first_camera->GetFOV() : third_camera->GetFOV(), ProjectionSettings.Aspect);
				if (!ProjectionSettings.IsPerspective) {
					ImGui::SliderFloat("Length", &ProjectionSettings.OrthogonalHeight, 1.0f, 100.0f);
				}
				ImGui::BulletText("left: %.2f, right: %.2f ", view_volume->ClippingParameters[2], view_volume->ClippingParameters[3]);
				ImGui::BulletText("bottom: %.2f, top: %.2f ", view_volume->ClippingParameters[0], view_volume->ClippingParameters[1]);
				ImGui::DragFloatRange2("Near & Far", &ProjectionSettings.ClippingNear, &ProjectionSettings.ClippingFar, 0.1f, 0.1f, 500.0f);
				ImGui::Spacing();

				if (ImGui::TreeNode("Projection Matrix")) {
					SetProjectionMatrix(Nexus::DISPLAY_MODE_DEFAULT);
					glm::mat4 proj = projection;

					ImGui::Columns(4, "mycolumns");
					ImGui::Separator();
					for (int i = 0; i < 4; i++) {
						ImGui::Text("%.2f", proj[0][i]); ImGui::NextColumn();
						ImGui::Text("%.2f", proj[1][i]); ImGui::NextColumn();
						ImGui::Text("%.2f", proj[2][i]); ImGui::NextColumn();
						ImGui::Text("%.2f", proj[3][i]); ImGui::NextColumn();
						ImGui::Separator();
						
					}
					ImGui::Columns(1);

					ImGui::TreePop();
				}
				ImGui::Spacing();
				ImGui::EndTabItem();
			}

            if (ImGui::BeginTabItem("Light Setting")) {
                // 新增 Light Position
                ImGui::SliderFloat3("Light Color", point_light->GetDiffusePointer(), 0.0f, 1.0f);

                ImGui::EndTabItem();
            }

			if (ImGui::BeginTabItem("Illustration")) {
				ImGui::Text("Current Screen: %d", Settings.CurrentDisplyMode);
				ImGui::Text("Showing Axes: %s", Settings.ShowOriginAnd3Axes ? "True" : "false");
				ImGui::Checkbox("Face Culling", &Settings.EnableFaceCulling);
				if (ImGui::BeginCombo("Culling Type", Settings.CullingTypeStr.c_str())) {
					for (int n = 0; n < Settings.CullingTypes.size(); n++) {
						bool is_selected = (Settings.CullingTypeStr == Settings.CullingTypes[n]);
						if (ImGui::Selectable(Settings.CullingTypes[n].c_str(), is_selected)) {
							Settings.CullingTypeStr = Settings.CullingTypes[n];
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				// ImGui::Checkbox("Enable Ball Culling", &enalbe_ball_culling);
				// ImGui::Text("Full Screen:  %s", isfullscreen ? "True" : "false");
				ImGui::Spacing();

				ImGui::EndTabItem();
			}
			
			ImGui::EndTabBar();
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::End();
	}

	void DrawOriginAnd3Axes(Nexus::Shader* shader) const {
		shader->SetBool("material.enableDiffuseTexture", false);
		shader->SetBool("material.enableSpecularTexture", false);
		shader->SetBool("material.enableEmission", true);
		shader->SetBool("material.enableEmissionTexture", false);
		
		// Draw the origin (0, 0, 0)
		model->Push();
		model->Save(glm::scale(model->Top(), glm::vec3(0.1f, 0.1f, 0.1f)));
		shader->SetVec4("material.ambient", glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
		shader->SetVec4("material.specular", glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
		shader->SetFloat("material.shininess", 64.0f);
		sphere->Draw(shader, model->Top());
		model->Pop();

		// Draw x, y ,z axes.
		model->Push();
		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(5.0f, 0.0f, 0.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(10.0f, 0.05f, 0.05f)));
		shader->SetVec4("material.ambient", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(1.0f, 0.0f, 0.0f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(1.0f, 0.0f, 0.0f, 1.0));
		shader->SetFloat("material.shininess", 64.0f);
		cube->Draw(shader, model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 5.0f, 0.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(0.05f, 10.0f, 0.05f)));
		shader->SetVec4("material.ambient", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(0.0f, 1.0f, 0.0f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(0.0f, 1.0f, 0.0f, 1.0));
		shader->SetFloat("material.shininess", 64.0f);
		cube->Draw(shader, model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 0.0f, 5.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(0.05f, 0.05f, 10.0f)));
		shader->SetVec4("material.ambient", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
		shader->SetVec4("material.diffuse", glm::vec4(0.0f, 0.0f, 1.0f, 1.0));
		shader->SetVec4("material.specular", glm::vec4(0.0f, 0.0f, 1.0f, 1.0));
		shader->SetFloat("material.shininess", 64.0f);
		cube->Draw(shader, model->Top());
		model->Pop();
		model->Pop();
	}

	void SetViewMatrix(Nexus::DisplayMode monitor_type) {
		glm::vec3 camera_position = Settings.EnableGhostMode ? first_camera->GetPosition() : third_camera->GetPosition();
		switch (monitor_type) {
			case Nexus::DISPLAY_MODE_ORTHOGONAL_X:
				view = glm::lookAt(glm::vec3(10.0f, 10.0f, 0.0f), glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
				break;
			case Nexus::DISPLAY_MODE_ORTHOGONAL_Y:
				view = glm::lookAt(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0, 0.0, -1.0));
				break;
			case Nexus::DISPLAY_MODE_ORTHOGONAL_Z:
				view = glm::lookAt(glm::vec3(0.0f, 10.0f, 10.0f), glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
				break;
			case Nexus::DISPLAY_MODE_DEFAULT:
				view = Settings.EnableGhostMode ? first_camera->GetViewMatrix() : third_camera->GetViewMatrix();
				break;
		}
	}

	void SetProjectionMatrix(Nexus::DisplayMode monitor_type) {
		ProjectionSettings.Aspect = (float)Settings.Width / (float)Settings.Height;

		if (monitor_type == Nexus::DISPLAY_MODE_DEFAULT) {
			if (ProjectionSettings.IsPerspective) {
				projection = GetPerspectiveProjMatrix(glm::radians(Settings.EnableGhostMode ? first_camera->GetFOV() : third_camera->GetFOV()), ProjectionSettings.Aspect, ProjectionSettings.ClippingNear, ProjectionSettings.ClippingFar);
			} else {
				projection = GetOrthoProjMatrix(-ProjectionSettings.OrthogonalHeight * ProjectionSettings.Aspect, ProjectionSettings.OrthogonalHeight * ProjectionSettings.Aspect, -ProjectionSettings.OrthogonalHeight, ProjectionSettings.OrthogonalHeight, ProjectionSettings.ClippingNear, ProjectionSettings.ClippingFar);
			}
		} else {
			projection = GetOrthoProjMatrix(-11.0 * ProjectionSettings.Aspect, 11.0 * ProjectionSettings.Aspect, -11.0, 11.0, 0.01f, 500.0f);
		}
	}

	void SetViewport(Nexus::DisplayMode monitor_type) override {
		if (Settings.CurrentDisplyMode == Nexus::DISPLAY_MODE_3O1P) {
			switch (monitor_type) {
				case Nexus::DISPLAY_MODE_ORTHOGONAL_X:
					glViewport(0, Settings.Height / 2, Settings.Width / 2, Settings.Height / 2);
					break;
				case Nexus::DISPLAY_MODE_ORTHOGONAL_Y:
					glViewport(Settings.Width / 2, Settings.Height / 2, Settings.Width / 2, Settings.Height / 2);
					break;
				case Nexus::DISPLAY_MODE_ORTHOGONAL_Z:
					glViewport(0, 0, Settings.Width / 2, Settings.Height / 2);
					break;
				case Nexus::DISPLAY_MODE_DEFAULT:
					glViewport(Settings.Width / 2, 0, Settings.Width / 2, Settings.Height / 2);
					break;
			}
		} else {
			glViewport(0, 0, Settings.Width, Settings.Height);
		}
	}
	
	void OnWindowResize() override {
		ProjectionSettings.Aspect = (float)Settings.Width / (float)Settings.Height;
	}
	
	void OnProcessInput(int key) override {
		if (Settings.EnableGhostMode) {
			if (key == GLFW_KEY_W) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_FORWARD, DeltaTime);
			}
			if (key == GLFW_KEY_S) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_BACKWARD, DeltaTime);
			}
			if (key == GLFW_KEY_A) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_LEFT, DeltaTime);
			}
			if (key == GLFW_KEY_D) {
				first_camera->ProcessKeyboard(Nexus::CAMERA_RIGHT, DeltaTime);
			}
		}
	}
	
	void OnKeyPress(int key) override {
		if (key == GLFW_KEY_LEFT_SHIFT) {
			if (Settings.EnableGhostMode) {
				first_camera->SetMovementSpeed(50.0f);
			}
		}

		if (key == GLFW_KEY_X) {
			if (Settings.ShowOriginAnd3Axes) {
				Settings.ShowOriginAnd3Axes = false;
				Nexus::Logger::Message(Nexus::LOG_INFO, "World coordinate origin and 3 axes: [Hide].");
			} else {
				Settings.ShowOriginAnd3Axes = true;
				Nexus::Logger::Message(Nexus::LOG_INFO, "World coordinate origin and 3 axes: [Show].");
			}
		}

		if (key == GLFW_KEY_P) {
			if (ProjectionSettings.IsPerspective) {
				ProjectionSettings.IsPerspective = false;
				Nexus::Logger::Message(Nexus::LOG_INFO, "Projection Mode: Orthogonal");
			} else {
				ProjectionSettings.IsPerspective = true;
				Nexus::Logger::Message(Nexus::LOG_INFO, "Projection Mode: Perspective");
			}
		}

		if (key == GLFW_KEY_G) {
			if (Settings.EnableGhostMode) {
				Settings.EnableGhostMode = false;
				Nexus::Logger::Message(Nexus::LOG_INFO, "Camera Mode: Third Person");
			} else {
				Settings.EnableGhostMode = true;
				Nexus::Logger::Message(Nexus::LOG_INFO, "Camera Mode: First Person");
			}
		}

		// 分鏡切換
		if (key == GLFW_KEY_1) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_ORTHOGONAL_X;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Orthogonal X.");
		}
		if (key == GLFW_KEY_2) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_ORTHOGONAL_Y;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Orthogonal Y.");
		}
		if (key == GLFW_KEY_3) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_ORTHOGONAL_Z;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Orthogonal Z.");
		}
		if (key == GLFW_KEY_4) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_DEFAULT;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to Default Camera.");
		}
		if (key == GLFW_KEY_5) {
			Settings.CurrentDisplyMode = Nexus::DISPLAY_MODE_3O1P;
			Nexus::Logger::Message(Nexus::LOG_INFO, "Switch to All Screen.");
		}
	}
	
	void OnKeyRelease(int key) override {
		if (key == GLFW_KEY_LEFT_SHIFT) {
			if (Settings.EnableGhostMode) {
				first_camera->SetMovementSpeed(10.0f);
			}
		}
	}
	
	void OnMouseMove(int xoffset, int yoffset) override {
		if (!Settings.EnableCursor) {
			if (Settings.EnableGhostMode) {
				first_camera->ProcessMouseMovement(xoffset, yoffset);
			} else {
				third_camera->ProcessMouseMovement(xoffset, yoffset);
			}
		}
	}
	
	void OnMouseButtonPress(int button) override {
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			SetCursorDisable(true);
			Settings.EnableCursor = false;
		} 
	}
	
	void OnMouseButtonRelease(int button) override {
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			SetCursorDisable(false);
			Settings.EnableCursor = true;
		}
	}
	
	void OnMouseScroll(int yoffset) override {
		if (ProjectionSettings.IsPerspective) {
			if (Settings.EnableGhostMode) {
				first_camera->ProcessMouseScroll(yoffset);
			} else {
				third_camera->AdjustDistance(yoffset, 1.0f, 20.0f, 1.0f);
			}
		} else {
			AdjustOrthogonalProjectionWidth(yoffset);
		}
	}

	void AdjustOrthogonalProjectionWidth(float yoffset) {
		if (ProjectionSettings.OrthogonalHeight >= 1.0f && ProjectionSettings.OrthogonalHeight <= 100.0f) {
			ProjectionSettings.OrthogonalHeight -= (float)yoffset * 1.0f;
		}
		if (ProjectionSettings.OrthogonalHeight < 1.0f) {
			ProjectionSettings.OrthogonalHeight = 1.0f;
		}
		if (ProjectionSettings.OrthogonalHeight > 100.0f) {
			ProjectionSettings.OrthogonalHeight = 100.0f;
		}
	}
	
private:
	std::unique_ptr<Nexus::Shader> myShader = nullptr;
	std::unique_ptr<Nexus::Shader> normalShader = nullptr;
	std::unique_ptr<Nexus::Shader> simpleDepthShader = nullptr;
	std::unique_ptr<Nexus::Shader> debugDepthQuad = nullptr;
	
	std::unique_ptr<Nexus::FirstPersonCamera> first_camera = nullptr;
	std::unique_ptr<Nexus::ThirdPersonCamera> third_camera = nullptr;
	
	std::unique_ptr<Nexus::MatrixStack> model = nullptr;
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	std::unique_ptr<Nexus::Rectangle> floor = nullptr;
	std::unique_ptr<Nexus::Cube> cube = nullptr;
	std::unique_ptr<Nexus::Sphere> sphere = nullptr;
	std::unique_ptr<Nexus::ViewVolume> view_volume = nullptr;

	std::unique_ptr<Nexus::Texture2D> texture_checkerboard = nullptr;

    std::unique_ptr<Nexus::PointLight> point_light;
};

int main() {
	NexusDemo app;
	return app.Run();
}