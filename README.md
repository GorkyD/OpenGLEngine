# OpenGLEngine

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
