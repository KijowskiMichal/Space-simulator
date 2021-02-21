#include "glew.h"
#include "freeglut.h"
#include "windows.h"
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



PxRigidDynamic* shipBody = nullptr;
PxRigidDynamic* shipLE = nullptr;
PxRigidDynamic* shipRE = nullptr;
PxVec3 shipBodyVel;
glm::vec3 cubePos = cubePos = glm::vec3(60, 10, 60);
float ns, we, np, nz;
float camera = 3.f;
float cameraTarget = 0;
int points = 0;
int timeToEnd = 360;
bool block = false;

PxFixedJoint* joint1;
PxFixedJoint* joint2;

//------------------------------------------------------------------
// contact pairs filtering function
static PxFilterFlags simulationFilterShader(PxFilterObjectAttributes attributes0,
    PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    pairFlags =
        PxPairFlag::eCONTACT_DEFAULT | // default contact processing
        PxPairFlag::eNOTIFY_CONTACT_POINTS | // contact points will be available in onContact callback
        PxPairFlag::eNOTIFY_TOUCH_FOUND | // onContact callback will be called for this pair
        PxPairFlag::eNOTIFY_TOUCH_PERSISTS;

    return physx::PxFilterFlag::eDEFAULT;
}

//------------------------------------------------------------------
// simulation events processor
class SimulationEventCallback : public PxSimulationEventCallback
{
public:
    void onContact(const PxContactPairHeader& pairHeader,
        const PxContactPair* pairs, PxU32 nbPairs)
    {
        // HINT: You can check which actors are in contact
        // using pairHeader.actors[0] and pairHeader.actors[1]
        const PxU32 bufferSize = 64;
        PxContactPairPoint contacts[bufferSize];
        int count = 0;
        for (PxU32 i = 0; i < nbPairs; i++)
        {
            const PxContactPair& cp = pairs[i];

            PxU32 nbContacts = pairs[i].extractContacts(contacts, bufferSize);
            for (PxU32 j = 0; j < nbContacts; j++)
            {
                PxVec3 point = contacts[j].position;

                ns *= -1;
                we *= -1;
                np *= -1;
                nz *= -1;

                if (ns > 480) ns = 480;
                
            }

        }
    }

    virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) {}
    virtual void onWake(PxActor** actors, PxU32 count) {}
    virtual void onSleep(PxActor** actors, PxU32 count) {}
    virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) {}
    virtual void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) {}
};

// Initalization of physical scene (PhysX)
SimulationEventCallback simulationEventCallback;
Physics pxScene(9.8 /* gravity (m/s^2) */, simulationFilterShader,
    &simulationEventCallback);

float appLoadingTime;

Core::Shader_Loader shaderLoader;
GLuint programColor;
GLuint programColorWB;
GLuint programTexture;
GLuint programSkybox;
GLuint programBlur;
GLuint programFinal;
GLuint programTexBloom;
GLuint programTexBloomSun;


struct Renderable {
    Core::RenderContext* context;
    glm::mat4 modelMatrix;
    GLuint textureId;
};
std::vector<Renderable*> renderables;
std::vector<glm::vec3> lights;
std::vector<float> marksizm;

obj::Model planeModel, shipModel, shipModelLE, shipModelRE, skyboxModel, boxModel, sphereModel, deathStar, circleModel, cylinderModel;
Core::RenderContext planeContext, shipContext, shipContextLE, shipContextRE, boxContext, skyboxContext, sphereContext, deathStarContext, circleContext, cylinderContext;
GLuint groundTexture, metalTexture, moonTexture, sunTexture, mercuryTexture, venusTexture, hothTexture, saturnTexture, neptuneTexture, shipTexture, circleTexture;
GLuint skyboxTexture[6];

glm::vec3 cameraDir;
glm::vec3 cameraPos = glm::vec3(100, 0, 0);
glm::vec3 cameraSide;
float cameraAngle = glm::radians(270.f);
glm::mat4 cameraMatrix, perspectiveMatrix, shipModelMatrix;


glm::quat rotation = glm::quat(1, 0, 0, 0);

glm::vec3 lightDir = glm::normalize(glm::vec3(0.5, -1, -0.5));

