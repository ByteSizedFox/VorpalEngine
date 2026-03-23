#include "Engine/SkinnedMesh3D.hpp"
#include "Engine/Engine.hpp"

// tinygltf is already implemented in Engine.cpp
#include "tiny_gltf.h"

#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>
#include <cmath>

// ---- Vulkan buffer helpers (reuse engine helpers) -------------------------

void SkinnedMesh3D::createVertexBuffer() {
    VkDeviceSize size = sizeof(SkinnedVertex) * m_vertices.size();
    VkBuffer staging; VkDeviceMemory stagingMem;
    Memory::createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging, stagingMem);

    void* data;
    vkMapMemory(VK::device, stagingMem, 0, size, 0, &data);
    memcpy(data, m_vertices.data(), size);
    vkUnmapMemory(VK::device, stagingMem);

    Memory::createBuffer(size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    Memory::copyBuffer(staging, vertexBuffer, size);

    vkDestroyBuffer(VK::device, staging, nullptr);
    vkFreeMemory(VK::device, stagingMem, nullptr);
}

void SkinnedMesh3D::createIndexBuffer() {
    VkDeviceSize size = sizeof(uint32_t) * m_indices.size();
    VkBuffer staging; VkDeviceMemory stagingMem;
    Memory::createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging, stagingMem);

    void* data;
    vkMapMemory(VK::device, stagingMem, 0, size, 0, &data);
    memcpy(data, m_indices.data(), size);
    vkUnmapMemory(VK::device, stagingMem);

    Memory::createBuffer(size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    Memory::copyBuffer(staging, indexBuffer, size);

    vkDestroyBuffer(VK::device, staging, nullptr);
    vkFreeMemory(VK::device, stagingMem, nullptr);
}

void SkinnedMesh3D::createBoneBuffers() {
    VkDeviceSize size = sizeof(glm::mat4) * MAX_BONES;

    // Per-frame persistently-mapped SSBOs
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        Memory::createBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            boneBuffers[i], boneBufferMemories[i]);
        vkMapMemory(VK::device, boneBufferMemories[i], 0, size, 0, &boneMappedPtrs[i]);

        // Fill with identity matrices so un-animated bones don't corrupt geometry
        std::vector<glm::mat4> idents(MAX_BONES, glm::mat4(1.0f));
        memcpy(boneMappedPtrs[i], idents.data(), size);
    }

    // Each skinned mesh owns a tiny descriptor pool (2 sets, one per frame)
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(VK::device, &poolInfo, nullptr, &boneDescriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create bone descriptor pool");

    // Allocate descriptor sets from this pool using the global bone layout
    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
    layouts.fill(VK::boneDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = boneDescriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(VK::device, &allocInfo, boneDescriptorSets) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate bone descriptor sets");

    // Point each descriptor set at its frame's SSBO
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = boneBuffers[i];
        bufInfo.offset = 0;
        bufInfo.range  = size;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = boneDescriptorSets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufInfo;
        vkUpdateDescriptorSets(VK::device, 1, &write, 0, nullptr);
    }
}

// ---- Texture loading (mirrors the logic in Assets::loadModel) -------------

static int loadOrGetTexture(const tinygltf::Model& model, int textureIndex,
                            const std::string& nameSuffix, VkFormat fmt) {
    if (textureIndex < 0) return -1;
    int sourceIndex = model.textures[textureIndex].source;
    if (sourceIndex < 0 || sourceIndex >= (int)model.images.size()) return -1;

    const tinygltf::Image& image = model.images[sourceIndex];
    std::string texPath;
    bool isEmbedded = true;

    if (!image.name.empty())          texPath = image.name + nameSuffix;
    else                              texPath = "skinned_mat_" + std::to_string(textureIndex) + nameSuffix;
    if (!image.uri.empty())         { texPath = "assets/textures/" + image.uri; isEmbedded = false; }

    auto it = std::find(VK::g_texturePathList.begin(), VK::g_texturePathList.end(), texPath);
    if (it != VK::g_texturePathList.end())
        return (int)std::distance(VK::g_texturePathList.begin(), it);

    if (!isEmbedded && !Utils::fileExistsZip(texPath)) return -1;

    int id = (int)VK::g_texturePathList.size();
    Texture tex;
    tex.textureID = id;
    if (isEmbedded) tex.createFromGLTFImage(image, fmt);
    else            tex.createTextureImage(texPath.c_str());
    tex.createTextureImageView();
    VK::textureMap[texPath] = tex;
    VK::g_texturePathList.push_back(texPath);
    return id;
}

