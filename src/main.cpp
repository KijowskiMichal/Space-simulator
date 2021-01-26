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
GLuint programSkybox;

obj::Model planeModel, shipModel, skyboxModel;
Core::RenderContext planeContext, shipContext, skyboxContext;
GLuint groundTexture;
GLuint skyboxTexture[6];

glm::vec3 cameraPos = glm::vec3(0, 3, 10);
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
    skyboxModel = obj::loadModelFromFile("models/skybox.obj");

    planeContext.initFromOBJ(planeModel);
    shipContext.initFromOBJ(shipModel);
    skyboxContext.initFromOBJ(skyboxModel);

    groundTexture = Core::LoadTexture("textures/sand.jpg");

    skyboxTexture[0] = Core::LoadTextureSkybox("textures/back.png");
    skyboxTexture[1] = Core::LoadTextureSkybox("textures/bottom.png");
    skyboxTexture[2] = Core::LoadTextureSkybox("textures/front.png");
    skyboxTexture[3] = Core::LoadTextureSkybox("textures/left.png");
    skyboxTexture[4] = Core::LoadTextureSkybox("textures/right.png");
    skyboxTexture[5] = Core::LoadTextureSkybox("textures/top.png");
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
    rotation = glm::normalize(x * y * z * rotation);

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

void drawObjectTexture(Core::RenderContext* context, glm::mat4 modelMatrix, GLuint textureId)
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

void drawObjectTexture(GLuint program, Core::RenderContext* context, glm::mat4 modelMatrix, GLuint textureId) {

    glUseProgram(program);

    glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
    Core::SetActiveTexture(textureId, "textureSampler", program, 0);

    glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    Core::DrawContext(*context);

    glUseProgram(0);
}

void renderSkybox(glm::vec3 userPos) {

    int sc = 70;
    glm::mat4 matrix = glm::translate(glm::vec3(userPos.x, userPos.y-sc, userPos.z));
    glm::mat4 scale = glm::scale(glm::vec3(sc, sc, sc));
    drawObjectTexture(programSkybox, &skyboxContext, matrix*glm::translate(glm::vec3(0,sc, sc)) * glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0)) * scale, skyboxTexture[0]); //back
    drawObjectTexture(programSkybox, &skyboxContext, matrix*glm::translate(glm::vec3(0,0,0)) * scale, skyboxTexture[1]); //bottom
    drawObjectTexture(programSkybox, &skyboxContext, matrix*glm::translate(glm::vec3(0, sc, -sc)) * glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0)) * scale, skyboxTexture[2]); //front
    drawObjectTexture(programSkybox, &skyboxContext, matrix*glm::translate(glm::vec3(sc, sc, 0)) * glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1)) * scale, skyboxTexture[4]); //right
    drawObjectTexture(programSkybox, &skyboxContext, matrix*glm::translate(glm::vec3(-sc, sc, 0)) * glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1)) * scale, skyboxTexture[3]); //left
    drawObjectTexture(programSkybox, &skyboxContext, matrix*glm::translate(glm::vec3(0, 2*sc, 0)) * scale, skyboxTexture[5]); //top
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
    perspectiveMatrix = Core::createPerspectiveMatrix(0.1f, 200.f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.1f, 0.3f, 1.0f);

    updateTransforms();

    for (Renderable* renderable : renderables) {
        drawObjectTexture(renderable->context, renderable->modelMatrix, renderable->textureId);
    }

    glm::mat4 shipMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * glm::translate(glm::vec3(0, -0.25f, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.25f));
    drawObjectTexture(&shipContext, shipMatrix, groundTexture);

    glDepthMask(GL_FALSE);
    renderSkybox(cameraPos);
    glDepthMask(GL_TRUE);

    glutSwapBuffers();
}

void init()
{
    srand(time(0));
    glEnable(GL_DEPTH_TEST);
    programColor = shaderLoader.CreateProgram("shaders/shader_color.vert", "shaders/shader_color.frag");
    programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
    programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");

    initRenderables();
    initPhysicsScene();
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
