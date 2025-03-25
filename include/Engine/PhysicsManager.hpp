// physics
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"

#include <iostream>
#include <memory>

// engine includes
#include "Mesh3D.hpp"
#include "Camera.hpp"
#include "DebugMesh.hpp"

inline btVector3 worldToPhysics(glm::vec3 pos) {
    return btVector3(pos.x, pos.y, pos.z);
}
inline glm::vec3 physicsToWorld(btVector3 pos) {
    return glm::vec3(pos.getX(),pos.getY(),pos.getZ()) / glm::vec3(1.0);
}

struct BulletContactResultCallback : public btCollisionWorld::ContactResultCallback {
public:
    bool triggered = false;

    bool needsCollision(btBroadphaseProxy* proxy0) const override {
        return true;
    }

    btScalar addSingleResult(btManifoldPoint &cp, const btCollisionObjectWrapper *colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper *colObj1Wrap, int partId1, int index1) override {
        if (colObj0Wrap->getCollisionObject() == colObj1Wrap->getCollisionObject()) {
            return 0;
        }

        btVector3 ptA = cp.getPositionWorldOnA();
        btVector3 ptB = cp.getPositionWorldOnB();
        double distance = cp.getDistance();

        triggered = true;
        
        return 0;
    }
};

namespace Physics {
    class PhysicsManager {
        std::vector<Mesh3D*> meshes;
        Camera *camera;

        std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
        std::unique_ptr<btCollisionDispatcher> dispatcher;
        std::unique_ptr<btBroadphaseInterface> overlappingPairCache;
        std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
        
    public:
        // need public access to draw in render loop
        MyDebugDrawer* debugDrawer = nullptr;
        std::shared_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

        void init() {
            collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
            dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfiguration.get());
            overlappingPairCache = std::make_unique<btDbvtBroadphase>();
            solver = std::make_unique<btSequentialImpulseConstraintSolver>();

            // Create dynamics world
            dynamicsWorld = std::make_shared<btDiscreteDynamicsWorld> (
                dispatcher.get(), 
                overlappingPairCache.get(), 
                solver.get(), 
                collisionConfiguration.get()
            );

            // load debug renderer
#ifdef DRAW_DEBUG
            debugDrawer = new MyDebugDrawer();
            dynamicsWorld->setDebugDrawer(debugDrawer);
#endif

            // Set gravity
            dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

            for (Mesh3D *mesh : meshes) {
                if (!mesh->hasPhysics) { // skip non-physics meshes
                    continue;
                }
                mesh->rigidBody->setActivationState(DISABLE_DEACTIVATION);
                mesh->rigidBody->setFriction(2.0f);
                dynamicsWorld->addRigidBody(mesh->rigidBody);
            }
            
            camera->rigidBody->setActivationState(DISABLE_DEACTIVATION);
            dynamicsWorld->addRigidBody(camera->rigidBody);
        }

        PhysicsManager(std::vector<Mesh3D *> &meshes, Camera *camera) {
            this->meshes = meshes;
            this->camera = camera;
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

            // draw debug
#ifdef DRAW_DEBUG
            dynamicsWorld->debugDrawWorld();
#endif

            //float velY = camera->rigidBody->getLinearVelocity().getY();
            //camera->rigidBody->setLinearVelocity(worldToPhysics(camera->getVelocity()) + btVector3(0.0, velY, 0.0));
            //camera->resetVelocity();

            camera->setPosition(physicsToWorld(camera->rigidBody->getInterpolationWorldTransform().getOrigin()) + glm::vec3(0.0, 0.5, 0.0));
            for (Mesh3D* mesh : meshes) {
                if (!mesh->hasPhysics) { // skip non-physics meshes
                    continue;
                }
                syncMesh(mesh, mesh->rigidBody);
            }
        }
    };
}