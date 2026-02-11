#pragma once
#include <map>
#include <memory>
#include <set>
#include "OEntity.h"
#include "Extension/OExtension.h"
class OEntitySystem
{
	public:
		OEntitySystem();
		~OEntitySystem();

		template<typename T>
		T* CreateEntity()
		{
			static_assert(std::is_base_of<OEntity,T>::value, "T must derive from OEntity class");
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
		bool CreateEntityInternal(OEntity* entity, size_t id);
		void RemoveEntity(OEntity* entity);

		void Update(float deltaTime);

		std::map<size_t, std::map< OEntity*, std::unique_ptr<OEntity>>> entities;
		std::set<OEntity*> entitiesToDestroy;

		friend class OEntity;
		friend class OEngine;
};

