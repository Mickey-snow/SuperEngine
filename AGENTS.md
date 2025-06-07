Purpose: Provide coding guidelines and instructions for working on this repository.

# Repository Overview
- This project implements "rlvm", a free reimplementation of VisualArt's/Key's RealLive interpreter used to run VisualArts games. The main code lives in `src/` and is split into subdirectories like `core`, `libreallive`, `libsiglus`, `machine`, `modules`, `systems`, `vm`, and `m6`.
- Tests are under `test/` and use GoogleTest.
- Third‑party libraries live in `vendor/`.
- Developer notes and format documentation are in `doc/`.

# Running Unit Tests
1. Run:
```bash
git submodule update --init --recursive
```
2. Install system dependencies:
```bash
sudo apt-get -qq update
sudo apt-get -qq install -y --fix-missing \
  libgtest-dev libgmock-dev \
  libsdl1.2-dev \
  libboost-all-dev \
  libgl1-mesa-dev libglu1-mesa-dev \
  libglew-dev \
  libvorbis-dev \
  zlib1g-dev \
  libfreetype6-dev \
  gettext \
  libtbb-dev
```

3. Configure with tests enabled and build:
```bash
cmake -S . -B build -DRLVM_BUILD_TESTS=ON
cmake --build build
```

4. Execute tests with:
```bash
./build/unittest
```

# Coding Style
- The repository uses a `.clang-format` file derived from the Chromium style. Run `clang-format -i` on changed C++ files before committing.

# Pull Request Guidelines
- Describe the purpose of your changes and reference relevant files or functions.
