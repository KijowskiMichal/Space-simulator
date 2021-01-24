#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"
#include "Physics.h"

Core::Shader_Loader shaderLoader;
GLuint programColor;
GLuint programTexture;
GLuint programBlur;
GLuint programFinal;

obj::Model planeModel, shipModel, boxModel;
Core::RenderContext planeContext, shipContext, boxContext;
GLuint groundTexture, metalTexture;

glm::vec3 cameraPos = glm::vec3(0, 0, 10);
glm::vec3 cameraDir;
glm::vec3 cameraSide;
float cameraAngle = 0;
glm::mat4 cameraMatrix, perspectiveMatrix;

glm::quat rotation = glm::quat(1, 0, 0, 0);

glm::vec3 lightDir = glm::normalize(glm::vec3(0.5, -1, -0.5));

int oldX = 0, oldY = 0, divX, divY;



Physics pxScene(9.8 /* gravity (m/s^2) */);


const double physicsStepTime = 1.f / 60.f;
double physicsTimeToProcess = 0;

PxRigidStatic *planeBody, *shipBody = nullptr;
PxMaterial *planeMaterial, *shipMaterial = nullptr;

int SCR_WIDTH = 600;
int SCR_HEIGHT = 600;

unsigned int hdrFBO;
unsigned int colorBuffers[2];
unsigned int rboDepth;
unsigned int pingpongFBO[2];
unsigned int pingpongColorbuffers[2];

bool bloom = true;
float exposure = 2.5f;

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
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

struct Renderable {
    Core::RenderContext* context;
    glm::mat4 modelMatrix;
    GLuint textureId;
};
std::vector<Renderable*> renderables;

void initRenderables()
{
    planeModel = obj::loadModelFromFile("models/plane.obj");
    shipModel = obj::loadModelFromFile("models/spaceship.obj");
    boxModel = obj::loadModelFromFile("models/box.obj");

    boxContext.initFromOBJ(boxModel);
    planeContext.initFromOBJ(planeModel);
    shipContext.initFromOBJ(shipModel);

    groundTexture = Core::LoadTexture("textures/sand.jpg");
    metalTexture = Core::LoadTexture("textures/box.jpg");

}

void initPhysicsScene()
{

}

void updateTransforms()
{
    auto actorFlags = PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC;
    PxU32 nbActors = pxScene.scene->getNbActors(actorFlags);
    if (nbActors)
    {
        std::vector<PxRigidActor*> actors(nbActors);
        pxScene.scene->getActors(actorFlags, (PxActor**)&actors[0], nbActors);
        for (auto actor : actors)
        {
            if (!actor->userData) continue;
            Renderable *renderable = (Renderable*)actor->userData;

            PxMat44 transform = actor->getGlobalPose();
            auto &c0 = transform.column0;
            auto &c1 = transform.column1;
            auto &c2 = transform.column2;
            auto &c3 = transform.column3;

            renderable->modelMatrix = glm::mat4(
                c0.x, c0.y, c0.z, c0.w,
                c1.x, c1.y, c1.z, c1.w,
                c2.x, c2.y, c2.z, c2.w,
                c3.x, c3.y, c3.z, c3.w);
        }
    }
}

void keyboard(unsigned char key, int x, int y)
{
    float angleSpeed = 0.1f;
    float moveSpeed = 0.1f;
    switch (key)
    {
    case 'z': cameraAngle -= angleSpeed; break;
    case 'x': cameraAngle += angleSpeed; break;
    case 'w': cameraPos += cameraDir * moveSpeed; break;
    case 's': cameraPos -= cameraDir * moveSpeed; break;
    case 'd': cameraPos += cameraSide * moveSpeed; break;
    case 'a': cameraPos -= cameraSide * moveSpeed; break;
    }
}

void mouse(int x, int y)
{
    divX = x - oldX;
    divY = y - oldY;
    oldX = x;
    oldY = y;
}

glm::mat4 createCameraMatrix()
{
    glm::quat x = glm::angleAxis(divX / 50.0f, glm::vec3(0, 1, 0));
    glm::quat y = glm::angleAxis(divY / 50.0f, glm::vec3(1, 0, 0));
    glm::quat z = glm::angleAxis(cameraAngle, glm::vec3(0, 0, 1));
    glm::quat xy = x * y * z;
    rotation = glm::normalize(xy * rotation);

    divX = 0;
    divY = 0;
    cameraAngle = 0;

    cameraDir = glm::inverse(rotation) * glm::vec3(0, 0, -1);
    cameraSide = glm::inverse(rotation) * glm::vec3(1, 0, 0);

    return Core::createViewMatrixQuat(cameraPos, rotation);
}

void drawObjectColor(Core::RenderContext* context, glm::mat4 modelMatrix, glm::vec3 color)
{
    GLuint program = programColor;

    glUseProgram(program);

    glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);
    glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);

    glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    Core::DrawContext(*context);

    glUseProgram(0);
}

