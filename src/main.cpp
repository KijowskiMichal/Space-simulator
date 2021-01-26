#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"
#include "Physics.h"

Core::Shader_Loader shaderLoader;
GLuint programColor;
GLuint programTexture;

obj::Model planeModel, shipModel, sphereModel, deathStar;
Core::RenderContext planeContext, shipContext, sphereContext, deathStarContext;
GLuint groundTexture, moonTexture;

glm::vec3 cameraPos = glm::vec3(0, 0, 600);
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

PxRigidStatic* planeBody, * shipBody = nullptr;
PxMaterial* planeMaterial, * shipMaterial = nullptr;

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


class Planet {
public:
    GLuint texture;
    std::string name;
    Core::RenderContext* context;
    glm::mat4 modelMatrix;
    float rotationVelocity = 0;
    glm::vec3 rotationAxis;
    

    void render(glm::mat4 rotator = glm::mat4(1.0f)) {
        drawObjectTexture(context, rotator * modelMatrix * rotator, texture);
    }
    glm::mat4 createRotator(float rotateTime) {
        glm::mat4 rotator;
        rotator = glm::rotate(rotator, (rotateTime / 2.f) * rotationVelocity * 3.14159f, rotationAxis);
        return rotator;
    }
    void createMoon(glm::vec3 distanceFromParent,glm::mat4 parentRotator,float rotateTime, glm::vec3 moonRotationAxis,float moonRotationVelocity) {
        glm::mat4 moonRotator;
        moonRotator = glm::rotate(moonRotator, (rotateTime / 2.f) * moonRotationVelocity * 3.14159f, moonRotationAxis);
        drawObjectTexture(&sphereContext, (parentRotator * modelMatrix) * moonRotator * glm::translate(distanceFromParent) * glm::scale(glm::vec3(0.3f)), texture);
    }

};

std::vector<Planet*> planets;

