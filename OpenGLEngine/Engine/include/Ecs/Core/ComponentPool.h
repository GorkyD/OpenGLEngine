#pragma once

#include <unordered_map>
#include "Entity.h"

class IComponentPool
{
	public:
	   virtual ~IComponentPool() = default;
	   virtual void Remove(Entity entity) = 0;
	   virtual bool Has(Entity entity) const = 0;
};

template<typename T>
class ComponentPool : public IComponentPool
{
	std::unordered_map<Entity, T> data;

	public:
	   T& Add(Entity entity) { return data[entity]; }
	   T& Get(Entity entity) { return data.at(entity); }

	   bool Has(Entity entity) const override { return data.count(entity) > 0; }
	   void Remove(Entity entity) override { data.erase(entity); }

	   auto begin() { return data.begin(); }
	   auto end() { return data.end(); }
};
