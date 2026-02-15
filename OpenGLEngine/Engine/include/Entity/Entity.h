#pragma once

class EntitySystem;

class Entity
{
	public:
		Entity();
		virtual ~Entity();

		void Release();

		EntitySystem* GetEntitySystem() const { return  entitySystem; }

	protected:
		virtual void OnCreate() {}
		virtual void OnUpdate(float deltaTime) {}

		EntitySystem* entitySystem = nullptr;

		size_t id = -1;

		friend class EntitySystem;
};
