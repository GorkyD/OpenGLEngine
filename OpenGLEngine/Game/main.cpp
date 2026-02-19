#include <iostream>
#include "ExampleGame.h"
#include "Engine/Engine.h"

int main()
{
	try
	{
		ExampleGame engine;
		engine.Run();
	}
	catch (const std::exception& ex)
	{
		std::wclog << ex.what() << '\n';
		return 1;
	}
	return 0;
}