// ---- Main loader ----------------------------------------------------------

void SkinnedMesh3D::loadSkinnedModel(const char* filename) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    bool ret = false;

    std::string ext(filename);
    ext = ext.substr(ext.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::vector<char> fileData = Utils::readFileZip(filename);
    if (fileData.empty()) throw std::runtime_error(std::string("SkinnedMesh3D: file not found: ") + filename);

    if (ext == "glb")
        ret = loader.LoadBinaryFromMemory(&model, &err, &warn,
            reinterpret_cast<const unsigned char*>(fileData.data()), fileData.size());
    else {
        std::string json(fileData.begin(), fileData.end());
        ret = loader.LoadASCIIFromString(&model, &err, &warn, json.c_str(), json.size(), "");
    }
    if (!ret) throw std::runtime_error("TinyGLTF error: " + err);

    // --- Build joint map from first skin -----------------------------------
    std::unordered_map<int, int> nodeToJoint; // glTF node index -> joint index

    if (!model.skins.empty()) {
        hasSkin = true;
        const tinygltf::Skin& skin = model.skins[0];

        joints.resize(skin.joints.size());
        for (int j = 0; j < (int)skin.joints.size(); j++) {
            joints[j].nodeIndex = skin.joints[j];
            joints[j].parentJoint = -1;
            nodeToJoint[skin.joints[j]] = j;

            // Rest pose from node TRS
            const tinygltf::Node& node = model.nodes[skin.joints[j]];
            joints[j].name = node.name;
            if (node.translation.size() == 3)
                joints[j].localPos = {(float)node.translation[0], (float)node.translation[1], (float)node.translation[2]};
            if (node.rotation.size() == 4)
                joints[j].localRot = glm::quat((float)node.rotation[3], (float)node.rotation[0],
                                               (float)node.rotation[1], (float)node.rotation[2]);
            if (node.scale.size() == 3)
                joints[j].localScale = {(float)node.scale[0], (float)node.scale[1], (float)node.scale[2]};
        }

        // Infer parent joints from the node child lists
        for (int nodeIdx = 0; nodeIdx < (int)model.nodes.size(); nodeIdx++) {
            auto parentIt = nodeToJoint.find(nodeIdx);
            for (int childIdx : model.nodes[nodeIdx].children) {
                auto childIt = nodeToJoint.find(childIdx);
                if (childIt != nodeToJoint.end() && parentIt != nodeToJoint.end()) {
                    joints[childIt->second].parentJoint = parentIt->second;
                }
            }
        }


        // Load inverse bind matrices
        if (skin.inverseBindMatrices >= 0) {
            const tinygltf::Accessor&   acc = model.accessors[skin.inverseBindMatrices];
            const tinygltf::BufferView& bv  = model.bufferViews[acc.bufferView];
            const tinygltf::Buffer&     buf = model.buffers[bv.buffer];
            const float* ibmData = reinterpret_cast<const float*>(&buf.data[bv.byteOffset + acc.byteOffset]);
            for (int j = 0; j < (int)joints.size() && j < (int)acc.count; j++)
                memcpy(&joints[j].inverseBindMatrix, ibmData + j * 16, sizeof(glm::mat4));
        }

        // Initialise bone matrices to identity
        boneMatrices.assign(joints.size(), glm::mat4(1.0f));
        computeJointMatrices(); // compute rest pose
    }

    // --- Load animations ---------------------------------------------------
    for (const auto& anim : model.animations) {
        SkinAnimation sa;
        sa.name = anim.name;

        for (const auto& s : anim.samplers) {
            AnimSampler as;
            as.interpolation = s.interpolation;

            // Times (input)
            {
                const tinygltf::Accessor&   acc = model.accessors[s.input];
                const tinygltf::BufferView& bv  = model.bufferViews[acc.bufferView];
                const tinygltf::Buffer&     buf = model.buffers[bv.buffer];
                const float* t = reinterpret_cast<const float*>(&buf.data[bv.byteOffset + acc.byteOffset]);
                as.times.assign(t, t + acc.count);
                if (!as.times.empty()) sa.duration = std::max(sa.duration, as.times.back());
            }
            // Values (output)
            {
                const tinygltf::Accessor&   acc = model.accessors[s.output];
                const tinygltf::BufferView& bv  = model.bufferViews[acc.bufferView];
                const tinygltf::Buffer&     buf = model.buffers[bv.buffer];
                const float* v = reinterpret_cast<const float*>(&buf.data[bv.byteOffset + acc.byteOffset]);
                int components = (acc.type == TINYGLTF_TYPE_VEC4) ? 4 : 3;
                as.values.resize(acc.count);
                for (size_t i = 0; i < acc.count; i++) {
                    as.values[i] = glm::vec4(
                        v[i * components + 0],
                        v[i * components + 1],
                        v[i * components + 2],
                        (components == 4) ? v[i * components + 3] : 0.0f);
                }
            }
            sa.samplers.push_back(std::move(as));
        }

        for (const auto& ch : anim.channels) {
            auto it = nodeToJoint.find(ch.target_node);
            if (it == nodeToJoint.end()) continue;
            AnimChannel ac;
            ac.jointIndex   = it->second;
            ac.path         = ch.target_path;
            ac.samplerIndex = ch.sampler;
            sa.channels.push_back(ac);
        }

        animations.push_back(std::move(sa));
    }

    // --- Load geometry (with JOINTS_0 / WEIGHTS_0) -------------------------
    glm::mat4 rootMat(1.0f);
    if (!model.scenes.empty() && !model.scenes[model.defaultScene].nodes.empty()) {
        int rootIdx = model.scenes[model.defaultScene].nodes[0];
        rootMat = Assets::getLocalMatrix(model.nodes[rootIdx]);
    }

    glm::vec3 minV(99999.0f), maxV(-99999.0f);

    for (const auto& mesh : model.meshes) {
        for (const auto& prim : mesh.primitives) {
            if (prim.attributes.find("POSITION") == prim.attributes.end()) continue;

            // Resolve material / textures
            int textureID = 0, normalID = -1, mrID = -1;
            if (prim.material >= 0 && prim.material < (int)model.materials.size()) {
                const auto& mat = model.materials[prim.material];
                textureID = loadOrGetTexture(model,
                    mat.pbrMetallicRoughness.baseColorTexture.index, "", VK_FORMAT_R8G8B8A8_SRGB);
                normalID  = loadOrGetTexture(model,
                    mat.normalTexture.index, "_n", VK_FORMAT_R8G8B8A8_SRGB);
                mrID      = loadOrGetTexture(model,
                    mat.pbrMetallicRoughness.metallicRoughnessTexture.index, "_mr", VK_FORMAT_R8G8B8A8_UNORM);
            }

            // Position
            const auto& posAcc = model.accessors[prim.attributes.at("POSITION")];
            const auto& posBV  = model.bufferViews[posAcc.bufferView];
            const float* posData = reinterpret_cast<const float*>(
                &model.buffers[posBV.buffer].data[posBV.byteOffset + posAcc.byteOffset]);
            size_t posStride = posAcc.ByteStride(posBV) ? posAcc.ByteStride(posBV)/sizeof(float) : 3;

            // Normal
            const float* normalData = nullptr; size_t normalStride = 3;
            if (prim.attributes.count("NORMAL")) {
                const auto& acc = model.accessors[prim.attributes.at("NORMAL")];
                const auto& bv  = model.bufferViews[acc.bufferView];
                normalData   = reinterpret_cast<const float*>(&model.buffers[bv.buffer].data[bv.byteOffset + acc.byteOffset]);
                normalStride = acc.ByteStride(bv) ? acc.ByteStride(bv)/sizeof(float) : 3;
            }

            // TexCoord
            const float* texData = nullptr; size_t texStride = 2;
            if (prim.attributes.count("TEXCOORD_0")) {
                const auto& acc = model.accessors[prim.attributes.at("TEXCOORD_0")];
                const auto& bv  = model.bufferViews[acc.bufferView];
                texData   = reinterpret_cast<const float*>(&model.buffers[bv.buffer].data[bv.byteOffset + acc.byteOffset]);
                texStride = acc.ByteStride(bv) ? acc.ByteStride(bv)/sizeof(float) : 2;
            }

            // Tangent
            const float* tangentData = nullptr; size_t tangentStride = 4;
            if (prim.attributes.count("TANGENT")) {
                const auto& acc = model.accessors[prim.attributes.at("TANGENT")];
                const auto& bv  = model.bufferViews[acc.bufferView];
                tangentData   = reinterpret_cast<const float*>(&model.buffers[bv.buffer].data[bv.byteOffset + acc.byteOffset]);
                tangentStride = acc.ByteStride(bv) ? acc.ByteStride(bv)/sizeof(float) : 4;
            }

            // JOINTS_0 (uint8 or uint16)
            const uint8_t*  jointsU8  = nullptr;
            const uint16_t* jointsU16 = nullptr;
            size_t jointsStride = 4;
            int jointsCompType = 0;
            if (prim.attributes.count("JOINTS_0")) {
                const auto& acc = model.accessors[prim.attributes.at("JOINTS_0")];
                const auto& bv  = model.bufferViews[acc.bufferView];
                const uint8_t* raw = &model.buffers[bv.buffer].data[bv.byteOffset + acc.byteOffset];
                jointsCompType = acc.componentType;
                if (jointsCompType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    jointsU8     = raw;
                    jointsStride = acc.ByteStride(bv) ? acc.ByteStride(bv) : 4;
                } else {
                    jointsU16    = reinterpret_cast<const uint16_t*>(raw);
                    jointsStride = acc.ByteStride(bv) ? acc.ByteStride(bv)/sizeof(uint16_t) : 4;
                }
            }

            // WEIGHTS_0
            const float* weightsData = nullptr; size_t weightsStride = 4;
            if (prim.attributes.count("WEIGHTS_0")) {
                const auto& acc = model.accessors[prim.attributes.at("WEIGHTS_0")];
                const auto& bv  = model.bufferViews[acc.bufferView];
                weightsData   = reinterpret_cast<const float*>(&model.buffers[bv.buffer].data[bv.byteOffset + acc.byteOffset]);
                weightsStride = acc.ByteStride(bv) ? acc.ByteStride(bv)/sizeof(float) : 4;
            }

            // Build vertices
            size_t vertCount = posAcc.count;
            size_t baseIndex = m_vertices.size();
            m_vertices.reserve(m_vertices.size() + vertCount);

            for (size_t v = 0; v < vertCount; v++) {
                SkinnedVertex sv{};

                sv.pos = glm::vec3(posData[v*posStride], posData[v*posStride+1], posData[v*posStride+2]);
                sv.pos = glm::vec3(rootMat * glm::vec4(sv.pos, 1.0f));

                minV = glm::min(minV, sv.pos); maxV = glm::max(maxV, sv.pos);
                modelCenter += sv.pos;

                if (texData)    { sv.texCoord = {texData[v*texStride], texData[v*texStride+1]}; }
                if (normalData) { sv.normal = glm::vec3(normalData[v*normalStride], normalData[v*normalStride+1], normalData[v*normalStride+2]);
                                  sv.normal = glm::mat3(rootMat) * sv.normal; }
                else              sv.normal = {0.0f, 1.0f, 0.0f};

                if (tangentData) {
                    sv.tangent = {tangentData[v*tangentStride], tangentData[v*tangentStride+1],
                                  tangentData[v*tangentStride+2], tangentData[v*tangentStride+3]};
                    sv.tangent = rootMat * sv.tangent;
                } else {
                    sv.tangent = (std::abs(sv.normal.y) < 0.99f) ?
                        glm::vec4(glm::normalize(glm::cross(glm::vec3(0,1,0), sv.normal)), 1.0f) :
                        glm::vec4(glm::normalize(glm::cross(glm::vec3(1,0,0), sv.normal)), 1.0f);
                }

                sv.textureID          = textureID;
                sv.normalID           = normalID;
                sv.metallicRoughnessID = mrID;

                // Bone influences
                if (jointsU8) {
                    sv.jointIndices = { jointsU8[v*jointsStride+0], jointsU8[v*jointsStride+1],
                                        jointsU8[v*jointsStride+2], jointsU8[v*jointsStride+3] };
                } else if (jointsU16) {
                    sv.jointIndices = { jointsU16[v*jointsStride+0], jointsU16[v*jointsStride+1],
                                        jointsU16[v*jointsStride+2], jointsU16[v*jointsStride+3] };
                }
                if (weightsData) {
                    sv.jointWeights = { weightsData[v*weightsStride+0], weightsData[v*weightsStride+1],
                                        weightsData[v*weightsStride+2], weightsData[v*weightsStride+3] };
                }

                // Clamp joint indices to valid range
                for (int k = 0; k < 4; k++) {
                    int& idx = (&sv.jointIndices.x)[k];
                    if (idx < 0 || idx >= MAX_BONES) idx = 0;
                }

                m_vertices.push_back(sv);
            }

            // Indices
            if (prim.indices >= 0) {
                const auto& idxAcc = model.accessors[prim.indices];
                const auto& idxBV  = model.bufferViews[idxAcc.bufferView];
                const uint8_t* idxRaw = &model.buffers[idxBV.buffer].data[idxBV.byteOffset + idxAcc.byteOffset];
                for (size_t i = 0; i < idxAcc.count; i++) {
                    uint32_t idx = 0;
                    if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                        idx = reinterpret_cast<const uint32_t*>(idxRaw)[i];
                    else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                        idx = reinterpret_cast<const uint16_t*>(idxRaw)[i];
                    else
                        idx = idxRaw[i];
                    m_indices.push_back((uint32_t)(baseIndex + idx));
                }
            }
        }
    }

    if (!m_vertices.empty()) {
        AA = minV; BB = maxV;
        modelCenter /= (float)m_vertices.size();
    }
}

