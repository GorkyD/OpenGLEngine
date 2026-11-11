#include "Engine/Engine.h"
#include "GameRegistry.h"
#include "CampfireScene.h"
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
        std::string sceneFile;
        std::string screenshot;
        int screenshotFrame = 120;

        for (int i = 1; i < argc; i++)
        {
            const std::string arg = argv[i];
            if (arg == "--play")
                play = true;
            else if (arg.rfind("--scene=", 0) == 0)
                startScene = arg.substr(8);
            else if (arg.rfind("--scene-file=", 0) == 0)
                sceneFile = arg.substr(13);
            else if (arg.rfind("--screenshot=", 0) == 0)
                screenshot = arg.substr(13);
            else if (arg.rfind("--screenshot-frame=", 0) == 0)
                screenshotFrame = std::stoi(arg.substr(19));
        }

        Engine engine(play ? EngineMode::Play : EngineMode::Editor);
        engine.SetSceneFileOverride(sceneFile);
        if (!screenshot.empty())
            engine.RequestScreenshot(screenshot, screenshotFrame);

        engine.RegisterScene("Fps", [] { return std::make_unique<FpsScene>(); }, "Fps", FindGameAssetsPath("Fps"));
        engine.RegisterScene("Campfire", [] { return std::make_unique<CampfireScene>(); }, "Fps", FindGameAssetsPath("Fps"));
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
