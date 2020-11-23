#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>

#include <hdvw/window.hpp>

namespace hd {
    struct CameraCreateInfo {
        Window window;
    };

    class Camera_t;
    typedef std::shared_ptr<Camera_t> Camera;

    class Camera_t {
        public:
            Camera_t(CameraCreateInfo ci) : window(ci.window->raw()) { 
                uint32_t width, height;
                ci.window->getFramebufferSize(width, height);
                lastX = width / 2;
                lastY = height / 2;
                pitch = -glm::pi<float>();
                yaw = 0;
                update(0);
            }

            static Camera conjure(CameraCreateInfo ci) {
                return std::make_shared<Camera_t>(ci);
            }

            ~Camera_t() { }

            void update(float _deltaTime) {
                deltaTime = _deltaTime;
                int _width, _height;
                glfwGetFramebufferSize(window, &_width, &_height);

                glm::quat quat = glm::quat(glm::vec3(pitch, yaw, roll));
                glm::quat quatI = glm::quat(glm::vec3(-pitch, yaw, -roll));
                pitch = yaw = roll = 0;

                camera = quat * camera;
                camera = glm::normalize(camera);
                glm::mat4 rotate = glm::mat4_cast(camera);
                
                cameraI = quatI * cameraI;
                cameraI = glm::normalize(cameraI);
                glm::mat4 rotateI = glm::mat4_cast(cameraI);

                glm::mat4 translateToPos = glm::translate(glm::mat4(1.0f), -cameraPos);
                view = translateToPos * rotate * glm::mat4(1.0f);

                glm::mat4 translateToPosI = glm::translate(glm::mat4(1.0f), -(cameraPos - distance(cameraPos)));
                viewI = translateToPosI * rotateI * glm::mat4(1.0f);

                proj = glm::perspective(glm::radians(fov), (float) _width / (float) _height, 0.1f, 100.0f);
                projI = glm::inverse(proj);
            }

            void processInput() {
                glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
                glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
                glm::vec3 cameraFront = glm::vec3(view[0][2], view[1][2], view[2][2]);
                glm::vec3 absoluteUp = glm::vec3(0.0f, 1.0f, 0.0f);

                const float cameraSpeed = 2.5f * deltaTime;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    cameraPos += cameraSpeed * cameraFront;
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    cameraPos -= cameraSpeed * cameraFront;
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    cameraPos += cameraRight * cameraSpeed;
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    cameraPos -= cameraRight * cameraSpeed;
                if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
                    pitch += cameraSpeed;
                if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
                    pitch -= cameraSpeed;
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                    cameraPos += absoluteUp * cameraSpeed;
                if (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS)
                    cameraPos -= absoluteUp * cameraSpeed;
                if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    glfwSetCursorPosCallback(window, nullptr);
                }
                if ((glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) && (gridHold != true)) {
                    gridHold = true;
                    gridMode = true;
                } else gridMode = false;
                
                if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
                    gridHold = false;
                }

                if (mousePressed) {
                    if (glm::abs(cameraFront.y) > 0.00001f) {
                        float t = (1 - cameraPos.y + 1) / cameraFront.y;
                        mousePosition.x = cameraPos.x + cameraFront.x * t;
                        mousePosition.x *= -1;
                        mousePosition.y = cameraPos.z + cameraFront.z * t;

                        if ((glm::abs(mousePosition.x)) < 6 && (glm::abs(mousePosition.y + 2.0f) < 5)) {
                            mousePressed = true;
                            mousePosition.x = (mousePosition.x + 5.0f) / 10.0f;
                            mousePosition.y = (mousePosition.y + 5.0f) / 10.0f;
                        }
                    }
                }
            }

            glm::vec3 distance(glm::vec3& _cameraPos) {
                return glm::vec3(0.0f, glm::abs(2 * (_cameraPos.y - 1.0f)), 0.0f);
            }

            void processMouse(double xpos, double ypos) {
                if(firstMouse) {
                    lastX = xpos;
                    lastY = ypos;
                    firstMouse = false;
                }

                float xoffset = xpos - lastX;
                float yoffset = lastY - ypos;
                lastX = xpos;
                lastY = ypos;

                const float sensitivity = 0.5f;
                xoffset *= sensitivity;
                yoffset *= sensitivity;

                roll -= yoffset * deltaTime;
                yaw += xoffset * deltaTime;
            }

            void mouseButton(int button, int action) {
                if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
                    mousePressed = true;
                else mousePressed = false;
            }

            glm::mat4 view;
            glm::mat4 viewI;
            glm::mat4 proj;
            glm::mat4 projI;
            glm::vec3 cameraPos = glm::vec3(6.0f, 4.f, -1.0f);

            bool gridMode = false;
            bool mousePressed = false;
            bool gridHold = false;

            glm::vec2 mousePosition = glm::vec2(0.0f, 0.0f);

        private:
            GLFWwindow *window;
            float deltaTime;

            float fov = 90.0f;
            float firstMouse = true;
            double lastX, lastY;

            float yaw = 0.0f;
            float pitch = 0.0f;
            float roll = 0.0f;

            glm::quat camera = glm::quat(glm::vec3(pitch, yaw, roll));
            glm::quat cameraI = glm::quat(glm::vec3(pitch, yaw, roll));
    };
}
