#pragma once

#include <stdio.h>
#include <vector>
#include "Mesh3D.hpp"

class Scene {
private:
    std::vector<Mesh3D> meshes;

public:
    virtual void init() {
        printf("WARNING: Unimplemented Scene!\n");
    }
    virtual void loop() {
        printf("WARNING: Unimplemented Loop!\n");
    }
    virtual void drawFrame() {
        printf("WARNING: Unimplemented DrawFrame!\n");
    }

    void add_object(Mesh3D mesh) {
        meshes.push_back(mesh);
    }
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t currentFrame) {
        for (Mesh3D mesh : meshes) {
            mesh.draw(commandBuffer, pipelineLayout, currentFrame);
        }
    }
};