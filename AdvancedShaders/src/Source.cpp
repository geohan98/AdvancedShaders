#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Shader.h"
#include "Camera.h"

#include <Model.h>
#include "Terrain.h"

#include<string>
#include <iostream>
#include <numeric>

//Window Settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;
//Functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* path);
void setVAO(vector <float> vertices);
//Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
//Camera
Camera camera(glm::vec3(135, 30, -150));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
//Lighting
glm::vec3 lightPos = glm::vec3(1000.0, 1000.0, 1000.0);
glm::vec3 lightColour = glm::vec3(1.0, 1.0, 1.0);
float ambientStrength = 0.4;
float diffuseStrength = 1.0;
float specularStrength = 1.0;
//Skybox
unsigned int skyboxTex;
std::string skyboxPath = "..\\resources\\textures\\skybox\\";
std::vector<std::string> faces =
{
	"right.jpg",
	"left.jpg",
	"top.jpg",
	"bottom.jpg",
	"front.jpg",
	"back.jpg"
};
//Water
unsigned int waterHeightMap;
unsigned int waterHeightMap2;
float waterScale = 25.0;
float waveHeight = 8.0;
float waterSpeedX = -0.1;
float waterSpeedY = 0.1;
float waterHeightTile = 5.0;
float waterHeightTile2 = 5.0;
//Terrain
unsigned int terrainHeightMap;
unsigned int terrainHeightMap2;
float terrainHeightScale = 100.0;
float terrainHeightTile = 1.0;
float terrainHeightTile2 = 1.0;

unsigned int VAO, VBO;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "IMAT3907", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);


	//Skybox
	Model skyboxModel = Model("..\\resources\\cube.obj");
	Shader skyboxShader("..\\shaders\\skybox\\skybox.vs", "..\\shaders\\skybox\\skybox.fs");
	glGenTextures(1, &skyboxTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
	int width, height, channels;
	for (int i = 0; i < 6; i++)
	{
		unsigned char* data = stbi_load((skyboxPath + faces[i]).c_str(), &width, &height, &channels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Failed to Load Texture" << (skyboxPath + faces[i]).c_str() << std::endl;
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	//Water
	Model waterModel = Model("..\\resources\\hiPolyPlane.obj");
	Shader waterShader = Shader("..\\shaders\\water\\water.vs", "..\\shaders\\water\\water.fs", "..\\shaders\\water\\water.gs", "..\\shaders\\water\\water.tcs", "..\\shaders\\water\\water.tes");
	waterHeightMap = loadTexture("..\\resources\\textures\\water\\height.jpg");
	waterHeightMap2 = loadTexture("..\\resources\\textures\\water\\height2.jpg");
	//Terrain
	Terrain terrain = Terrain(251, 251, 10);
	std::vector<float> vertices = terrain.getVertices();
	setVAO(vertices);
	Shader terrainShader = Shader("..\\shaders\\terrain\\terrain.vs", "..\\shaders\\terrain\\terrain.fs", "..\\shaders\\terrain\\terrain.gs", "..\\shaders\\terrain\\terrain.tcs", "..\\shaders\\terrain\\terrain.tes");
	terrainHeightMap = loadTexture("..\\resources\\textures\\Terrain\\height.jpg");
	terrainHeightMap2 = loadTexture("..\\resources\\textures\\Terrain\\height2.jpg");

	while (!glfwWindowShouldClose(window))
	{
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 5000.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));

		//Timing
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		//Input
		processInput(window);
		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
			camera.printCameraCoords();
		//Clear Window
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		//Skybox
		glDepthMask(GL_FALSE);
		skyboxShader.use();
		skyboxShader.setMat4("u_view", skyboxView);
		skyboxShader.setMat4("u_projection", projection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
		skyboxModel.Draw(skyboxShader);
		glDepthMask(GL_TRUE);
		//Water

		glm::mat4 waterModelMat = glm::scale(glm::mat4(1.0), glm::vec3(waterScale, 1, waterScale));
		waterShader.use();
		waterShader.setMat4("u_model", waterModelMat);
		waterShader.setMat4("u_view", view);
		waterShader.setMat4("u_projection", projection);
		waterShader.setFloat("u_ambientStrength", ambientStrength);
		waterShader.setFloat("u_diffuseStrength", diffuseStrength);
		waterShader.setFloat("u_specularStrength", specularStrength);
		waterShader.setVec3("u_lightPos", lightPos);
		waterShader.setVec3("u_lightColor", lightColour);
		waterShader.setVec3("u_viewPos", camera.Position);
		waterShader.setFloat("u_waveHeight", waveHeight);
		waterShader.setFloat("u_time", glfwGetTime());
		waterShader.setFloat("u_speedX", waterSpeedX);
		waterShader.setFloat("u_speedY", waterSpeedY);
		waterShader.setInt("u_heightMap", 0);
		waterShader.setFloat("u_heightTile", waterHeightTile);
		waterShader.setInt("u_heightMap2", 1);
		waterShader.setFloat("u_heightTile2", waterHeightTile2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, waterHeightMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, waterHeightMap2);
		waterModel.DrawTess(waterShader);
		//Terrain
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glm::mat4 terrainModelMat = glm::translate(glm::mat4(1.0), glm::vec3(-1250, 60, -1250));
		terrainShader.use();
		terrainShader.setMat4("u_model", terrainModelMat);
		terrainShader.setMat4("u_view", view);
		terrainShader.setMat4("u_projection", projection);
		terrainShader.setFloat("u_ambientStrength", ambientStrength);
		terrainShader.setFloat("u_diffuseStrength", diffuseStrength);
		terrainShader.setFloat("u_specularStrength", specularStrength);
		terrainShader.setVec3("u_lightPos", lightPos);
		terrainShader.setVec3("u_lightColor", lightColour);
		terrainShader.setVec3("u_viewPos", camera.Position);
		terrainShader.setFloat("u_heightScale", terrainHeightScale);
		terrainShader.setInt("u_heightMap", 0);
		terrainShader.setFloat("u_heightTile", terrainHeightTile);
		terrainShader.setInt("u_heightMap2", 1);
		terrainShader.setFloat("u_heightTile2", terrainHeightTile2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, terrainHeightMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, terrainHeightMap2);
		glBindVertexArray(VAO);
		glDrawArrays(GL_PATCHES, 0, vertices.size() / 3);
		//Sawp Buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}


	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		stbi_image_free(data);
		std::cout << "Loaded texture at path: " << path << " width " << width << " id " << textureID << std::endl;

	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);

	}

	return textureID;
}


void setVAO(vector <float> vertices) {

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, (vertices.size() * sizeof(GLfloat)), vertices.data(), GL_STATIC_DRAW);

	//xyz
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//texture
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



