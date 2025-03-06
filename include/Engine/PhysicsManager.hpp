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
        Mesh3D *meshFloor;
        Camera *camera;

        std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
        std::unique_ptr<btCollisionDispatcher> dispatcher;
        std::unique_ptr<btBroadphaseInterface> overlappingPairCache;
        std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
        std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

        btRigidBody* floor;
        btRigidBody* body;
        btRigidBody* cameraBody;
        btRigidBody* meshFloorBody;
        
    public:
        btVector3 worldToPhysics(glm::vec3 pos) {
            return btVector3(pos.x, pos.y, pos.z);
        }
        glm::vec3 physicsToWorld(btVector3 pos) {
            return glm::vec3(pos.getX(),pos.getY(),pos.getZ()) / glm::vec3(1.0);
        }

        btRigidBody* createGroundPlane() {
            btCollisionShape* groundShape = new btBoxShape(btVector3(10000, 1, 10000));

            btTransform groundTransform;
            groundTransform.setIdentity();
            groundTransform.setOrigin(btVector3(0.0, -2.0, 0.0));

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
        btRigidBody* createMeshFloorBody() {
            glm::vec3 size = glm::vec3(abs(meshFloor->BB.x - meshFloor->AA.x), abs(meshFloor->BB.y - meshFloor->AA.y), abs(meshFloor->BB.z - meshFloor->AA.z));

            printf("Floor Size: %f %f %f\n", size.x, size.y, size.z);
            btCollisionShape* bodyShape = new btBoxShape(btVector3(size.x / 2.0, size.y / 2.0, size.z / 2.0));
            btTransform bodyTransform;
            bodyTransform.setIdentity();
            glm::vec3 pos = meshFloor->getPosition();
            bodyTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
            bodyTransform.setRotation(btQuaternion(meshFloor->getOrientation().x, meshFloor->getOrientation().y, meshFloor->getOrientation().z, meshFloor->getOrientation().w));

            printf("Size Floor: %f %f %f, pos: %f %f %f\n", size.x, size.y, size.z, pos.x, pos.y, pos.z);

            float mass = 0.0;
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

            mesh->createRigidBody(10.0, ColliderType::CONVEXHULL, 0.25);
            body = mesh->rigidBody;
            body->setActivationState(DISABLE_DEACTIVATION);
            body->setFriction(2.0f);
            dynamicsWorld->addRigidBody(body);

            cameraBody = camera->rigidBody;// createCameraBody();
            cameraBody->setActivationState(DISABLE_DEACTIVATION);
            //cameraBody->setDamping(0.95f, 0.0f);
            dynamicsWorld->addRigidBody(cameraBody);

            meshFloorBody = meshFloor->rigidBody; // createMeshFloorBody();
            meshFloorBody->setActivationState(DISABLE_DEACTIVATION);
            meshFloorBody->setFriction(2.0f);
            dynamicsWorld->addRigidBody(meshFloorBody);
        }

        PhysicsManager(Mesh3D *mesh, Camera *camera, Mesh3D *meshFloor) {
            this->mesh = mesh;
            this->camera = camera;
            this->meshFloor = meshFloor;
            init();
        }

        void syncMesh(Mesh3D *mesh, btRigidBody *body) {
            btVector3 pos = body->getInterpolationWorldTransform().getOrigin();
            btQuaternion rot = body->getInterpolationWorldTransform().getRotation();
            mesh->setOrientation( glm::quat(rot.getW(), rot.getX(), rot.getY(), rot.getZ()) );
            mesh->setPosition(physicsToWorld(pos));

            if (pos.getY() < -20.0) {
                btTransform tran;
                tran.setIdentity();
                tran.setOrigin(btVector3(0.0, 5.0, 0.0));

                body->setWorldTransform(tran);
                body->setAngularVelocity(btVector3(0.0, 0.0, 0.0));
                body->setLinearVelocity(btVector3(0.0, 0.0, 0.0));
            }
        }

        void process(float deltaTime) {
            dynamicsWorld->stepSimulation(deltaTime, 1.0);

            cameraBody->applyCentralImpulse(worldToPhysics(camera->getVelocity()));
            camera->resetVelocity();

            camera->setPosition(physicsToWorld(cameraBody->getInterpolationWorldTransform().getOrigin()));
            syncMesh(mesh, body);
            syncMesh(meshFloor, meshFloorBody);
            

            glm::vec3 pos = physicsToWorld(meshFloorBody->getWorldTransform().getOrigin());
            //printf("Position Floor: %f %f %f\n", pos.x, pos.y, pos.z);
        }
    };
}