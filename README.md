# OpenGLEngine

## Build Setup

Dependencies are managed through `vcpkg` manifest mode (`vcpkg.json`).

### 1) Install toolchain

- Windows: LLVM/Clang, Ninja, Visual Studio Build Tools, `vcpkg` in `C:/vcpkg`
- Linux/macOS: Clang, Ninja, `vcpkg` in `$HOME/vcpkg`

If your `vcpkg` path is different, override `VCPKG_ROOT` in `CMakeUserPresets.json`.

### 2) Configure and build

- Windows:
  - `cmake --preset clang-cl-debug`
  - `cmake --build --preset clang-cl-debug -j 8`
- Linux:
  - `cmake --preset clang-debug-linux`
  - `cmake --build --preset clang-debug-linux -j 8`
- macOS:
  - `cmake --preset clang-debug-macos`
  - `cmake --build --preset clang-debug-macos -j 8`

### 3) Local overrides (optional)

Create `CMakeUserPresets.json` for machine-specific settings:
- `WWISE_SDK_DIR`
- `OPENGLENGINE_ENABLE_WWISE`
- `CMAKE_AR` (Windows)