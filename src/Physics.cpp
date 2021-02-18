#include "Physics.h"

#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL; }

Physics::Physics(float gravity,
    PxSimulationFilterShader simulationFilterShader,
    PxSimulationEventCallback* simulationEventCallback)
{
    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);

    physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true);

    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -gravity, 0.0f);
    dispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = dispatcher;
    sceneDesc.filterShader = simulationFilterShader;
    sceneDesc.kineKineFilteringMode = PxPairFilteringMode::eKEEP; // So kin-kin contacts with be reported
    sceneDesc.staticKineFilteringMode = PxPairFilteringMode::eKEEP; // So static-kin constacts will be reported
    sceneDesc.simulationEventCallback = simulationEventCallback;
    scene = physics->createScene(sceneDesc);
}

Physics::~Physics()
{
    PX_RELEASE(scene);
    PX_RELEASE(dispatcher);
    PX_RELEASE(physics);
    PX_RELEASE(foundation);
}

void Physics::step(float dt)
{
    scene->simulate(dt);
    scene->fetchResults(true);
}