void drawCube(glm::mat4 modelMatrix)
{
    glm::vec3 color = glm::vec3(1,0.843f,0);
    glUseProgram(programTexture);

    glUniform3f(glGetUniformLocation(programTexture, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
    Core::SetActiveTexture(metalTexture, "textureSampler", programTexture, 0);

    glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(programTexture, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(programTexture, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    Core::DrawContext(boxContext);

    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.60, 0.85, 1.05)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.60, 0.85, 1.05)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.60, -0.85, 1.05)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.60, -0.85, 1.05)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.85, 0.60, 1.05)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.85, 0.60, 1.05)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.85, -0.60, 1.05)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.85, -0.60, 1.05)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);

    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.60, 0.85, -1.05)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.60, 0.85, -1.05)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.60, -0.85, -1.05)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.60, -0.85, -1.05)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.85, 0.60, -1.05)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.85, 0.60, -1.05)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.85, -0.60, -1.05)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.85, -0.60, -1.05)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);

    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(1.05, 0.85, 0.60)) * glm::rotate((float)glm::radians(180.f), glm::vec3(1, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(1.05, -0.85, 0.60)) * glm::rotate((float)glm::radians(180.f), glm::vec3(1, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(1.05, -0.85, -0.60)) * glm::rotate((float)glm::radians(180.f), glm::vec3(1, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(1.05, 0.85, -0.60)) * glm::rotate((float)glm::radians(180.f), glm::vec3(1, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(1.05, 0.60, 0.85)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(1.05, -0.60, 0.85)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(1.05, -0.60, -0.85)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(1.05, 0.60, -0.85)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);

    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-1.05, 0.85, 0.60)) * glm::rotate((float)glm::radians(180.f), glm::vec3(1, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-1.05, -0.85, 0.60)) * glm::rotate((float)glm::radians(180.f), glm::vec3(1, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-1.05, -0.85, -0.60)) * glm::rotate((float)glm::radians(180.f), glm::vec3(1, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-1.05, 0.85, -0.60)) * glm::rotate((float)glm::radians(180.f), glm::vec3(1, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-1.05, 0.60, 0.85)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-1.05, -0.60, 0.85)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-1.05, -0.60, -0.85)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-1.05, 0.60, -0.85)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);

    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.60, 1.05, 0.85)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.60, 1.05, 0.85)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.60, 1.05, -0.85)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.60, 1.05, -0.85)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.85, 1.05, 0.60)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.85, 1.05, 0.60)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.85, 1.05, -0.60)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.85, 1.05, -0.60)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);

    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.60, -1.05, 0.85)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.60, -1.05, 0.85)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.60, -1.05, -0.85)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.60, -1.05, -0.85)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.85, -1.05, 0.60)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.85, -1.05, 0.60)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(-0.85, -1.05, -0.60)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);
    drawObjectColor(&boxContext, modelMatrix * glm::translate(glm::vec3(0.85, -1.05, -0.60)) * glm::rotate((float)glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.3f, 0.05f, 0.05f)), color);

    glUseProgram(0);
}

void drawObjectTexture(Core::RenderContext * context, glm::mat4 modelMatrix, GLuint textureId)
{
    GLuint program = programTexture;

    glUseProgram(program);

    glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
    Core::SetActiveTexture(textureId, "textureSampler", program, 0);

    glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    Core::DrawContext(*context);

    glUseProgram(0);
}
void renderScene()
{
    double time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    static double prevTime = time;
    double dtime = time - prevTime;
    prevTime = time;

    if (dtime < 1.f) {
        physicsTimeToProcess += dtime;
        while (physicsTimeToProcess > 0) {
            pxScene.step(physicsStepTime);
            physicsTimeToProcess -= physicsStepTime;
        }
    }

    cameraMatrix = createCameraMatrix();
    perspectiveMatrix = Core::createPerspectiveMatrix();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateTransforms();

    for (Renderable* renderable : renderables) {
        drawObjectTexture(renderable->context, renderable->modelMatrix, renderable->textureId);
    }

    glm::mat4 shipMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * glm::translate(glm::vec3(0, -0.25f, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.25f));
    drawObjectTexture(&shipContext, shipMatrix, groundTexture);

    drawCube(glm::translate(glm::vec3(0, 0, 0)));


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    bool horizontal = true, first_iteration = true;
    unsigned int amount = 10;
    glUseProgram(programBlur);
    for (unsigned int i = 0; i < amount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        glUniform1i(glGetUniformLocation(programBlur, "horizontal"), horizontal);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);
        renderQuad();
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(programFinal);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
    glUniform1i(glGetUniformLocation(programFinal, "bloom"), bloom);
    glUniform1f(glGetUniformLocation(programFinal, "exposure"), exposure);
    renderQuad();

    glutSwapBuffers();
}

void init()
{
    srand(time(0));
    glEnable(GL_DEPTH_TEST);
    programColor = shaderLoader.CreateProgram("shaders/shader_color.vert", "shaders/shader_color.frag");
    programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
    programBlur = shaderLoader.CreateProgram("shaders/gaussian.vert", "shaders/gaussian.frag");
    programFinal = shaderLoader.CreateProgram("shaders/final.vert", "shaders/final.frag");

    initRenderables();
    initPhysicsScene();

    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
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

    glUseProgram(programBlur);
    glUniform1i(glGetUniformLocation(programBlur, "image"), 0);
    glUseProgram(programFinal);
    glUniform1i(glGetUniformLocation(programFinal, "scene"), 0);
    glUniform1i(glGetUniformLocation(programFinal, "bloomBlur"), 1);
}

void shutdown()
{
    shaderLoader.DeleteProgram(programColor);
    shaderLoader.DeleteProgram(programTexture);
}

void idle()
{
    glutPostRedisplay();
}

int main(int argc, char ** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(200, 200);
    glutInitWindowSize(600, 600);
    glutCreateWindow("Space Simulator");
    glewInit();

    init();
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouse);
    glutDisplayFunc(renderScene);
    glutIdleFunc(idle);

    glutMainLoop();

    shutdown();

    return 0;
}
