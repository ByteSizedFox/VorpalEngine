// physics
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"

#include <iostream>

// engine includes
#include "Mesh3D.hpp"
#include "Camera.hpp"


namespace Physics {
    class PhysicsManager {
        Mesh3D *mesh;
        Camera *camera;

        std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
        std::unique_ptr<btCollisionDispatcher> dispatcher;
        std::unique_ptr<btBroadphaseInterface> overlappingPairCache;
        std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
        std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

        btRigidBody* floor;
        btRigidBody* body;
        btRigidBody* cameraBody;

    public:
        btVector3 worldToPhysics(glm::vec3 pos) {
            return btVector3(pos.x * 100.0, pos.y * 100.0, pos.z * 100.0);
        }
        glm::vec3 physicsToWorld(btVector3 pos) {
            return glm::vec3(pos.getX(),pos.getY(),pos.getZ()) / glm::vec3(100.0);
        }

        btRigidBody* createGroundPlane() {
            btCollisionShape* groundShape = new btBoxShape(btVector3(10000, 1, 10000));

            btTransform groundTransform;
            groundTransform.setIdentity();
            groundTransform.setOrigin(btVector3(0.0, -1.5, 0.0));

            btVector3 localInertia(0, 0, 0);
            groundShape->calculateLocalInertia(0.0, localInertia);
            btRigidBody* groundBody = new btRigidBody(
                btRigidBody::btRigidBodyConstructionInfo(
                    0, new btDefaultMotionState(groundTransform), groundShape, localInertia
                )
            );
            groundBody->setActivationState(DISABLE_DEACTIVATION);
            groundBody->setFriction(1.0f);
            groundBody->setRestitution(0.0f);
            
            dynamicsWorld->addRigidBody(groundBody);
            return groundBody;
        }
        btRigidBody* createMeshBody() {
            btCollisionShape* bodyShape = new btBoxShape(btVector3(1, 1, 1));

            btTransform bodyTransform;
            bodyTransform.setIdentity();
            glm::vec3 pos = mesh->getPosition();
            bodyTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
            bodyTransform.setRotation(btQuaternion(mesh->getOrientation().x, mesh->getOrientation().y, mesh->getOrientation().z, mesh->getOrientation().w));

            float mass = 0.01;
            btVector3 localInertia(0, 0, 0);
            bodyShape->calculateLocalInertia(mass, localInertia);
            btDefaultMotionState* motionState = new btDefaultMotionState(bodyTransform);

            bodyShape->setMargin(0.001);

            btRigidBody* meshBody = new btRigidBody(
                btRigidBody::btRigidBodyConstructionInfo(
                    mass, motionState, bodyShape, localInertia
                )
            );

            dynamicsWorld->addRigidBody(meshBody);
            return meshBody;
        }
        btRigidBody* createCameraBody() {
            btCollisionShape* bodyShape = new btCapsuleShape(1.0, 2.0);
            btTransform bodyTransform;
            bodyTransform.setIdentity();
            //bodyTransform.setRotation(btQuaternion(0.1,0.0,0.0,1.0));

            float mass = 1.0;
            btVector3 localInertia(0, 0, 0);
            bodyShape->calculateLocalInertia(mass, localInertia);
            btDefaultMotionState* motionState = new btDefaultMotionState(bodyTransform);

            bodyShape->setMargin(0.001);

            btRigidBody* meshBody = new btRigidBody(
                btRigidBody::btRigidBodyConstructionInfo(
                    mass, motionState, bodyShape, localInertia
                )
            );

            meshBody->setFriction(2.0f);
            meshBody->setRestitution(0.0f);
            meshBody->setAngularFactor(btVector3(0,0,0));
            //meshBody->setLinear(btVector3(0,1,0));

            dynamicsWorld->addRigidBody(meshBody);
            return meshBody;
        }
        void init() {
            collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
            dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfiguration.get());
            overlappingPairCache = std::make_unique<btDbvtBroadphase>();
            solver = std::make_unique<btSequentialImpulseConstraintSolver>();

            // Create dynamics world
            dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
                dispatcher.get(), 
                overlappingPairCache.get(), 
                solver.get(), 
                collisionConfiguration.get()
            );

            // Set gravity
            dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

            floor = createGroundPlane();
            floor->setActivationState(DISABLE_DEACTIVATION);

            body = createMeshBody();
            body->setActivationState(DISABLE_DEACTIVATION);
            body->setFriction(2.0f);
            //body->setCcdMotionThreshold(0.0001f);
            //body->setCcdSweptSphereRadius(0.2f);

            cameraBody = createCameraBody();
            cameraBody->setActivationState(DISABLE_DEACTIVATION);
        }

        PhysicsManager(Mesh3D *mesh, Camera *camera) {
            this->mesh = mesh;
            this->camera = camera;
            init();
        }

        void syncMesh(Mesh3D *mesh, btRigidBody *body) {
            btVector3 pos = body->getWorldTransform().getOrigin();
            btQuaternion rot = body->getWorldTransform().getRotation();
            mesh->setOrientation( glm::quat(rot.getW(), rot.getX(), rot.getY(), rot.getZ()) );
            mesh->setPosition(physicsToWorld(pos));
        }

        void process(float deltaTime) {
            dynamicsWorld->stepSimulation(deltaTime, 1.0);

            //btTransform cameraTransform;
            //cameraTransform.setIdentity();
            //cameraTransform.setOrigin(worldToPhysics(camera->getPosition()));
            //cameraBody->setWorldTransform(cameraTransform);

            cameraBody->applyCentralForce(worldToPhysics(camera->getVelocity()));
            camera->resetVelocity();

            camera->setPosition(physicsToWorld(cameraBody->getWorldTransform().getOrigin()));
            //btQuaternion rot = cameraBody->getWorldTransform().getRotation();
            //camera->setOrientation(glm::quat(rot.getX(), rot.getY(), rot.getZ(), rot.getW()));

            syncMesh(mesh, body);
        }
    };
}