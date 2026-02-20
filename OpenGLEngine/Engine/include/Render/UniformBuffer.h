#pragma once
#include "Extension/Extension.h"

class UniformBuffer
{
	public:
		UniformBuffer(const UniformBufferDesc& desc);
		~UniformBuffer();

	void SetData(void* data);
	void SetSubData(void* data, unsigned int offset, unsigned int subSize);

	int GetId() const { return id; }

	private:
		unsigned int id = 0;
		unsigned int size = 0;
};
