#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

#include <vector>
#include <cstdio>

struct KeyFrame {
    glm::vec3 position;
    glm::quat rotation;
    double timeStamp;
};

struct Animation {
    std::vector<KeyFrame> keyframes;
    double time;
    bool finished;
    glm::vec3 position;
    glm::quat rotation;
};

void tickAnimation(double deltaTime, Animation *animation) {
    animation->time += deltaTime;

    auto nextIt = std::lower_bound(
        animation->keyframes.begin(),
        animation->keyframes.end(),
        animation->time,
        [](const KeyFrame& frame, float time) {return frame.timeStamp < time;}
    );
    int nextIndex = std::distance(animation->keyframes.begin(), nextIt);
    int currentIndex = nextIndex - 1;

    float timeStamp = animation->keyframes[currentIndex].timeStamp;
    KeyFrame nextKeyFrame = animation->keyframes[nextIndex];
    float nextTimeStamp = nextKeyFrame.timeStamp;
    float timeDiff = nextTimeStamp - timeStamp;
    float res;
    float t;
    if (timeDiff > 0) {
        t = (animation->time - timeStamp) / timeDiff;
    }

    //printf("Animation: Start: %f, End: %f, Progress: %f\n", timeStamp, nextTimeStamp, t);

    animation->position = glm::mix(animation->keyframes[currentIndex].position, nextKeyFrame.position, t);
    animation->rotation = glm::slerp(animation->keyframes[currentIndex].rotation, nextKeyFrame.rotation, t);
}

void testAnimation() {
    Animation anim;
    //anim.currentIndex = 0;
    
    anim.keyframes.push_back({
        glm::vec3(0.0, 0.0, 0.0),
        glm::quat(glm::radians(glm::vec3(0.0, 0.0, 0.0))),
        0.0
    });
    anim.keyframes.push_back({
        glm::vec3(0.0, 1.0, 1.0),
        glm::quat(glm::radians(glm::vec3(0.0, 0.0, 90.0))),
        10.0
    });
    anim.keyframes.push_back({
        glm::vec3(1.0, 0.0, 0.0),
        glm::quat(glm::radians(glm::vec3(0.0, 0.0, 45.0))),
        15.0
    });

    for (int i = 0; i < 15; i++) {
        tickAnimation(1.0, &anim);
        glm::vec3 rot = glm::degrees(glm::eulerAngles(anim.rotation));
        printf("position: %f %f %f, rotation: %f %f %f\n",
            anim.position.x, anim.position.y, anim.position.z,
            rot.x, rot.y, rot.z
        );
    }
}