int oldX = 0, oldY = 0, divX, divY, itr;
float frustumscale;




const double physicsStepTime = 1.f / 60.f;
double physicsTimeToProcess = 0;
PxMaterial* shipMaterial = nullptr;

int SCR_WIDTH = 600;
int SCR_HEIGHT = 600;

unsigned int hdrFBO;
unsigned int colorBuffers[2];
unsigned int rboDepth;
unsigned int pingpongFBO[2];
unsigned int pingpongColorbuffers[2];

bool bloom = true;
float exposure = 2.5f;
int oldtime;

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

void drawObjectTexture(Core::RenderContext* context, glm::mat4 modelMatrix, GLuint textureId)
{
    GLuint program = programTexture;

    glUseProgram(program);
    glUniform3f(glGetUniformLocation(program, "lightPosCube"), 60, 10, 60);
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

class Moon {
public:
    GLuint texture = moonTexture;
    Core::RenderContext* context = &sphereContext;
    glm::mat4 modelMatrix = glm::mat4(0.0f);
    float rotationVelocity = 0;
    glm::vec3 rotationAxis;
    glm::vec3 distance;
    PxRigidDynamic* moonBody;
    int ri = 0;

    void init(float v, glm::vec3 a, glm::vec3 d) {
        rotationVelocity = v;
        rotationAxis = a;
        distance = d;
    }
    void usephysx()
    {
        moonBody = pxScene.physics->createRigidDynamic(PxTransform(0, 0, 0));
        PxMaterial* material = pxScene.physics->createMaterial(1, 1, 0.6);
        PxShape* moonShape = pxScene.physics->createShape(PxSphereGeometry(0.2), *material);
        moonBody->attachShape(*moonShape);
        moonShape->release();
        moonBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        moonBody->userData = renderables[ri];
        pxScene.scene->addActor(*moonBody);
    }
    void updateTargetPhysx()
    {
        moonBody->setKinematicTarget(PxTransform(modelMatrix[3][0], modelMatrix[3][1], modelMatrix[3][2]));
    }
    void render() {
        drawObjectTexture(context, modelMatrix, texture);
    }
    glm::mat4 createRotator(float rotateTime) {
        glm::mat4 rotator;
        rotator = glm::rotate(rotator, (rotateTime / 2.f) * rotationVelocity * 3.14159f, rotationAxis);
        return rotator;
    }
};


class Planet {
public:
    GLuint texture;
    std::string name;
    Core::RenderContext* context = &sphereContext;
    glm::mat4 modelMatrix = glm::mat4(0.0f);
    float rotationVelocity = 0;
    glm::vec3 rotationAxis;
    std::vector<Moon*> children;
    PxRigidDynamic *planetBody;
    Renderable* renderable;
    int ri = 0;

    void init(GLuint t, std::string n, float v, glm::vec3 a) {
        texture = t;
        name = n;
        rotationVelocity = v;
        rotationAxis = a;
        renderable = new Renderable();
        renderable->context = context;
        renderable->modelMatrix = modelMatrix;
        renderable->textureId = texture;
        renderables.emplace_back(renderable);
        ri = renderables.size()-1;
    }
    void usephysx()
    {
        planetBody = pxScene.physics->createRigidDynamic(PxTransform(0, 0, 0));
        PxMaterial *material = pxScene.physics->createMaterial(1, 1, 0.6);


        PxShape* planetShape;
        if (name == "sun")  planetShape = pxScene.physics->createShape(PxSphereGeometry(30), *material);
        else if (name == "mercury")  planetShape = pxScene.physics->createShape(PxSphereGeometry(5), *material);
        else if (name == "venus")  planetShape = pxScene.physics->createShape(PxSphereGeometry(15), *material);
        else if (name == "neptune")  planetShape = pxScene.physics->createShape(PxSphereGeometry(25), *material);
        else if (name == "hoth")  planetShape = pxScene.physics->createShape(PxSphereGeometry(28), *material);
        else if (name == "saturn")  planetShape = pxScene.physics->createShape(PxSphereGeometry(35), *material);


        planetBody->attachShape(*planetShape);
        planetShape->release();
        planetBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        planetBody->userData = renderables[ri];
        pxScene.scene->addActor(*planetBody);
    }
    void updateTargetPhysx()
    {
        planetBody->setKinematicTarget(PxTransform(modelMatrix[3][0], modelMatrix[3][1], modelMatrix[3][2]));
    }
    void render() {
        drawObjectTexture(context, modelMatrix, texture);
    }
    void renderBloom() {
        drawObjectTexture(programTexBloom, context, modelMatrix, texture);
    }
    void renderSun() {
        drawObjectTexture(programTexBloomSun, context, modelMatrix, texture);
    }
    glm::mat4 createRotator(float rotateTime) {
        glm::mat4 rotator;
        rotator = glm::rotate(rotator, (rotateTime / 2.f) * rotationVelocity * 3.14159f, rotationAxis);
        return rotator;
    }

};

std::vector<Planet*> planets;
std::vector<Moon*> moons;

void initRenderables()
{
    shipModel = obj::loadModelFromFile("models/ship.obj");
    shipContext.initFromOBJ(shipModel);
    shipModelLE = obj::loadModelFromFile("models/engineleft.obj");
    shipContextLE.initFromOBJ(shipModelLE);
    shipModelRE = obj::loadModelFromFile("models/engineright.obj");
    shipContextRE.initFromOBJ(shipModelRE);
    sphereModel = obj::loadModelFromFile("models/sphere.obj");
    sphereContext.initFromOBJ(sphereModel);
    deathStar = obj::loadModelFromFile("models/0.obj");
    deathStarContext.initFromOBJ(deathStar);
    boxModel = obj::loadModelFromFile("models/box.obj");
    boxContext.initFromOBJ(boxModel);
    skyboxModel = obj::loadModelFromFile("models/skybox.obj");
    skyboxContext.initFromOBJ(skyboxModel);
    planeModel = obj::loadModelFromFile("models/plane.obj");
    planeContext.initFromOBJ(planeModel);
    circleModel = obj::loadModelFromFile("models/circle.obj");
    circleContext.initFromOBJ(circleModel);
    cylinderModel = obj::loadModelFromFile("models/cylinder.obj");
    cylinderContext.initFromOBJ(cylinderModel);

    moonTexture = Core::LoadTexture("textures/moon.jpg");
    groundTexture = Core::LoadTexture("textures/sand.jpg");
    metalTexture = Core::LoadTexture("textures/box.jpg");
    sunTexture = Core::LoadTexture("textures/2k_sun.jpg");
    mercuryTexture = Core::LoadTexture("textures/2k_mercury.jpg");
    venusTexture = Core::LoadTexture("textures/2k_venus_surface.jpg");
    neptuneTexture = Core::LoadTexture("textures/2k_neptune.jpg");
    hothTexture = Core::LoadTexture("textures/hoth.jpg");
    saturnTexture = Core::LoadTexture("textures/2k_saturn.jpg");
    shipTexture = Core::LoadTexture("textures/ship.png");
    circleTexture = Core::LoadTexture("textures/circle.jpg");

    skyboxTexture[0] = Core::LoadTextureSkybox("textures/back.png");
    skyboxTexture[1] = Core::LoadTextureSkybox("textures/bottom.png");
    skyboxTexture[2] = Core::LoadTextureSkybox("textures/front.png");
    skyboxTexture[3] = Core::LoadTextureSkybox("textures/left.png");
    skyboxTexture[4] = Core::LoadTextureSkybox("textures/right.png");
    skyboxTexture[5] = Core::LoadTextureSkybox("textures/top.png");

    Renderable* ship = new Renderable();
    ship->context = &shipContext;
    ship->textureId = shipTexture;
    ship->modelMatrix = glm::rotate(glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f));
    renderables.emplace_back(ship);

    Renderable* shipLE = new Renderable();
    shipLE->context = &shipContextLE;
    shipLE->textureId = shipTexture;
    shipLE->modelMatrix = glm::rotate(glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f));
    renderables.emplace_back(shipLE);

    Renderable* shipRE = new Renderable();
    shipRE->context = &shipContextRE;
    shipRE->textureId = shipTexture;
    shipRE->modelMatrix = glm::rotate(glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f));
    renderables.emplace_back(shipRE);

    Planet* sun = new Planet();
    sun->init(sunTexture, "sun", 0, glm::vec3(0, 0, 0));
    sun->usephysx();
    planets.emplace_back(sun);


    //MERCURY
    Planet* mercury = new Planet();
    mercury->init(mercuryTexture, "mercury", 0.2f, glm::vec3(0.0, -1.0, 0));
    mercury->usephysx();
    planets.emplace_back(mercury);

    Moon* mercury1 = new Moon();
    mercury1->init(0.5f, glm::vec3(1, 1, 0), glm::vec3(0, 0, 2.0f));
    mercury1->usephysx();
    mercury->children.emplace_back(mercury1);
    moons.emplace_back(mercury1);


    //VENUS
    Planet* venus = new Planet();
    venus->init(venusTexture, "venus", 0.1f, glm::vec3(0.0, -1.0, 0.0));
    venus->usephysx();
    planets.emplace_back(venus);

    Moon* venus1 = new Moon();
    Moon* venus2 = new Moon();
    venus1->init(0.5f, glm::vec3(-1.0, 1.0, 0.0), glm::vec3(0, 0, 2.0f));
    venus1->usephysx();
    venus2->init(0.25f, glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0, 0, 2.5f));
    venus2->usephysx();
    venus->children.emplace_back(venus1);
    venus->children.emplace_back(venus2);
    moons.emplace_back(venus1);
    moons.emplace_back(venus2);

    //NEPTUNE
    Planet* neptune = new Planet();
    neptune->init(neptuneTexture, "neptune", 0.15f, glm::vec3(0.0, -1.0, 0.0));
    neptune->usephysx();
    planets.emplace_back(neptune);

    Moon* neptune1 = new Moon();
    Moon* neptune2 = new Moon();
    Moon* neptune3 = new Moon();

    neptune1->init(0.5f, glm::vec3(0.0, 1.0, -1.0), glm::vec3(0, 0, 1.5f));
    neptune1->usephysx();
    neptune2->init(1.2f, glm::vec3(1.0, 0.0, 0.0), glm::vec3(0, 0, 2));
    neptune2->usephysx();
    neptune3->init(1.5f, glm::vec3(1.0, -1.0, 0.0), glm::vec3(0, 0, 2.5f));
    neptune3->usephysx();

    neptune->children.emplace_back(neptune1);
    neptune->children.emplace_back(neptune2);
    neptune->children.emplace_back(neptune3);


    moons.emplace_back(neptune1);
    moons.emplace_back(neptune2);
    moons.emplace_back(neptune3);

    //HOTH
    Planet* hoth = new Planet();
    hoth->init(hothTexture, "hoth", 0.25f, glm::vec3(0.0, -1.0, 0.0));
    hoth->usephysx();
    planets.emplace_back(hoth);

    Moon* hoth1 = new Moon();
    Moon* hoth2 = new Moon();

    hoth1->init(0.3f, glm::vec3(0.0, 1.0, 0.0), glm::vec3(0, 0, 1.5f));
    hoth1->usephysx();
    hoth2->init(0.4f, glm::vec3(1.0, 0.0, 0.0), glm::vec3(0, 0, 2.0f));
    hoth2->usephysx();

    hoth->children.emplace_back(hoth1);
    hoth->children.emplace_back(hoth2);

    moons.emplace_back(hoth1);
    moons.emplace_back(hoth2);

    //SATURN
    Planet* saturn = new Planet();
    saturn->init(saturnTexture, "saturn", 0.2f, glm::vec3(0.0, -1.0, 0.0));
    saturn->usephysx();
    planets.emplace_back(saturn);

    lights.emplace_back(glm::vec3(0, 0, 0));
}

