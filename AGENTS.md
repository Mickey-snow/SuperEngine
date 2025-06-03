Purpose: Provide coding guidelines and instructions for working on this repository.

# Repository Overview
- This project implements "rlvm", a free reimplementation of VisualArt's/Key's RealLive interpreter used to run VisualArts games. The main code lives in `src/` and is split into subdirectories like `core`, `libreallive`, `libsiglus`, `machine`, `modules`, `systems`, `vm`, and `m6`.
- Tests are under `test/` and use GoogleTest.
- Third‑party libraries live in `vendor/`.
- Developer notes and format documentation are in `doc/`.

# Building
1. Ensure dependencies from the README are installed (CMake≥3.18, Boost, SDL 1.2, OpenGL, etc.).
2. Configure and build:
   ```bash
   cmake -S . -B build -G "<generator>"
   cmake --build build
   ```
3. The main executable is `./build/rlvm`.

# Running Unit Tests
1. Configure with tests enabled and build:
   ```bash
   cmake -S . -B build -DRLVM_BUILD_TESTS=ON
   cmake --build build
   ```
2. Execute tests with:
   ```bash
   ./build/unittest
   ```

# Coding Style
- The repository uses a `.clang-format` file derived from the Chromium style. Run `clang-format -i` on changed C++ files before committing.

# Pull Request Guidelines
- Describe the purpose of your changes and reference relevant files or functions.
- Include test results from running `./build/unittest`.