void initRenderables()
{

    shipModel = obj::loadModelFromFile("models/spaceship.obj");
    shipContext.initFromOBJ(shipModel);

    sphereModel = obj::loadModelFromFile("models/sphere.obj");
    sphereContext.initFromOBJ(sphereModel);

    deathStar = obj::loadModelFromFile("models/0.obj");
    deathStarContext.initFromOBJ(deathStar);

    moonTexture = Core::LoadTexture("textures/moon.jpg");


    groundTexture = Core::LoadTexture("textures/sand.jpg");


   

    Planet* sun = new Planet();
    sun->name = "sun";
    sun->context = &sphereContext;
    sun->texture = Core::LoadTexture("textures/2k_sun.jpg");
    sun->modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(30));
    planets.emplace_back(sun);

    Planet* mercury = new Planet();
    mercury->name = "mercury";
    mercury->context = &sphereContext;
    mercury->texture = Core::LoadTexture("textures/2k_mercury.jpg");
    mercury->modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -65.0f)) * glm::scale(glm::vec3(5));
    mercury->rotationVelocity = 0.4f;
    mercury->rotationAxis = glm::vec3(1.0, 1.0, 0.0);
    planets.emplace_back(mercury);



    Planet* venus = new Planet();
    venus->name = "venus";
    venus->context = &sphereContext;
    venus->texture = Core::LoadTexture("textures/2k_venus_surface.jpg");
    venus->modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -100.0f)) * glm::scale(glm::vec3(15));
    venus->rotationVelocity = 0.2f;
    venus->rotationAxis = glm::vec3(-1.0, 1.0, 0.0);
   
    planets.emplace_back(venus);

    Planet* neptune = new Planet();
    neptune->name = "neptune";
    neptune->context = &sphereContext;
    neptune->texture = Core::LoadTexture("textures/2k_neptune.jpg");
    neptune->modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -140.0f)) * glm::scale(glm::vec3(25));
    neptune->rotationVelocity = 0.3f;
    neptune->rotationAxis = glm::vec3(0.0, 1.0, 0.0);
    
    planets.emplace_back(neptune);

    Planet* hoth = new Planet();
    hoth->name = "hoth";
    hoth->context = &sphereContext;
    hoth->texture = Core::LoadTexture("textures/hoth.jpg");
    hoth->modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -220.0f)) * glm::scale(glm::vec3(28));
    hoth->rotationVelocity = 0.5f;
    hoth->rotationAxis = glm::vec3(1.0, 1.0, 0.0);
   
    planets.emplace_back(hoth);

    Planet* saturn = new Planet();
    saturn->name = "saturn";
    saturn->context = &sphereContext;
    saturn->texture = Core::LoadTexture("textures/2k_saturn.jpg");
    saturn->modelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -300.0f)) * glm::scale(glm::vec3(35));
    saturn->rotationVelocity = 0.4f;
    saturn->rotationAxis = glm::vec3(0.0, -1.0, 0.0);
    
    planets.emplace_back(saturn);




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
            Planet* planet = (Planet*)actor->userData;

            PxMat44 transform = actor->getGlobalPose();
            auto& c0 = transform.column0;
            auto& c1 = transform.column1;
            auto& c2 = transform.column2;
            auto& c3 = transform.column3;

            planet->modelMatrix = glm::mat4(
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
    float moveSpeed = 5.0f;
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

    float rotateTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;





    cameraMatrix = createCameraMatrix();
    perspectiveMatrix = Core::createPerspectiveMatrix();



    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    updateTransforms();


    for (Planet* planet : planets) {
        glm::mat4 rotator = planet->createRotator(rotateTime);
        if (bool(planet->rotationVelocity)) {
            planet->render(rotator);
            if (planet->name == "mercury") {
                glm::mat4 moonrotator;
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 2.5f * 3.14159f, glm::vec3(1, 1, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 2.0f)) * glm::scale(glm::vec3(0.5f)), moonTexture);
            }
            else if (planet->name == "venus") {
                glm::mat4 moonrotator;
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 2.5f * 3.14159f, glm::vec3(0, 1, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 1.3)) * glm::scale(glm::vec3(0.2f)), moonTexture);
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 1.5f * 3.14159f, glm::vec3(1, 1, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 1.8f)) * glm::scale(glm::vec3(0.3f)), moonTexture);

            }
            else if (planet->name == "neptune") {
                glm::mat4 moonrotator;
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 0.5f * 3.14159f, glm::vec3(0, 1, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 1.5f)) * glm::scale(glm::vec3(0.1f)), moonTexture);
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 1.2f * 3.14159f, glm::vec3(1, 0, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 2)) * glm::scale(glm::vec3(0.2f)), moonTexture);
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 1.5f * 3.14159f, glm::vec3(1, 1, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 2.2f)) * glm::scale(glm::vec3(0.3f)), moonTexture);

            }
            else if (planet->name == "hoth") {
                glm::mat4 moonrotator;
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 1.5f * 3.14159f, glm::vec3(0, 1, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 1.2f)) * glm::scale(glm::vec3(0.1f)), moonTexture);
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 1.7f * 3.14159f, glm::vec3(1, 0, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 1.5f)) * glm::scale(glm::vec3(0.2f)), moonTexture);
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 1.9f * 3.14159f, glm::vec3(1, 1, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 2.0f)) * glm::scale(glm::vec3(0.3f)), moonTexture);
            }
            else if (planet->name == "saturn") {
                glm::mat4 moonrotator;
                moonrotator = glm::rotate(moonrotator, (rotateTime / 2.f) * planet->rotationVelocity * 1.25f * 3.14159f, glm::vec3(1, 1, 0));
                drawObjectTexture(&sphereContext, (rotator * planet->modelMatrix) * moonrotator * glm::translate(glm::vec3(0, 0, 2.0f)) * glm::scale(glm::vec3(0.2f)), moonTexture);
            }

        }
        else {
            planet->render();
        };
    };


    glm::mat4 shipMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * glm::translate(glm::vec3(0, -0.1f, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.20f));

    drawObjectTexture(&shipContext, shipMatrix, groundTexture);

    glutSwapBuffers();
}

void init()
{
    srand(time(0));
    glEnable(GL_DEPTH_TEST);
    programColor = shaderLoader.CreateProgram("shaders/shader_color.vert", "shaders/shader_color.frag");
    programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");

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

int main(int argc, char** argv)
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