// ---- Public interface ----------------------------------------------------

void SkinnedMesh3D::init(const char* filename) {
    fileName = filename;
    loadSkinnedModel(filename);
    updateModelMatrix();
    createVertexBuffer();
    createIndexBuffer();
    createBoneBuffers();
    // Pick the first spine bone (by name), skipping any IK helpers.
    // Falls back to joint 0 if no spine bone is found.
    if (hasSkin && !joints.empty()) {
        auto nameContains = [&](int ji, const char* substr) {
            std::string lower(joints[ji].name.size(), '\0');
            std::transform(joints[ji].name.begin(), joints[ji].name.end(), lower.begin(), ::tolower);
            return lower.find(substr) != std::string::npos;
        };

        testBoneIndex = 0; // fallback
        for (int j = 0; j < (int)joints.size(); j++) {
            if (nameContains(j, "spine") &&
                !nameContains(j, "ik") &&
                !nameContains(j, "pole") &&
                !nameContains(j, "target") &&
                !nameContains(j, "ctrl")) {
                testBoneIndex = j;
                break;
            }
        }
        testBonePhase = (float)(rand() % 628) / 100.0f;
    }

}

void SkinnedMesh3D::destroy() {
    vkDestroyBuffer(VK::device, indexBuffer, nullptr);
    vkFreeMemory(VK::device, indexBufferMemory, nullptr);
    vkDestroyBuffer(VK::device, vertexBuffer, nullptr);
    vkFreeMemory(VK::device, vertexBufferMemory, nullptr);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(VK::device, boneBuffers[i], nullptr);
        vkFreeMemory(VK::device, boneBufferMemories[i], nullptr);
    }

    if (boneDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(VK::device, boneDescriptorPool, nullptr);
        boneDescriptorPool = VK_NULL_HANDLE;
    }
}

