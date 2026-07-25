#include "Engine/Engine.h"
#include "Scene/DemoScene.h"
#include <iostream>
#include <memory>

int main()
{
    try
    {
        Engine engine;
        engine.LoadScene(std::make_unique<DemoScene>());
        engine.Run();
    }
    catch (const std::exception& ex)
    {
        std::clog << ex.what() << '\n';
        return 1;
    }
    return 0;
}