void initPhysicsScene()
{
    shipBody = pxScene.physics->createRigidDynamic(PxTransform(80, 10, 0));
    shipMaterial = pxScene.physics->createMaterial(1, 1, 0.6);
    //PxShape* shipShape = pxScene.physics->createShape(PxBoxGeometry(7, 5, 2.3f), *shipMaterial);
    PxShape* shipShape = pxScene.physics->createShape(PxBoxGeometry(1, 1, 1), *shipMaterial);
    shipBody->attachShape(*shipShape);
    shipShape->release();
    //shipBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
    shipBody->userData = renderables[0];
    pxScene.scene->addActor(*shipBody);

    shipLE = pxScene.physics->createRigidDynamic(PxTransform(80, 11, 0));
    PxShape* shipShapeLE = pxScene.physics->createShape(PxBoxGeometry(0.001f, 0.001f, 0.001f), *shipMaterial);
    shipLE->attachShape(*shipShapeLE);
    shipShapeLE->release();
    shipLE->userData = renderables[1];
    pxScene.scene->addActor(*shipLE);

    shipRE = pxScene.physics->createRigidDynamic(PxTransform(80, 11, 0));
    PxShape* shipShapeRE = pxScene.physics->createShape(PxBoxGeometry(0.001f, 0.001f, 0.001f), *shipMaterial);
    shipRE->attachShape(*shipShapeRE);
    shipShapeRE->release();
    shipRE->userData = renderables[2];
    pxScene.scene->addActor(*shipRE);

    joint1 = PxFixedJointCreate(*pxScene.physics, shipBody, PxTransform(-0.489227f, 0.173815f, -0.008409f), shipLE, PxTransform(-0.506138f, 0.104812f, -0.057499f));
    joint2 = PxFixedJointCreate(*pxScene.physics, shipBody, PxTransform(0.489227f, 0.1f, 0.1f), shipRE, PxTransform(0.506138f, 0.104812f, 0.207124f));

    joint1->setBreakForce(550.f, 550.f);
    joint2->setBreakForce(550.f, 550.f);

}

