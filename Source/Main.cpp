#include "Application.h"
#include "Logger.h"
#include "FirsrPersonCamera.h"
#include "ThirdPersonCamera.h"
#include "Shader.h"
#include "MatrixStack.h"
#include "FileLoader.h"
#include "Texture2D.h"
#include "Light.h"

#include "Rectangle.h"
#include "Cube.h"
#include "Sphere.h"
#include "ViewVolume.h"
#include <random>

std::mt19937_64 rand_generator;

class NexusDemo final : public Nexus::Application {
public:
	NexusDemo() {
		Settings.Width = 800;
		Settings.Height = 600;
		Settings.WindowTitle = "Scientific Visualization | Project #5: High Dimensional Data Visualization";
		Settings.EnableDebugCallback = true;
		Settings.EnableFullScreen = false;

		Settings.EnableGhostMode = GL_FALSE;
		Settings.EnableFaceCulling = true;
		Settings.CullingTypeStr = "Back Face";
		Settings.ShowOriginAnd3Axes = GL_TRUE;

		// Projection Settings Initalize
		ProjectionSettings.IsPerspective = GL_FALSE;
		ProjectionSettings.OrthogonalHeight = 200.0f;
		ProjectionSettings.ClippingNear = 0.01f;
		ProjectionSettings.ClippingFar = 1000.0f;
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
		first_camera = std::make_unique<Nexus::FirstPersonCamera>(glm::vec3(0.0f, 0.0f, 500.0f));
		third_camera = std::make_unique<Nexus::ThirdPersonCamera>(glm::vec3(0.0f, 0.0f, 500.0f));
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

        // Loading High Dimension data
        Nexus::FileLoader::LoadHighDimensionData("Resource/Data/student.data", high_dims_data, data_count, data_dims);

        // compute the sum of every data
        data_sum = std::vector<GLfloat>(data_count, 0);
        for (GLuint i = 0; i < data_count; i++) {
            for (GLuint j = 0; j < data_dims; j++) {
                data_sum[i] += high_dims_data[i * 9 + j];
            }
        }

        // normalize the data (find the max and min)
        GLfloat sum_max = *max_element(std::begin(data_sum), std::end(data_sum));
        GLfloat sum_min = *min_element(std::begin(data_sum), std::end(data_sum));
        for (GLuint i = 0; i < data_count; i++) {
            data_sum[i] = (data_sum[i] - sum_min) / (sum_max - sum_min);
        }


        // Compute the original distance of the data
        original_distance = std::vector<GLfloat>(data_count * data_count, 0);
        projected_distance = std::vector<GLfloat>(data_count * data_count, 0);
        for (GLuint i = 0; i < data_count; i++) {
            for (GLuint j = i + 1; j < data_count; j++) {
                GLfloat temp = 0;
                for (GLuint n = 0; n < data_dims; n++) {
                    temp += pow(high_dims_data[i * 9 + n] - high_dims_data[j * 9 + n], 2);
                }
                GLfloat distance = sqrt(temp);
                original_distance[i * data_count + j] = distance;
            }
        }

        // Initialize the Q's position (2D)
        GLfloat distance_max = *max_element(std::begin(original_distance), std::end(original_distance));
        std::uniform_real_distribution<GLfloat> initialize_position(0, sqrt(distance_max));
        for (GLuint i = 0; i < data_count; i++) {
            projected_data.push_back(glm::vec3(initialize_position(rand_generator), initialize_position(rand_generator), 0.0f));
        }

	}

	void Update() override {
	    // Setting camera
	    third_camera->SetTarget(glm::vec3(0.0f));

        // Start iteration
        if (is_training_start) {
            if (iteration < iteration_max) {
                for (GLuint i = 0; i < data_count; i++) {
                    for (GLuint j = i + 1; j < data_count; j++) {

                        GLfloat new_distance = glm::distance(projected_data[i], projected_data[j]);
                        if (new_distance <= 1e-5) new_distance = 1e-3;

                        glm::vec3 delta_Qi = lambda * ((original_distance[i * data_count + j] - new_distance) / new_distance) * (projected_data[i] - projected_data[j]);
                        glm::vec3 delta_Qj = -delta_Qi;
                        projected_data[i] = projected_data[i] + delta_Qi;
                        projected_data[j] = projected_data[j] + delta_Qj;
                    }
                }
                lambda = lambda * alpha;
                iteration += 1;
            } else {
                is_training_start = GL_FALSE;
                is_training_finished = GL_TRUE;
            }
        }
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

        SetViewMatrix(Nexus::DISPLAY_MODE_DEFAULT);
        SetProjectionMatrix(Nexus::DISPLAY_MODE_DEFAULT);
        SetViewport(Nexus::DISPLAY_MODE_DEFAULT);

		myShader->Use();
        myShader->SetMat4("view", view);
        myShader->SetMat4("projection", projection);

		// ==================== Draw origin and 3 axes ====================
		if (Settings.ShowOriginAnd3Axes) {
			this->DrawOriginAnd3Axes(myShader.get());
		}

		// ==================== Draw the projected data on the 2D space ====================
		if (is_training_start || is_training_finished) {
            for (GLuint i = 0; i < projected_data.size(); i++) {
                model->Push();
                model->Save(glm::translate(model->Top(), projected_data[i]));
                model->Save(glm::scale(model->Top(), glm::vec3(2.0f, 2.0f, 2.0f)));
                myShader->SetVec3("objectColor", glm::vec3(0.8, data_sum[i], 0.6));
                sphere->Draw(myShader.get(), model->Top());
                model->Pop();
            }
		}

		// third_camera->SetTarget(balls[0].GetPosition());
		// ImGui::ShowDemoWindow();
	}

