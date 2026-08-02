#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Extension/Extension.h"
#include "Math/Matrix4.h"
#include "Resource/SkinnedModelLoader.h"

struct AnimatorState
{
    std::shared_ptr<Skeleton> skeleton;
    std::shared_ptr<std::vector<AnimationClip>> clips;

    int currentClip = -1;
    float time = 0.0f;
    float speed = 1.0f;
    bool playing = false;
    bool looping = true;

    int previousClip = -1;
    float previousTime = 0.0f;
    float blendElapsed = 0.0f;
    float blendDuration = 0.15f;

    std::vector<Matrix4> boneMatrices;
    std::vector<Matrix4> nodeTransforms;

    const Matrix4* GetAttachmentTransform(const std::string& nodeName) const
    {
        if (!skeleton)
            return nullptr;

        const auto it = skeleton->nodeNameToIndex.find(nodeName);
        if (it == skeleton->nodeNameToIndex.end())
            return nullptr;

        const size_t index = static_cast<size_t>(it->second);
        return index < nodeTransforms.size() ? &nodeTransforms[index] : nullptr;
    }

    bool HasPose() const
    {
        return !nodeTransforms.empty();
    }
};

struct AnimatorComponent
{
    std::shared_ptr<AnimatorState> state;
};

class Animator
{
public:
    static void Play(AnimatorState& state, const std::string& clipName, bool loop = true)
    {
        if (!state.clips)
            return;

        for (size_t i = 0; i < state.clips->size(); i++)
        {
            if ((*state.clips)[i].name == clipName)
            {
                if (state.currentClip >= 0 && state.currentClip != static_cast<int>(i))
                {
                    state.previousClip = state.currentClip;
                    state.previousTime = state.time;
                    state.blendElapsed = 0.0f;
                }

                state.currentClip = static_cast<int>(i);
                state.time = 0.0f;
                state.playing = true;
                state.looping = loop;
                return;
            }
        }

        OGL_WARNING("Animator | Clip not found: " << clipName)
    }

    static void Stop(AnimatorState& state)
    {
        state.playing = false;
    }

    static void SetSpeed(AnimatorState& state, float speed)
    {
        state.speed = speed;
    }
};
