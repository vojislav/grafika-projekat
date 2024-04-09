#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadTexture(char const* path);
unsigned int loadCubemap(vector<std::string> faces);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool bloom = true;
bool bloomKeyPressed = false;
float exposure = 0.5f;
void renderQuad();

bool streetLampOn1 = true;
bool streetLampOn2 = true;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = true;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 modelPosition = glm::vec3(0.0f);
    float modelScale = 0.05f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(160.0f, 25.0f, -38.0f)) {}
};

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    programState = new ProgramState;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    glEnable(GL_DEPTH_TEST);

    Shader pointLightShader("resources/shaders/mainLightning.vs", "resources/shaders/mainLightning.fs");
    Shader platformShader("resources/shaders/grass.vs", "resources/shaders/grass.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader sunShader("resources/shaders/sun.vs", "resources/shaders/sun.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader bloomShader("resources/shaders/bloom.vs", "resources/shaders/bloom.fs");

    Model buildingModel("resources/objects/building2/Building.obj");
    buildingModel.SetShaderTextureNamePrefix("material.");
    Model sunModel("resources/objects/sun/sun.obj");
    sunModel.SetShaderTextureNamePrefix("material.");
    Model platformModel("resources/objects/platform/concrete.obj");
    platformModel.SetShaderTextureNamePrefix("material.");
    Model streetLampModel("resources/objects/streetlamp2/StreetLamp.obj");
    streetLampModel.SetShaderTextureNamePrefix("material.");

    glm::vec3 sunPosition = glm::vec3(0.0f, 65.0f, -90.0f);
    glm::vec3 streetLampPosition1 = glm::vec3(20.0, 0.0, -50.0);
    glm::vec3 streetLampPosition2 = glm::vec3(20.0, 0.0, 50.0);
    glm::vec3 streetLampLightOffset = glm::vec3(10.0, 20.0, 0.0);

    PointLight& sunPointLight = programState->pointLight;
    sunPointLight.ambient = glm::vec3(70.0);
    sunPointLight.diffuse = glm::vec3(5.0f);
    sunPointLight.specular = glm::vec3(1.0f);
    sunPointLight.constant = 1.0f;
    sunPointLight.linear = 0.09f;
    sunPointLight.quadratic = 0.032f;

    PointLight streetLampPointLight;
    streetLampPointLight.ambient = glm::vec3(5.0f);
    streetLampPointLight.diffuse = glm::vec3(10.0f);
    streetLampPointLight.specular = glm::vec3(1.0f);
    streetLampPointLight.constant = 1.0f;
    streetLampPointLight.linear = 0.09f;
    streetLampPointLight.quadratic = 0.032f;

    float transparentVertices[] = {
        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
        1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    unsigned int crackTex = loadTexture(FileSystem::getPath("resources/textures/crack3.png").c_str());
    unsigned int sunflowerTex = loadTexture(FileSystem::getPath("resources/textures/sunflower.png").c_str());

    unsigned int grassVAO, grassVBO;
    glGenVertexArrays(1, &grassVAO);
    glGenBuffers(1, &grassVBO);
    glBindVertexArray(grassVAO);
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), &transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    //right - px
    //left - nx
    //top - py
    //bottom - ny
    //front - pz
    //back - nz
    vector<std::string> faces = {
        "resources/cubemaps/cloudy2/px.png",
        "resources/cubemaps/cloudy2/nx.png",
        "resources/cubemaps/cloudy2/py2.png",
        "resources/cubemaps/cloudy2/ny.png",
        "resources/cubemaps/cloudy2/pz.png",
        "resources/cubemaps/cloudy2/nz.png"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    blurShader.use();
    blurShader.setInt("image", 0);
    bloomShader.use();
    bloomShader.setInt("scene", 0);
    bloomShader.setInt("bloomBlur", 1);

    srand(glfwGetTime());
    const int streetLampOnPercent = 2;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        streetLampOn1 = rand() % 100 > streetLampOnPercent;
        streetLampOn2 = rand() % 100 > streetLampOnPercent;

        processInput(window);

        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 1200.0f);
        pointLightShader.use();
        glEnable(GL_CULL_FACE);

        pointLightShader.setVec3("pointLights[0].position", sunPosition);
        pointLightShader.setVec3("pointLights[0].ambient", sunPointLight.ambient);
        pointLightShader.setVec3("pointLights[0].diffuse", sunPointLight.diffuse);
        pointLightShader.setVec3("pointLights[0].specular", sunPointLight.specular);
        pointLightShader.setFloat("pointLights[0].constant", sunPointLight.constant);
        pointLightShader.setFloat("pointLights[0].linear", sunPointLight.linear);
        pointLightShader.setFloat("pointLights[0].quadratic", sunPointLight.quadratic);

        pointLightShader.setVec3("pointLights[1].position", streetLampPosition1 + streetLampLightOffset);
        pointLightShader.setVec3("pointLights[1].ambient", streetLampPointLight.ambient);
        pointLightShader.setVec3("pointLights[1].diffuse", streetLampPointLight.diffuse);
        pointLightShader.setVec3("pointLights[1].specular", streetLampPointLight.specular);
        pointLightShader.setFloat("pointLights[1].constant", streetLampPointLight.constant);
        pointLightShader.setFloat("pointLights[1].linear", streetLampPointLight.linear);
        pointLightShader.setFloat("pointLights[1].quadratic", streetLampPointLight.quadratic);

        pointLightShader.setVec3("pointLights[2].position", streetLampPosition2 + streetLampLightOffset);
        pointLightShader.setVec3("pointLights[2].ambient", streetLampPointLight.ambient);
        pointLightShader.setVec3("pointLights[2].diffuse", streetLampPointLight.diffuse);
        pointLightShader.setVec3("pointLights[2].specular", streetLampPointLight.specular);
        pointLightShader.setFloat("pointLights[2].constant", streetLampPointLight.constant);
        pointLightShader.setFloat("pointLights[2].linear", streetLampPointLight.linear);
        pointLightShader.setFloat("pointLights[2].quadratic", streetLampPointLight.quadratic);

        pointLightShader.setVec3("viewPosition", programState->camera.Position);
        pointLightShader.setFloat("material.shininess", 32.0f);
        pointLightShader.setMat4("projection", projection);
        pointLightShader.setMat4("view", view);
        pointLightShader.setBool("streetLampOn1", streetLampOn1);
        pointLightShader.setBool("streetLampOn2", streetLampOn2);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, programState->modelPosition);
        model = glm::scale(model, glm::vec3(programState->modelScale));
