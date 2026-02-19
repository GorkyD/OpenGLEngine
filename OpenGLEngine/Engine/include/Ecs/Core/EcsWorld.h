#pragma once

#include "ComponentPool.h"
#include "Entity.h"
#include <unordered_map>
#include <typeindex>
#include <memory>

class EcsWorld
{
	public:
		Entity CreateEntity();

		void DestroyEntity(Entity entity);
		void ProcessDeferred();

		template<typename T>
	   ComponentPool<T>& GetPool()
		{
	      auto key = std::type_index(typeid(T));
	      if (pools.find(key) == pools.end())
	         pools[key] = std::make_unique<ComponentPool<T>>();
	      return *static_cast<ComponentPool<T>*>(pools[key].get());
	   }

		template<typename T>
	   T& AddComponent(Entity entity) { return GetPool<T>().Add(entity); }

		template<typename T>
	   T& GetComponent(Entity entity) { return GetPool<T>().Get(entity); }

		template<typename T>
	   bool HasComponent(Entity entity) { return GetPool<T>().Has(entity); }

	private:
		Entity nextEntity = 0;
		std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools;
		std::vector<Entity> alive;
		std::vector<Entity> toDestroy;
};
