#pragma once
#include <map>
#include <memory>
#include <set>
#include "Entity.h"
#include "Extension/Extension.h"
class EntitySystem
{
	public:
		EntitySystem();
		~EntitySystem();

		template<typename T>
		T* CreateEntity()
		{
			static_assert(std::is_base_of<Entity,T>::value, "T must derive from Entity class");
			auto id = typeid(T).hash_code();
			auto e = new T();

			if (CreateEntityInternal(e, id)) 
			{
				OGL_INFO("Create player with id: " << id);
				return e;
			}
			
			return nullptr;
		}


	private:
		bool CreateEntityInternal(Entity* entity, size_t id);
		void RemoveEntity(Entity* entity);

		void Update(float deltaTime);

		std::map<size_t, std::map< Entity*, std::unique_ptr<Entity>>> entities;
		std::set<Entity*> entitiesToDestroy;

		friend class Entity;
		friend class Engine;
};
