#pragma once
#include "Math/Vector3.h"

struct TransformComponent
{
   Vector3 position = { 0, 0, 0 };
   Vector3 scale = { 1, 1, 1 };

   Matrix4 rotation;

   Matrix4 GetModelMatrix() const
   {
      Matrix4 t;
      t.SetTranslation(position);

      Matrix4 s;
      s.SetScale(scale);

      return s * rotation * t;
   }
};
