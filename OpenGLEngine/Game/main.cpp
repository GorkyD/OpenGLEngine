#include "Engine/Engine.h"
#include "GameRegistry.h"
#include "FpsScene.h"
#include "Scene/DemoScene.h"
#include "ZigZagScene.h"
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char** argv)
{
    try
    {
        std::string startScene = "Fps";
        bool play = false;

        for (int i = 1; i < argc; i++)
        {
            const std::string arg = argv[i];
            if (arg == "--play")
                play = true;
            else if (arg.rfind("--scene=", 0) == 0)
                startScene = arg.substr(8);
        }

        Engine engine(play ? EngineMode::Play : EngineMode::Editor);

        engine.RegisterScene("Fps", [] { return std::make_unique<FpsScene>(); }, "Fps", FindGameAssetsPath("Fps"));
        engine.RegisterScene("ZigZag", [] { return std::make_unique<ZigZagScene>(); }, "ZigZag_OpenGL", FindGameAssetsPath("ZigZag_OpenGL"));
        engine.RegisterScene("Demo", [] { return std::make_unique<DemoScene>(); });

        for (const auto& entry : engine.GetSceneRegistry())
        {
            if (entry.name == startScene)
            {
                engine.LoadScene(entry.factory(), entry.name);
                break;
            }
        }

        engine.Run();
    }
    catch (const std::exception& ex)
    {
        std::clog << ex.what() << '\n';
        return 1;
    }
    return 0;
}