void SkinnedMesh3D::updateAnimation(float dt) {
    if (!hasSkin) return;

    animTime += dt * animSpeed;

    if (!animations.empty()) {
        SkinAnimation& anim = animations[currentAnimation];
        if (anim.duration > 0.0f) {
            if (animTime > anim.duration)
                animTime = looping ? std::fmod(animTime, anim.duration) : anim.duration;

            // Sample each animation channel
            for (const auto& channel : anim.channels) {
                if (channel.jointIndex < 0 || channel.jointIndex >= (int)joints.size()) continue;
                if (channel.samplerIndex < 0 || channel.samplerIndex >= (int)anim.samplers.size()) continue;
                const AnimSampler& sampler = anim.samplers[channel.samplerIndex];
                if (sampler.times.size() < 2) continue;

                float t = glm::clamp(animTime, sampler.times.front(), sampler.times.back());

                int idx = (int)(std::lower_bound(sampler.times.begin(), sampler.times.end(), t)
                                - sampler.times.begin()) - 1;
                idx = glm::clamp(idx, 0, (int)sampler.times.size() - 2);

                float t0 = sampler.times[idx];
                float t1 = sampler.times[idx + 1];
                float blend = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
                blend = glm::clamp(blend, 0.0f, 1.0f);

                // For CUBICSPLINE each keyframe has 3 output values:
                // [in-tangent, value, out-tangent]. Pick the value (offset 1).
                bool cubic = (sampler.interpolation == "CUBICSPLINE");
                int stride = cubic ? 3 : 1;
                int vi  = idx  * stride + (cubic ? 1 : 0);
                int vi1 = (idx + 1) * stride + (cubic ? 1 : 0);
                if (vi >= (int)sampler.values.size() || vi1 >= (int)sampler.values.size()) continue;

                Joint& joint = joints[channel.jointIndex];

                if (channel.path == "translation") {
                    joint.localPos = glm::mix(glm::vec3(sampler.values[vi]),
                                              glm::vec3(sampler.values[vi1]), blend);
                } else if (channel.path == "rotation") {
                    glm::quat q0(sampler.values[vi].w,  sampler.values[vi].x,
                                 sampler.values[vi].y,  sampler.values[vi].z);
                    glm::quat q1(sampler.values[vi1].w, sampler.values[vi1].x,
                                 sampler.values[vi1].y, sampler.values[vi1].z);
                    joint.localRot = glm::slerp(q0, q1, blend);
                } else if (channel.path == "scale") {
                    joint.localScale = glm::mix(glm::vec3(sampler.values[vi]),
                                                glm::vec3(sampler.values[vi1]), blend);
                }
            }
        }
    }

    // Apply test bone rotation only when there is no real animation data,
    // so we don't clobber animation-driven joints.
    if (animations.empty() && testBoneIndex >= 0 && testBoneIndex < (int)joints.size()) {
        float angle = std::sin(animTime * 1.5f + testBonePhase) * glm::radians(5.0f);
        joints[testBoneIndex].localRot = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    computeJointMatrices();
}

void SkinnedMesh3D::computeJointMatrices() {
    std::vector<glm::mat4> globalTransforms(joints.size(), glm::mat4(1.0f));
    boneMatrices.resize(joints.size());

    // Joints must be in topological order (parents before children), which
    // glTF exporters virtually always guarantee.
    for (int i = 0; i < (int)joints.size(); i++) {
        glm::mat4 local = glm::translate(glm::mat4(1.0f), joints[i].localPos)
                        * glm::toMat4(joints[i].localRot)
                        * glm::scale(glm::mat4(1.0f), joints[i].localScale);

        if (joints[i].parentJoint < 0)
            globalTransforms[i] = local;
        else
            globalTransforms[i] = globalTransforms[joints[i].parentJoint] * local;

        boneMatrices[i] = globalTransforms[i] * joints[i].inverseBindMatrix;
    }
}

void SkinnedMesh3D::uploadBoneMatrices(int frameIndex) {
    size_t count = std::min((int)boneMatrices.size(), MAX_BONES);
    if (count > 0)
        memcpy(boneMappedPtrs[frameIndex], boneMatrices.data(), count * sizeof(glm::mat4));
}

void SkinnedMesh3D::updateModelMatrix() {
    modelMatrix = glm::translate(glm::mat4(1.0f), position * glm::vec3(WORLD_SCALE))
                * glm::toMat4(orientation)
                * glm::scale(glm::mat4(1.0f), scale);
}

ModelBufferObject SkinnedMesh3D::getModelMatrix() {
    if (isDirty) { updateModelMatrix(); isDirty = false; }
    ModelBufferObject mbo{};
    mbo.model = modelMatrix;
    mbo.enableNormal = 1;
    mbo.metallic  = metallic;
    mbo.roughness = roughness;
    return mbo;
}

void SkinnedMesh3D::draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout, int frameIndex) {
    if (vertexBuffer == VK_NULL_HANDLE || indexBuffer == VK_NULL_HANDLE) return;

    // Bind bone SSBO at descriptor set 1
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout, 1, 1, &boneDescriptorSets[frameIndex], 0, nullptr);

    ModelBufferObject mbo = getModelMatrix();
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(ModelBufferObject), &mbo);

    VkBuffer vbs[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vbs, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, (uint32_t)m_indices.size(), 1, 0, 0, 0);
}
