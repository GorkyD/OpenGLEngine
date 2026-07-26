# OpenGLEngine

![Demo](docs/demo.gif)

## Build Setup

Dependencies are managed through `vcpkg` manifest mode (`vcpkg.json`). On Linux and macOS, the helper scripts bootstrap `vcpkg` into the local `./vcpkg` directory. The macOS preset targets Apple Silicon (`arm64-osx`).

### 1) Install toolchain

- Windows: LLVM/Clang, Ninja, Visual Studio Build Tools, `vcpkg` in `C:/vcpkg`
- Linux/macOS: Clang, Ninja, Git, CMake

If your `vcpkg` path is different, set the `VCPKG_ROOT` environment variable before running `./scripts/configure.sh`. Direct `cmake --preset ...` uses the repository-local `./vcpkg` path unless overridden through `CMakeUserPresets.json`.

### 2) Configure and build

- Windows:
  - `cmake --preset clang-cl-debug`
  - `cmake --build --preset clang-cl-debug -j 8`
- Linux:
  - `./scripts/configure.sh clang-debug-linux`
  - `cmake --build --preset clang-debug-linux -j 8`
- macOS:
  - `./scripts/configure.sh clang-debug-macos`
  - `cmake --build --preset clang-debug-macos -j 8`

`./scripts/configure.sh` auto-detects the Linux/macOS preset when no preset name is passed. Extra arguments are forwarded to `cmake --preset`.

### 3) Local overrides (optional)

Create `CMakeUserPresets.json` for machine-specific settings:
- `WWISE_SDK_DIR`
- `OPENGLENGINE_ENABLE_WWISE`
- `CMAKE_AR` (Windows)
- `VCPKG_ROOT`

Example `CMakeUserPresets.json` (Windows):

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "clang-cl-debug-local",
      "inherits": "clang-cl-debug",
      "cacheVariables": {
        "WWISE_SDK_DIR": "D:/WwiseSdk/Wwise2025.1.5.9095/SDK",
        "OPENGLENGINE_ENABLE_WWISE": "ON"
      }
    }
  ],
  "buildPresets": [
    { "name": "clang-cl-debug-local", "configurePreset": "clang-cl-debug-local" }
  ]
}
```

This is only needed if you want to force a specific SDK path/toggle. See below — Wwise is normally picked up automatically.

### 4) Audio (Wwise)

**Wwise is detected automatically** if it's installed via the Audiokinetic Launcher — no `CMakeUserPresets.json` required. `CMakeLists.txt` looks for a valid SDK, in order:

1. `WWISE_SDK_DIR` passed explicitly (`-D`/preset `cacheVariables`).
2. The `WWISESDK` environment variable (set via the Launcher's Wwise page → **Settings → Set Environment Variables**).
3. The `WWISEROOT` environment variable (`<WWISEROOT>/SDK`).
4. The legacy `WWISE_SDK_DIR` environment variable.
5. The Launcher's own install list (`%AppData%/WwiseLauncher/Json/knownInstalls.json` on Windows, the equivalent `~/Library/Application Support/...` on macOS), so it works even without setting any environment variable.

If any of these resolve to a valid SDK, `OPENGLENGINE_ENABLE_WWISE` defaults to `ON` automatically; otherwise the engine builds Wwise-free (`OPENGLENGINE_USE_WWISE=0`, stub audio). You can still force it off with `-DOPENGLENGINE_ENABLE_WWISE=OFF` or force a specific path with `-DWWISE_SDK_DIR=...`.

When enabled, `AudioSystem` (`OpenGLEngine/Engine/include/Audio/AudioSystem.h`, `OpenGLEngine/Engine/source/Audio/AudioSystem.cpp`) initializes the Wwise sound engine (memory/stream managers, default low-level I/O, and — in debug builds — the profiler `Communication` module) and loads SoundBanks from `OpenGLEngine/Assets/Wwise/`.

**SoundBanks are synced automatically on every build.** After `Generate SoundBanks` in Wwise Authoring, just rebuild the engine — a `POST_BUILD` step (`cmake/CopyWwiseBanks.cmake`) copies every `*.bnk` from the Wwise project's `GeneratedSoundBanks/<Platform>/` folder into `OpenGLEngine/Assets/Wwise/`, no manual copy needed. The Wwise project folder is resolved, in order:

1. `WWISE_PROJECT_DIR` passed explicitly (`-D`/preset `cacheVariables`).
2. The `WWISE_PROJECT_DIR` environment variable.
3. `<WWISEROOT>/../Projects/<project name>` (the Launcher's default "Projects" folder layout next to the installed SDK).

If none resolve, the sync step is simply skipped and `Assets/Wwise/*.bnk` stays whatever was copied there manually last.

- A listener game object is registered automatically and is kept at the camera position every frame.
- Game objects (e.g. torches) are registered via `audioSystem->RegisterGameObject(id, name)`, positioned via `audioSystem->SetPosition(id, x, y, z)`, and triggered via `audioSystem->PlayEvent(eventName, id)`.
- A reserved, non-positional `AudioSystem::AmbientId` game object is registered automatically in `Init()` for background/ambient sounds (no `SetPosition` needed). `ExampleGame` posts `Play_Ambient` on it once at startup.
- `ExampleGame` posts the `Play_Fire` event on each torch's flame entity as a demo.
- Three volume knobs are wired as RTPC (Real-Time Parameter Control), driven from code without touching Wwise/regenerating banks: `audioSystem->SetFireVolume(0-100)`, `SetAmbientVolume(0-100)`, `SetMasterVolume(0-100)` (or the generic `SetRTPCValue(name, value)`). These expect Wwise Game Parameters named `Volume_Fire`, `Volume_Ambient`, `Volume_Master`, bound as RTPC to the Voice Volume of the fire sound, the ambient sound, and the root output bus respectively.

To hear audio you need SoundBanks generated by a Wwise project:

1. In Wwise Authoring, create/author the `Init` bank and a `Main` bank containing at least a `Play_Fire` event.
2. Generate SoundBanks for Windows and copy `Init.bnk` and `Main.bnk` (plus any referenced media/`.txt` metadata) into `OpenGLEngine/Assets/Wwise/`.
3. Build and run with the Wwise-enabled preset. If the banks are missing, `AudioSystem::Init` logs a warning to stderr and the engine keeps running without audio (no crash).

When Wwise is disabled (default), `AudioSystem` compiles to no-op stubs so the rest of the engine builds and runs unchanged.
