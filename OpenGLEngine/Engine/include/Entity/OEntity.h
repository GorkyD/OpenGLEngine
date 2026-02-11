#pragma once

class OEntitySystem;

class OEntity
{
	public:
		OEntity();
		virtual ~OEntity();

		void Release();

		OEntitySystem* GetEntitySystem() const { return  entitySystem; }

	protected:
		virtual void OnCreate() {}
		virtual void OnUpdate(float deltaTime) {}

		OEntitySystem* entitySystem = nullptr;

		size_t id = -1;

		friend class OEntitySystem;
};