//        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        pointLightShader.setMat4("model", model);
        buildingModel.Draw(pointLightShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(1.0f));
//        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        pointLightShader.setMat4("model", model);
        platformModel.Draw(pointLightShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, streetLampPosition1);
        model = glm::scale(model, glm::vec3(10.0f));
//        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        pointLightShader.setMat4("model", model);
        streetLampModel.Draw(pointLightShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, streetLampPosition2);
        model = glm::scale(model, glm::vec3(10.0f));
//        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        pointLightShader.setMat4("model", model);
        streetLampModel.Draw(pointLightShader);

        sunShader.use();
        model = glm::translate(model, sunPosition);
        model = glm::scale(model, glm::vec3(4.0f));
//        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        sunShader.setVec3("lightColor",  glm::vec3(10.0f));
        sunShader.setMat4("model", model);
        sunShader.setMat4("view", view);
        sunShader.setMat4("projection", projection);
        sunModel.Draw(sunShader);

        glDisable(GL_CULL_FACE);

        platformShader.use();

        glBindTexture(GL_TEXTURE_2D, crackTex);
        glActiveTexture(crackTex);
        glBindVertexArray(grassVAO);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(14.0f,  0.3f, -49.0f));
        model = glm::scale(model, glm::vec3(12.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        platformShader.setMat4("model", model);
        platformShader.setMat4("view", view);
        platformShader.setMat4("projection", projection);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, sunflowerTex);
        glActiveTexture(sunflowerTex);
        glBindVertexArray(grassVAO);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(22.0f,  1.5f, -52.5f));
        model = glm::scale(model, glm::vec3(2.5f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        platformShader.setMat4("model", model);
        platformShader.setMat4("view", view);
        platformShader.setMat4("projection", projection);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        skyboxShader.use();
        skyboxShader.setMat4("view", glm::mat4(glm::mat3(programState->camera.GetViewMatrix())));
        skyboxShader.setMat4("projection", projection);
        skyboxShader.setInt("bloom", bloom);
        bloomShader.setFloat("exposure", exposure);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloomShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        bloomShader.setInt("bloom", bloom);
        bloomShader.setFloat("exposure", exposure);
        renderQuad();

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete programState;
    glfwTerminate();
    return 0;
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Camera info");
    const Camera& c = programState->camera;
    ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
    ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
    ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
    ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !bloomKeyPressed) {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        bloomKeyPressed = false;
    }
}

/*
unsigned int loadCubemap(vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
*/

unsigned int loadCubemap(vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum internalFormat;
            GLenum dataFormat;
            if (nrChannels == 1) {
                internalFormat = dataFormat = GL_RED;
            } else if (nrChannels == 3) {
                internalFormat = GL_SRGB;
                dataFormat = GL_RGB;
            } else if (nrChannels == 4) {
                internalFormat = GL_SRGB;
                dataFormat = GL_RGBA;
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int loadTexture(char const* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    GLenum format, internalFormat;
    if (nrChannels == 1)
        format = internalFormat = GL_RED;
    else if (nrChannels == 3){
        format = GL_RGB;
        internalFormat = GL_SRGB;
    }
    else if (nrChannels == 4){
        format = GL_RGBA;
        internalFormat = GL_SRGB_ALPHA;
    }

    if (data)
    {
        glBindTexture(GL_TEXTURE_2D, textureID);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}