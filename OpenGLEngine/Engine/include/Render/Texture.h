#pragma once
#include <string>
#include "Extension/Extension.h"

class Texture
{
	public:
		static TexturePtr LoadFromFile(const std::string& path);
		~Texture();

		unsigned int GetId() const { return id; }

	private:
		Texture() = default;
		unsigned int id = 0;
		int width = 0;
		int height = 0;
		int channels = 0;
};
