#include <iostream>
#include <memory>
#include "Engine/Engine.h"
#include "Scene/DemoScene.h"

int main()
{
    try
    {
        Engine engine(EngineMode::Play);
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
