#include "Engine/Engine.h"
#include "ExampleGame.h"
#include <iostream>

int main()
{
    try
    {
        ExampleGame engine;
        engine.Run();
    }
    catch (const std::exception& ex)
    {
        std::clog << ex.what() << '\n';
        return 1;
    }
    return 0;
}
