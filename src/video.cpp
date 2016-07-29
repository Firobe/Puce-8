#include "video.h"
#include "model2d.h"
#include "keymanager.h"

#include <stdexcept>
#include <iostream>
#include <sstream>

int screen_width = 640;
int screen_height = 320;

using namespace std;
enum Axes { xAxis, yAxis, zAxis };

static void error_callback(int error, const char* description) {
    cout << description << endl;
    }

Video::Video(string a, string b) {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        throw runtime_error("Unable to initialize GLFW");

	int min, maj, rev;
	glfwGetVersion(&maj, &min, &rev);
	cout << "GLFW version is " << maj << "." << min << "." << rev << endl;
    glfwWindowHint(GLFW_SAMPLES, 4); //Anti-aliasing

    if (FULLSCREEN) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		screen_width = mode->width;
		screen_height = mode->height;
        _window = glfwCreateWindow(screen_width, screen_height, "BeatRacer", glfwGetPrimaryMonitor(), nullptr);
        }
    else
        _window = glfwCreateWindow(screen_width, screen_height, "BeatRacer", nullptr, nullptr);

    if (!_window)
        throw runtime_error("Unable to create window");

    glfwMakeContextCurrent(_window);
#ifdef _WIN32
    cout << "Windows patch enabled !" << endl;
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
        throw ("Unable to initialize GLEW");

#endif
    glfwSwapInterval(VERTICAL_SYNC);
    glfwSetKeyCallback(_window, KeyManager::keyCallback);
	glfwSetCursorPosCallback(_window, KeyManager::cursorPosCallback);
	glfwSetMouseButtonCallback(_window, KeyManager::mouseButtonCallback);
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    _shaderArray.emplace_back(a, b);
    _shaderArray[0].load();
    _projection = glm::perspective(FOV, (double) screen_width / screen_height, NEAR, FAR);
    glEnable(GL_MULTISAMPLE);
	cout << "OpenGL context version is " << (unsigned char*)glGetString(GL_VERSION) << endl;
    }

Video::~Video() {
    glfwDestroyWindow(_window);
    glfwTerminate();
    }

void Video::refresh() {
    GLenum err;

    if ((err = glGetError()) != GL_NO_ERROR) {
        stringstream ss;
        ss << "0x" << std::hex << err;
        throw runtime_error("OpenGL issue " + ss.str());
        }
    glfwSwapBuffers(_window);
    }

GLFWwindow* Video::win() {
    return _window;
    }
void Video::addShader(string a, string b) {
    _shaderArray.emplace_back(a, b);
    }

void Video::render2D(GLuint id, int size, Texture& tex, Model2D* mod, glm::dmat4 model, int shaderNb) {
    if ((unsigned int)shaderNb >= _shaderArray.size())
        throw runtime_error("Unknown shader");

    glDisable(GL_DEPTH_TEST);
    glUseProgram(_shaderArray[shaderNb].getProgramID());
    mod->uniformize(_shaderArray[shaderNb].getProgramID());
    glm::mat4 modViewProj(model);
    glUniformMatrix4fv(glGetUniformLocation(_shaderArray[shaderNb].getProgramID(), "modViewProj"), 1, GL_FALSE, glm::value_ptr(modViewProj));
    glBindVertexArray(id);

    if (!tex.empty())
        glBindTexture(GL_TEXTURE_2D, tex.getID());

    glDrawArrays(GL_TRIANGLES, 0, size);

    if (!tex.empty())
        glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
    }

glm::dvec3 toCartesian(glm::dvec3 v) {
    glm::dvec3 res(0.);
    res[0] = v[0] * cos(v[1]) * sin(v[2] + glm::half_pi<double>());
    res[1] = v[0] * sin(v[1]) * sin(v[2] + glm::half_pi<double>());
    res[2] = v[0] * cos(v[2] + glm::half_pi<double>());
    return res;
    }

glm::dvec3 toSpherical(glm::dvec3 v) {
    glm::dvec3 res(0.);
    res[0] = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    res[1] = atan(v.y / v.x); //THETA
    res[2] = (acos(v.z / res[0]) - glm::half_pi<double>()); //PHI
    return res;
    }
