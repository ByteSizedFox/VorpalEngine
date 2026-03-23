#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <array>
#include <algorithm>

#include "volk.h"
#include "config.h"
#include "Engine/Vertex.hpp"
#include "Engine/Node3D.hpp"
#include "Engine/Engine.hpp"

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
    int jointIndex;   // index into SkinnedMesh3D::joints
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

class SkinnedMesh3D : public Node3D {
public:
    std::vector<SkinnedVertex> m_vertices;
    std::vector<uint32_t> m_indices;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    // Per-frame persistently-mapped bone matrix SSBOs
    VkBuffer boneBuffers[MAX_FRAMES_IN_FLIGHT]{};
    VkDeviceMemory boneBufferMemories[MAX_FRAMES_IN_FLIGHT]{};
    void* boneMappedPtrs[MAX_FRAMES_IN_FLIGHT]{};

    // Descriptor pool owned by this mesh (keeps it isolated from scene pool)
    VkDescriptorPool boneDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet boneDescriptorSets[MAX_FRAMES_IN_FLIGHT]{};

    std::vector<Joint> joints;
    std::vector<glm::mat4> boneMatrices; // final skinning matrices to upload

    std::vector<SkinAnimation> animations;
    int currentAnimation = 0;
    float animTime = 0.0f;
    bool looping = true;
    bool hasSkin = false;

    // Per-instance test bone: randomly chosen non-root joint that gets a
    // sinusoidal rotation so skeletal animation is visually verifiable even
    // on models that have no embedded animation clips.
    int testBoneIndex = -1;
    float testBonePhase = 0.0f;

    float metallic = 0.0f;
    float roughness = 0.5f;
    std::string fileName;
    glm::vec3 AA{0.0f}, BB{0.0f}, modelCenter{0.0f};

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