void updateTransforms()
{
    // Here we retrieve the current transforms of the objects from the physical simulation.
    auto actorFlags = PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC;
    PxU32 nbActors = pxScene.scene->getNbActors(actorFlags);
    if (nbActors)
    {
        std::vector<PxRigidActor*> actors(nbActors);
        pxScene.scene->getActors(actorFlags, (PxActor**)&actors[0], nbActors);
        for (auto actor : actors)
        {
            // We use the userData of the objects to set up the model matrices
            // of proper renderables.
            if (!actor->userData) continue;
            Renderable* renderable = (Renderable*)actor->userData;

            // get world matrix of the object (actor)
            PxMat44 transform = actor->getGlobalPose();
            auto& c0 = transform.column0;
            auto& c1 = transform.column1;
            auto& c2 = transform.column2;
            auto& c3 = transform.column3;

            // set up the model matrix used for the rendering
            renderable->modelMatrix = glm::mat4(
                c0.x, c0.y, c0.z, c0.w,
                c1.x, c1.y, c1.z, c1.w,
                c2.x, c2.y, c2.z, c2.w,
                c3.x, c3.y, c3.z, c3.w);
        }
    }
}

void text(int x, int y, std::string text, int rozmiar)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, SCR_WIDTH, 0, SCR_HEIGHT, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glPushAttrib(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glRasterPos2i(x, y);
    void* rozm = GLUT_BITMAP_HELVETICA_18;
    if (rozmiar == 10) rozm = GLUT_BITMAP_HELVETICA_10;
    if (rozmiar == 12) rozm = GLUT_BITMAP_HELVETICA_12;
    for (int i = 0; i < text.size(); i++)
    {
        glutBitmapCharacter(rozm, text[i]);
    }
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'w': if (ns>-980) ns-=20;  break;
    case 's': if (ns < 480) ns+=20; break;
    case 'd': if (we < 28) we += 2; break;
    case 'a': if (we > -28) we -= 2; break;
    case 'e': if (np < 28) np += 2; break;
    case 'q': if (np > -28) np -= 2; break;
    case 'z': if (nz < 28) nz += 2; break;
    case 'c': if (nz > -28) nz -= 2; break;
    case 'p': ns = 0; we = 0; break;
    case '1': if (camera == 3.f) cameraTarget = -2.f; break;
    case '2': if (camera==1.f) cameraTarget = 2.f; break;
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
    PxTransform pxtr = shipBody->getGlobalPose();
    glm::quat pxtq = glm::quat(pxtr.q.w, pxtr.q.x, pxtr.q.y, pxtr.q.z);
    glm::vec3 cameraDirMat = pxtq * glm::vec3(0, 0, 1);
    glm::vec3 offset = cameraDirMat * camera;
    cameraPos = offset + glm::vec3(pxtr.p.x, pxtr.p.y, pxtr.p.z);


    glUseProgram(programTexture);
    glUniform3f(glGetUniformLocation(programTexture, "viewPos"), pxtr.p.x, pxtr.p.y + 3, pxtr.p.z + 8);
    glUseProgram(0);

    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 cameraTarget = glm::vec3(pxtr.p.x, pxtr.p.y, pxtr.p.z);
    glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

    cameraDir = cameraDirection;

    glm::mat4 returnowaTablica = glm::lookAt(cameraPos, cameraPos - cameraDirection, cameraUp);

    return glm::translate(glm::vec3(0,-1, 0)) * returnowaTablica;
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

void drawObjectColor(GLuint program, Core::RenderContext* context, glm::mat4 modelMatrix, glm::vec3 color)
{

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
    glm::vec3 color = glm::vec3(1, 0.843f, 0);
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

void renderSkybox(glm::vec3 userPos) {

    int sc = 1200;
    glm::mat4 matrix = glm::translate(glm::vec3(userPos.x, userPos.y - sc, userPos.z));
    glm::mat4 scale = glm::scale(glm::vec3(sc, sc, sc));
    drawObjectTexture(programSkybox, &skyboxContext, matrix * glm::translate(glm::vec3(0, sc, sc)) * glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0)) * scale, skyboxTexture[0]); //back
    drawObjectTexture(programSkybox, &skyboxContext, matrix * glm::translate(glm::vec3(0, 0, 0)) * scale, skyboxTexture[1]); //bottom
    drawObjectTexture(programSkybox, &skyboxContext, matrix * glm::translate(glm::vec3(0, sc, -sc)) * glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0)) * scale, skyboxTexture[2]); //front
    drawObjectTexture(programSkybox, &skyboxContext, matrix * glm::translate(glm::vec3(sc, sc, 0)) * glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1)) * scale, skyboxTexture[4]); //right
    drawObjectTexture(programSkybox, &skyboxContext, matrix * glm::translate(glm::vec3(-sc, sc, 0)) * glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1)) * scale, skyboxTexture[3]); //left
    drawObjectTexture(programSkybox, &skyboxContext, matrix * glm::translate(glm::vec3(0, 2 * sc, 0)) * scale, skyboxTexture[5]); //top
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

    int ftime = (int)(time / (0.5f));

    shipBody->setLinearVelocity(PxVec3(0, 0, 0));
    shipBody->setAngularVelocity(PxVec3(0, 0, 0));

    if (oldtime != ftime)
    {
        oldtime = ftime;
        we *= 0.7;
        np *= 0.7;
        nz *= 0.7;
        timeToEnd -= 1;
    }
    if (cameraTarget != 0)
    {
        float tmp = cameraTarget * 0.1;
        camera += tmp;
        cameraTarget -= tmp;
        if (std::abs(cameraTarget) < 0.01f)
        {
            cameraTarget = 0;
            if (camera <= 1.1)
            {
                camera = 1;
            }
            else
            {
                camera = 3;
            }
        }
    }

    float rotateTime = glutGet(GLUT_ELAPSED_TIME) / 4000.0f;

    PxRigidBodyExt::addLocalForceAtLocalPos(*shipBody, PxVec3(0, 0, we), PxVec3(2, 0, 0));
    PxRigidBodyExt::addLocalForceAtLocalPos(*shipBody, PxVec3(0, 0, ns), PxVec3(0, 0, 0));
    PxRigidBodyExt::addLocalForceAtLocalPos(*shipBody, PxVec3(0, np, 0), PxVec3(0, 0, 2));
    PxRigidBodyExt::addLocalForceAtLocalPos(*shipBody, PxVec3(0, nz, 0), PxVec3(2, 0, 0));

    shipBodyVel = shipBody->getLinearVelocity();


    PxTransform pxtr = shipBody->getGlobalPose();
    glUseProgram(programTexture);
    glUniform3f(glGetUniformLocation(programTexture, "viewPos"), pxtr.p.x, pxtr.p.y + 3, pxtr.p.z + 8);
    glUseProgram(0);

    cameraMatrix = createCameraMatrix();
    perspectiveMatrix = Core::createPerspectiveMatrix(1.f, 3000.f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);



    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderSkybox(cameraPos);

    updateTransforms();


    if (timeToEnd >= 0)
    {


        for (Planet* planet : planets) {
            glm::mat4 childRotator;
            glm::mat4 rotator = planet->createRotator(rotateTime);
            if (planet->name == "sun") {
                planet->modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0)) * glm::scale(glm::vec3(30));
                planet->renderSun();
            }
            else if (planet->name == "mercury") {
                planet->modelMatrix = rotator * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -65.0f)) * glm::scale(glm::vec3(5));
                planet->updateTargetPhysx();
                planet->render();
                for (Moon* child : planet->children) {
                    childRotator = child->createRotator(rotateTime);
                    child->modelMatrix = planet->modelMatrix * childRotator * glm::translate(child->distance) * glm::scale(glm::vec3(0.2f));
                    child->updateTargetPhysx();
                    child->render();
                }
            }
            else if (planet->name == "venus") {
                planet->modelMatrix = rotator * glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -120.0f)) * glm::scale(glm::vec3(15));
                planet->updateTargetPhysx();
                planet->render();
                for (Moon* child : planet->children) {
                    childRotator = child->createRotator(rotateTime);
                    child->modelMatrix = planet->modelMatrix * childRotator * glm::translate(child->distance) * glm::scale(glm::vec3(0.2f));
                    child->updateTargetPhysx();
                    child->render();
                }
            }
            else if (planet->name == "neptune") {
                planet->modelMatrix = rotator * glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -190.0f)) * glm::scale(glm::vec3(25));
                planet->updateTargetPhysx();
                planet->render();
                for (Moon* child : planet->children) {
                    childRotator = child->createRotator(rotateTime);
                    child->modelMatrix = planet->modelMatrix * childRotator * glm::translate(child->distance) * glm::scale(glm::vec3(0.2f));
                    child->updateTargetPhysx();
                    child->render();
                }
            }
            else if (planet->name == "hoth") {
                planet->modelMatrix = rotator * glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -280.0f)) * glm::scale(glm::vec3(28));
                planet->updateTargetPhysx();
                planet->render();
                for (Moon* child : planet->children) {
                    childRotator = child->createRotator(rotateTime);
                    child->modelMatrix = planet->modelMatrix * childRotator * glm::translate(child->distance) * glm::scale(glm::vec3(0.2f));
                    child->updateTargetPhysx();
                    child->render();
                }
            }
            else if (planet->name == "saturn") {
                planet->modelMatrix = rotator * glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -380.0f)) * glm::scale(glm::vec3(35));
                planet->updateTargetPhysx();
                planet->render();
                for (Moon* child : planet->children) {
                    childRotator = child->createRotator(rotateTime);
                    child->modelMatrix = planet->modelMatrix * childRotator * glm::translate(child->distance) * glm::scale(glm::vec3(0.2f));
                    child->updateTargetPhysx();
                    child->render();
                }
            }
        };

        for (Renderable* renderable : renderables) {
            drawObjectTexture(renderable->context, renderable->modelMatrix, renderable->textureId);
        }


        if (glm::distance(cubePos, glm::vec3(pxtr.p.x, pxtr.p.y, pxtr.p.z)) < 2)
        {
            points += 1;
            cubePos = glm::ballRand(100.f);
        }

        drawCube(glm::translate(cubePos));

        text(10, 10, "Points: " + std::to_string(points), 18);
        text(SCR_WIDTH - 170, SCR_HEIGHT - 25, "Time remaining: " + std::to_string(timeToEnd / 2), 18);


        drawObjectTexture(&circleContext, glm::translate(glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z)) * glm::toMat4(glm::inverse(glm::quat_cast(createCameraMatrix())))* glm::translate(glm::vec3(0.8f, 0.2f, -2.4f)) * glm::rotate((float)time, glm::vec3(0,0,1)) *glm::scale(glm::vec3(10)), circleTexture);
        drawObjectColor(&circleContext, glm::translate(glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z)) * glm::toMat4(glm::inverse(glm::quat_cast(createCameraMatrix())))* glm::translate(glm::vec3((pxtr.p.x*0.17f)/200.f, 0, (pxtr.p.z * 0.17f) / 200.f)) * glm::translate(glm::vec3(0.78f, 0.22f, -2.35f)) * glm::rotate((float)time, glm::vec3(0,0,1)) *glm::scale(glm::vec3(1)), glm::vec3(1,0,0));
        drawObjectColor(&circleContext, glm::translate(glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z)) * glm::toMat4(glm::inverse(glm::quat_cast(createCameraMatrix())))* glm::translate(glm::vec3((cubePos.x*0.17f)/200.f, 0, (cubePos.z * 0.17f) / 200.f)) * glm::translate(glm::vec3(0.78f, 0.22f, -2.35f)) * glm::rotate((float)time, glm::vec3(0,0,1)) *glm::scale(glm::vec3(1)), glm::vec3(1,1,0));

        float tmp1 = ns;
        if (tmp1 > 0) tmp1 = 0;
        tmp1 *= -1;
        float tmp2 = ns;
        if (tmp2 < 0) tmp2 = 0;

        drawObjectColor(programColorWB, &cylinderContext, glm::translate(glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z)) * glm::toMat4(glm::inverse(glm::quat_cast(createCameraMatrix()))) * glm::translate(glm::vec3(-1.05f, 0.1f, -2.4f)) * glm::scale(glm::vec3(0.03f, ((480.f -tmp2) * 0.4f) / 480.f, 0.03f)), glm::vec3(0, 0, 1));
        drawObjectColor(programColorWB, &cylinderContext, glm::translate(glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z)) * glm::toMat4(glm::inverse(glm::quat_cast(createCameraMatrix()))) * glm::translate(glm::vec3(-1.05f, 0.90f, -2.4f)) * glm::scale(glm::vec3(0.03f, (tmp1*0.4f)/ 980.f, 0.03f)), glm::vec3(1, 0, 0));
        drawObjectColor(programColorWB, &cylinderContext, glm::translate(glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z)) * glm::toMat4(glm::inverse(glm::quat_cast(createCameraMatrix()))) * glm::translate(glm::vec3(-1.05f, 0.90f, -2.4f)) * glm::scale(glm::vec3(0.03f, (-tmp2*0.4f)/480.f, 0.03f)), glm::vec3(1, 0, 0));
        drawObjectColor(programColorWB, &cylinderContext, glm::translate(glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z)) * glm::toMat4(glm::inverse(glm::quat_cast(createCameraMatrix()))) * glm::translate(glm::vec3(-1.05f, 1.7f, -2.4f)) * glm::scale(glm::vec3(0.03f, -((980.f - tmp1) * 0.4f) / 980.f, 0.03f)), glm::vec3(0, 1, 0));
    }
    else
    {
        text(SCR_WIDTH/2- 135, SCR_HEIGHT/2+20, "You have finished this round.", 18);
        text(SCR_WIDTH/2-95, SCR_HEIGHT/2-20, "Number of points: " + std::to_string(points), 18);
    }

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
    appLoadingTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    glEnable(GL_DEPTH_TEST);
    programColor = shaderLoader.CreateProgram("shaders/shader_color.vert", "shaders/shader_color.frag");
    programColorWB = shaderLoader.CreateProgram("shaders/shader_color_wb.vert", "shaders/shader_color_wb.frag");
    programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
    programTexBloom = shaderLoader.CreateProgram("shaders/shader_tex_bloom.vert", "shaders/shader_tex_bloom.frag");
    programBlur = shaderLoader.CreateProgram("shaders/gaussian.vert", "shaders/gaussian.frag");
    programFinal = shaderLoader.CreateProgram("shaders/final.vert", "shaders/final.frag");
    programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");
    programTexBloomSun = shaderLoader.CreateProgram("shaders/shader_sun.vert", "shaders/shader_sun.frag");

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

    oldtime = 0;
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

void onReshape(int width, int height)
{
    frustumscale = width / height;
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
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
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(600, 600);
    glutCreateWindow("Space Simulator");
    glewInit();

    init();
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouse);
    glutDisplayFunc(renderScene);
    glutIdleFunc(idle);
    glutReshapeFunc(onReshape);

    glutMainLoop();

    shutdown();

    return 0;
}
