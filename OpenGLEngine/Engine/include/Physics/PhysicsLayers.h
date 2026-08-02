#pragma once

namespace PhysicsLayers
{
constexpr unsigned int None = 0u;
constexpr unsigned int All = ~0u;

constexpr unsigned int Default = 1u << 0;
constexpr unsigned int Player = 1u << 1;
constexpr unsigned int Enemy = 1u << 2;
constexpr unsigned int Projectile = 1u << 3;
constexpr unsigned int Trigger = 1u << 4;
} // namespace PhysicsLayers