	void ShowDebugUI() override {

        ImGui::Begin("Sammon Mapping");
        ImGui::Text("Iteration: %d / %d", iteration, iteration_max);
        if (!is_training_start) {
            ImGui::InputText("Max Iteration", iteration_max_string, IM_ARRAYSIZE(iteration_max_string));
            ImGui::SliderFloat("Learning rate", &lambda, 0.0f, 1.0f, "%.8f");
            ImGui::SliderFloat("Decay", &alpha, 0.0f, 1.0f, "%.3f");
            if (ImGui::Button("Start Training")) {
                iteration_max = static_cast<GLuint>(std::stoi(std::string(iteration_max_string)));
                is_training_start = GL_TRUE;
                is_training_finished = GL_FALSE;
            }
            ImGui::SameLine();
            if (!is_training_start && is_training_finished) {
                if (ImGui::Button("Reset Parameters")) {
                    lambda = 1.0f;
                    alpha = 0.999f;
                    iteration = 0;
                }
            }
        } else {
            ImGui::Text("Learning rate: %.8f", lambda);
            ImGui::Text("Decay: %.3f", alpha);
            if (ImGui::Button("Stop")) {
                is_training_start = GL_FALSE;
                is_training_finished = GL_TRUE;
            }
        }
        ImGui::End();

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
					third_camera->SetPosition(glm::vec3(0.0f, 0.0f, 500.0f));
                    third_camera->SetPitch(0.0f);
                    third_camera->SetYaw(0.0f);
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Projection")) {

				ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), (ProjectionSettings.IsPerspective) ? "Perspective Projection" : "Orthogonal Projection");
				ImGui::Text("Parameters");
				ImGui::BulletText("FoV = %.2f deg, Aspect = %.2f", Settings.EnableGhostMode ? first_camera->GetFOV() : third_camera->GetFOV(), ProjectionSettings.Aspect);
				if (!ProjectionSettings.IsPerspective) {
					ImGui::SliderFloat("Length", &ProjectionSettings.OrthogonalHeight, 1.0f, 400.0f);
				}
				ImGui::DragFloatRange2("Near & Far", &ProjectionSettings.ClippingNear, &ProjectionSettings.ClippingFar, 0.01f, 0.01f, 1000.0f);
				ImGui::Spacing();
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
		// Draw the origin (0, 0, 0)
		model->Push();
		model->Save(glm::scale(model->Top(), glm::vec3(4.0f, 4.0f, 4.0f)));
        shader->SetVec3("objectColor", glm::vec3(0.6, 0.6, 0.6));
		sphere->Draw(shader, model->Top());
		model->Pop();

		// Draw x, y ,z axes.
		model->Push();
		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(100.0f, 0.0f, 0.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(200.0f, 1.0f, 1.0f)));
        shader->SetVec3("objectColor", glm::vec3(1.0, 0.0, 0.0));
		cube->Draw(shader, model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 100.0f, 0.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(1.0f, 200.0f, 1.0f)));
        shader->SetVec3("objectColor", glm::vec3(0.0, 1.0, 0.0));
		cube->Draw(shader, model->Top());
		model->Pop();

		model->Push();
		model->Save(glm::translate(model->Top(), glm::vec3(0.0f, 0.0f, 100.0f)));
		model->Save(glm::scale(model->Top(), glm::vec3(1.0f, 1.0f, 200.0f)));
        shader->SetVec3("objectColor", glm::vec3(0.0, 0.0, 1.0));
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
				view = glm::lookAt(glm::vec3(0.0f, 0.0f, 500.0f), glm::vec3(0.0f, 0.f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
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
			projection = GetOrthoProjMatrix(-200.0 * ProjectionSettings.Aspect, 200.0 * ProjectionSettings.Aspect, -200.0, 200.0, 0.01f, 1000.0f);
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

        /*
        if (key == GLFW_KEY_G) {

            if (Settings.EnableGhostMode) {
                Settings.EnableGhostMode = false;
                Nexus::Logger::Message(Nexus::LOG_INFO, "Camera Mode: Third Person");
            } else {
                Settings.EnableGhostMode = true;
                Nexus::Logger::Message(Nexus::LOG_INFO, "Camera Mode: First Person");
            }
		}
        */

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
		if (ProjectionSettings.OrthogonalHeight >= 1.0f && ProjectionSettings.OrthogonalHeight <= 500.0f) {
			ProjectionSettings.OrthogonalHeight -= (float)yoffset * 20.0f;
		}
		if (ProjectionSettings.OrthogonalHeight < 1.0f) {
			ProjectionSettings.OrthogonalHeight = 1.0f;
		}
		if (ProjectionSettings.OrthogonalHeight > 500.0f) {
			ProjectionSettings.OrthogonalHeight = 500.0f;
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

    GLboolean is_training_start = GL_FALSE;
    GLboolean is_training_finished = GL_FALSE;
    GLuint iteration = 0;
    char iteration_max_string[128] = "20000";
    GLuint iteration_max = 10000;
    GLfloat lambda = 0.8f;
    GLfloat alpha = 0.999f;
    GLuint data_count, data_dims;
    std::vector<GLfloat> high_dims_data;
    std::vector<glm::vec3> projected_data;
    std::vector<GLfloat> original_distance;
    std::vector<GLfloat> projected_distance;
    std::vector<GLfloat> data_sum;
};

int main() {
	NexusDemo app;
	return app.Run();
}