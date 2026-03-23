#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <array>
#include <algorithm>
#include <unordered_map>

#include "config.h"
#include "Engine/Mesh3D.hpp"

#define MAX_BONES 256

// Animation sampler: times + interpolated output values
struct AnimSampler {
    std::vector<float> times;
    // Values stored as vec4: translation/scale use .xyz, rotation uses .xyzw (glTF xyzw order)
    std::vector<glm::vec4> values;
    std::string interpolation; // "LINEAR", "STEP", "CUBICSPLINE"
};

// One channel: drives translation/rotation/scale of a single joint
struct AnimChannel {
    int jointIndex;   // index into joints
    std::string path; // "translation", "rotation", "scale"
    int samplerIndex;
};

struct SkinAnimation {
    std::string name;
    std::vector<AnimSampler> samplers;
    std::vector<AnimChannel> channels;
    float duration = 0.0f;
};

struct Joint {
    int nodeIndex;           // glTF node index
    int parentJoint = -1;    // -1 if root joint
    std::string name;
    glm::mat4 inverseBindMatrix{1.0f};
    // Current local transform — overwritten each frame by animation sampling
    glm::vec3 localPos{0.0f};
    glm::quat localRot{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 localScale{1.0f};
};

// Geometry and skeleton shared by all instances of the same model file.
struct SharedSkinnedGeometry {
    std::vector<SkinnedVertex> vertices;
    std::vector<uint32_t>      indices;
    glm::vec3 AA{0}, BB{0}, modelCenter{0};
    VkBuffer       vertexBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer       indexBuffer        = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory  = VK_NULL_HANDLE;
    std::vector<Joint>         joints;
    std::vector<SkinAnimation> animations;
    bool hasSkin      = false;
    int  testBoneIndex = -1;
    int  refCount      = 0;
};

class SkinnedMesh3D : public Mesh3D {
public:
    // Skinned-geometry cache (separate from Mesh3D::s_cache — different vertex layout)
    static std::unordered_map<std::string, SharedSkinnedGeometry*> s_cache;
    SharedSkinnedGeometry* skinnedSharedGeom = nullptr;

    // Typed vertex data (Mesh3D::m_vertices holds Vertex; this holds SkinnedVertex)
    std::vector<SkinnedVertex> m_skinnedVertices;

    // Per-frame persistently-mapped bone matrix SSBOs
    VkBuffer boneBuffers[MAX_FRAMES_IN_FLIGHT]{};
    VkDeviceMemory boneBufferMemories[MAX_FRAMES_IN_FLIGHT]{};
    void* boneMappedPtrs[MAX_FRAMES_IN_FLIGHT]{};

    // Descriptor pool owned by this mesh (keeps it isolated from scene pool)
    VkDescriptorPool boneDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet boneDescriptorSets[MAX_FRAMES_IN_FLIGHT]{};

    std::vector<Joint> joints;
    std::vector<glm::mat4> boneMatrices;

    std::vector<SkinAnimation> animations;
    int currentAnimation = 0;
    float animTime = 0.0f;
    bool looping = true;
    bool hasSkin = false;

    int testBoneIndex = -1;
    float testBonePhase = 0.0f;

    SkinnedMesh3D() = default;

    void init(const char* filename);
    void destroy();

    // Call once per frame before drawing to advance animation
    void updateAnimation(float dt);
    // Upload current boneMatrices to GPU for the given frame index
    void uploadBoneMatrices(int frameIndex);
    // Bind bone descriptor set (set=1) and draw
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout, int frameIndex);

    void updateModelMatrix();
    ModelBufferObject getModelMatrix();

    float animSpeed = 1.0f;

    // Capsule rigid body — alternative to Mesh3D::createRigidBody for character controllers
    void createCapsuleRigidBody(float mass, float radius = 0.3f, float height = 1.0f);

    void playAnimation(int index) {
        currentAnimation = std::clamp(index, 0, (int)animations.size() - 1);
        animTime = 0.0f;
    }
    void setLooping(bool loop) { looping = loop; }
    void setAnimSpeed(float speed) { animSpeed = speed; }
    float getAnimSpeed() const { return animSpeed; }

    int getJointCount()    const { return (int)joints.size(); }
    int getTestBoneIndex() const { return testBoneIndex; }
    int getAnimCount()     const { return (int)animations.size(); }
    float getAnimTime()    const { return animTime; }
    float getAnimDuration(int index) const {
        int i = std::clamp(index, 0, (int)animations.size() - 1);
        return animations.empty() ? 0.0f : animations[i].duration;
    }

private:
    void loadSkinnedModel(const char* filename);
    void createVertexBuffer();
    void createIndexBuffer();
    void createBoneBuffers();
    void computeJointMatrices();
};
