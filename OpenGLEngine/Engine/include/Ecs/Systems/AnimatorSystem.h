#pragma once

#include <cmath>
#include <unordered_set>
#include <vector>
#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/VisibilityComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Math/Matrix4.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Resource/SkinnedModelLoader.h"

class AnimatorSystem : public IEcsSystem
{
public:
    bool skipInvisible = true;

    void Run(EcsWorld& world, float deltaTime) override
    {
        processed.clear();
        visibleStates.clear();

        auto& visibilities = world.GetPool<VisibilityComponent>();

        for (auto& [entityId, animatorComponent] : world.GetPool<AnimatorComponent>())
        {
            AnimatorState* state = animatorComponent.state.get();
            if (!state)
                continue;

            if (!visibilities.Has(entityId) || visibilities.Get(entityId).visibleInFrustum)
                visibleStates.insert(state);
        }

        for (auto& [entityId, animatorComponent] : world.GetPool<AnimatorComponent>())
        {
            AnimatorState* state = animatorComponent.state.get();
            if (!state || !processed.insert(state).second)
                continue;

            if (skipInvisible && state->HasPose() && visibleStates.count(state) == 0)
                continue;

            Update(*state, deltaTime);
        }
    }

private:
    struct LocalPose
    {
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;
    };

    static Vector3 SampleVector(const std::vector<VectorKey>& keys, float time, const Vector3& fallback)
    {
        if (keys.empty())
            return fallback;
        if (keys.size() == 1 || time <= keys.front().time)
            return keys.front().value;
        if (time >= keys.back().time)
            return keys.back().value;

        for (size_t i = 0; i + 1 < keys.size(); i++)
        {
            if (time >= keys[i].time && time <= keys[i + 1].time)
            {
                const float span = keys[i + 1].time - keys[i].time;
                const float t = span > 0.0001f ? (time - keys[i].time) / span : 0.0f;
                return keys[i].value + (keys[i + 1].value - keys[i].value) * t;
            }
        }

        return keys.back().value;
    }

    static Quaternion SampleQuaternion(const std::vector<QuaternionKey>& keys, float time, const Quaternion& fallback)
    {
        if (keys.empty())
            return fallback;
        if (keys.size() == 1 || time <= keys.front().time)
            return keys.front().value;
        if (time >= keys.back().time)
            return keys.back().value;

        for (size_t i = 0; i + 1 < keys.size(); i++)
        {
            if (time >= keys[i].time && time <= keys[i + 1].time)
            {
                const float span = keys[i + 1].time - keys[i].time;
                const float t = span > 0.0001f ? (time - keys[i].time) / span : 0.0f;
                return Quaternion::Slerp(keys[i].value, keys[i + 1].value, t);
            }
        }

        return keys.back().value;
    }

    static LocalPose SampleNodePose(const SkeletonNode& node, const AnimationClip* clip, float time)
    {
        if (clip)
        {
            for (const auto& channel : clip->channels)
            {
                if (channel.nodeName != node.name)
                    continue;

                return {SampleVector(channel.positionKeys, time, node.bindPosition), SampleQuaternion(channel.rotationKeys, time, node.bindRotation),
                        SampleVector(channel.scaleKeys, time, node.bindScale)};
            }
        }

        return {node.bindPosition, node.bindRotation, node.bindScale};
    }

    void Update(AnimatorState& state, float deltaTime)
    {
        const Skeleton* skeleton = state.skeleton.get();
        if (!skeleton || skeleton->nodes.empty())
            return;

        if (state.boneMatrices.size() != skeleton->bones.size())
            state.boneMatrices.assign(skeleton->bones.size(), Matrix4());

        const AnimationClip* clip = nullptr;
        if (state.clips && state.currentClip >= 0 && state.currentClip < static_cast<int>(state.clips->size()))
            clip = &(*state.clips)[state.currentClip];

        const AnimationClip* previousClip = nullptr;
        if (state.clips && state.previousClip >= 0 && state.previousClip < static_cast<int>(state.clips->size()))
            previousClip = &(*state.clips)[state.previousClip];

        state.blendElapsed += deltaTime;
        const bool blending = previousClip != nullptr && state.blendElapsed < state.blendDuration;
        const float blendT = blending ? (state.blendDuration > 0.0001f ? state.blendElapsed / state.blendDuration : 1.0f) : 1.0f;

        if (state.playing && clip)
        {
            const float ticksPerSecond = clip->ticksPerSecond > 0.0f ? clip->ticksPerSecond : 25.0f;
            state.time += deltaTime * state.speed * ticksPerSecond;

            if (clip->durationTicks > 0.0f)
            {
                if (state.looping)
                    state.time = std::fmod(state.time, clip->durationTicks);
                else if (state.time > clip->durationTicks)
                    state.time = clip->durationTicks;
            }
        }

        globalTransforms.assign(skeleton->nodes.size(), Matrix4());

        for (size_t i = 0; i < skeleton->nodes.size(); i++)
        {
            const SkeletonNode& node = skeleton->nodes[i];

            LocalPose pose = SampleNodePose(node, clip, state.time);

            if (blending)
            {
                const LocalPose previousPose = SampleNodePose(node, previousClip, state.previousTime);
                pose.position = previousPose.position + (pose.position - previousPose.position) * blendT;
                pose.rotation = Quaternion::Slerp(previousPose.rotation, pose.rotation, blendT);
                pose.scale = previousPose.scale + (pose.scale - previousPose.scale) * blendT;
            }

            const Matrix4 localTransform = Matrix4::Compose(pose.position, pose.rotation, pose.scale);
            globalTransforms[i] = node.parentIndex >= 0 ? localTransform * globalTransforms[node.parentIndex] : localTransform;
        }

        for (size_t b = 0; b < skeleton->bones.size(); b++)
        {
            const BoneInfo& bone = skeleton->bones[b];
            const Matrix4& nodeGlobal = bone.nodeIndex >= 0 ? globalTransforms[bone.nodeIndex] : globalTransforms[0];
            state.boneMatrices[b] = bone.offsetMatrix * nodeGlobal * skeleton->globalInverseTransform;
        }

        if (state.nodeTransforms.size() != skeleton->nodes.size())
            state.nodeTransforms.resize(skeleton->nodes.size());

        for (size_t i = 0; i < skeleton->nodes.size(); i++)
            state.nodeTransforms[i] = globalTransforms[i] * skeleton->globalInverseTransform;
    }

    std::vector<Matrix4> globalTransforms;
    std::unordered_set<AnimatorState*> processed;
    std::unordered_set<AnimatorState*> visibleStates;
};
