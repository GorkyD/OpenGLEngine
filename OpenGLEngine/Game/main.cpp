#include <iostream>

#include "MyGame.h"
#include  "Engine/Engine.h"

int main()
{
	try
	{
		MyGame engine;
		engine.Run();
	}
	catch (const std::exception& ex)
	{
		std::wclog << ex.what() << '\n';
		return 1;
	}
	return 0;
}
