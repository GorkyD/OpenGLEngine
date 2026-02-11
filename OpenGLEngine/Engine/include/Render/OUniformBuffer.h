#pragma once
#include "Extension/OExtension.h"

class OUniformBuffer
{
	public:
		OUniformBuffer(const OUniformBufferDesc& desc);
		~OUniformBuffer();

		void SetData(void* data);

		int GetId() const { return id; }

	private:
		unsigned int id = 0;
		unsigned int size = 0;
};

