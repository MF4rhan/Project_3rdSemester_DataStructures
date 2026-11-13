# Building and Development Guide

This project builds with **CMake 3.16+** and **C++20**, producing a shared static library (`HuffmanCore`), the console executable (`HuffmanCLI`), the Qt Widgets GUI (`HuffmanGUI`), and automated tests (`HuffmanCoreTests`, plus `HuffmanGUITests` when `Qt6Test` is available).

---

## Requirements

- **Compiler:** C++20 capable compiler (GCC 11+, Clang 14+, or MSVC 2022+)
- **Build System:** CMake 3.16 or newer
- **Qt:** Qt 6.2 or newer (`Widgets` component required for `HuffmanGUI`; `Test` component optional for `HuffmanGUITests`)

---

## Linux Build Instructions

### CachyOS / Arch Linux
Install CMake and Qt 6:

```bash
sudo pacman -S cmake qt6-base
```

Configure, build, and test:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Launch the applications:

```bash
./build/HuffmanGUI
./build/HuffmanCLI
```

### Ubuntu 22.04+ / Debian 12+
Install build tools and Qt 6 development packages:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev
```

Configure and build using the same CMake commands above. Note that Ubuntu 22.04 ships Qt 6.2.4 (`glibc 2.35`), which satisfies the project's `Qt6 6.2` minimum requirement and provides a clean baseline for future portable Linux builds.

*(Building only the console executable without Qt or CMake: `g++ -std=c++20 -Icore console/main.cpp core/*.cpp -o HuffmanCLI`.)*

---

## Windows 10 / 11 (x64) Build Instructions

> **Status:** Windows CMake/MSVC instructions below are prepared for Visual Studio + Qt 6, **but have not yet been compiled or tested on an actual Windows machine**.

### Prerequisites
1. **Visual Studio** (2022 or newer) with the *Desktop development with C++* workload.
2. **CMake** (standalone or bundled with Visual Studio).
3. **Qt 6** (MSVC 64-bit kit, Qt 6.2 or newer, e.g., via Qt Online Installer or `aqtinstall`), noting its prefix path such as `C:\Qt\6.x.x\msvc2022_64`.

### Building with CMake
Open an **x64 Native Tools Command Prompt** (or Developer PowerShell for Visual Studio) in the repository root:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

*(If CMake does not select your preferred generator automatically, pass `-G "Ninja"` inside the x64 Native Tools prompt or `-G "Visual Studio 17 2022" -A x64` / your installed Visual Studio generator.)*

### Deploying Qt Runtime Dependencies (for Testing Standalone)
Run `windeployqt.exe` from your Qt kit against the built GUI executable:

```powershell
C:\Qt\6.x.x\msvc2022_64\bin\windeployqt.exe build\Release\HuffmanGUI.exe
.\build\Release\HuffmanGUI.exe
```

Note that running `HuffmanGUI.exe` on a clean Windows PC without Visual Studio installed will also require the **Microsoft Visual C++ x64 Redistributable** (to be addressed during packaging).

---

## Legacy Visual Studio Console Solution

The original console-only Visual Studio solution in `HuffmanProject/` has been updated to reference `core/` and `console/main.cpp` directly so there is no duplicated backend code. It builds `HuffmanCLI`-equivalent console behavior on Windows without requiring Qt.
