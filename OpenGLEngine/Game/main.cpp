#include "Engine/Engine.h"
#include "GameRegistry.h"
#include "Scene/DemoScene.h"
#if __has_include("FpsScene.h")
#include "CampfireScene.h"
#include "FpsScene.h"
#define OPENGLENGINE_HAS_FPS 1
#endif
#if __has_include("LadScene.h")
#include "LadScene.h"
#define OPENGLENGINE_HAS_LAD 1
#endif
#if __has_include("ZigZagScene.h")
#include "ZigZagScene.h"
#define OPENGLENGINE_HAS_ZIGZAG 1
#endif
#include <algorithm>
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

#if OPENGLENGINE_HAS_FPS
        engine.RegisterScene("Fps", [] { return std::make_unique<FpsScene>(); }, "Fps", FindGameAssetsPath("Fps"));
        engine.RegisterScene("Campfire", [] { return std::make_unique<CampfireScene>(); }, "Fps", FindGameAssetsPath("Fps"));
#endif
#if OPENGLENGINE_HAS_LAD
        engine.RegisterScene("Lad", [] { return std::make_unique<LadScene>(); }, "Lad", FindGameAssetsPath("Lad"));
#endif
#if OPENGLENGINE_HAS_ZIGZAG
        engine.RegisterScene("ZigZag", [] { return std::make_unique<ZigZagScene>(); }, "ZigZag_OpenGL", FindGameAssetsPath("ZigZag_OpenGL"));
#endif
        engine.RegisterScene("Demo", [] { return std::make_unique<DemoScene>(); });

        const auto& registry = engine.GetSceneRegistry();
        const auto start = std::find_if(registry.begin(), registry.end(), [&](const auto& entry) { return entry.name == startScene; });
        const auto& chosen = start != registry.end() ? *start : registry.front();
        engine.LoadScene(chosen.factory(), chosen.name);

        engine.Run();
    }
    catch (const std::exception& ex)
    {
        std::clog << ex.what() << '\n';
        return 1;
    }
    return 0;
}
