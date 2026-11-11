#include <iostream>
#include <memory>
#include "Engine/Engine.h"
#include "FpsScene.h"

int main()
{
    try
    {
        Engine engine(EngineMode::Play);
        engine.RegisterScene("Fps", [] { return std::make_unique<FpsScene>(); }, OPENGLENGINE_GAME_NAME, OPENGLENGINE_GAME_ASSETS);
        engine.LoadScene(std::make_unique<FpsScene>(), "Fps");
        engine.Run();
    }
    catch (const std::exception& ex)
    {
        std::clog << ex.what() << '\n';
        return 1;
    }

    return 0;
}
