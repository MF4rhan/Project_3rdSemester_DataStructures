# Huffman File Compression

A lossless file compression and decompression project written in C++20 using the Huffman Coding algorithm. It provides both a **console application (`HuffmanCLI`)** and a **Qt 6 Widgets desktop application (`HuffmanGUI`)** powered by a single shared backend library (**`HuffmanCore`**).

## Overview

Huffman Coding reduces file size by assigning shorter binary prefix codes to frequently occurring byte values and longer codes to less frequent ones. Because the implementation operates on all 256 possible byte values (`0x00`–`0xFF`) using binary file streams, it supports both text and arbitrary binary files and restores the original input byte-for-byte.

The compressed binary format stores:
1. **Metadata:** Original file size (`uint64_t`, 8 bytes).
2. **Serialized Huffman Tree:** Preorder traversal (`0` for internal nodes, `1` followed by the byte value for leaves).
3. **Compressed Bitstream:** Bit-packed Huffman codes.

## Applications & Shared Backend

- **`HuffmanCore` (`core/`):** Static C++20 library containing the manually implemented `MinHeap`, binary tree construction, code generation, tree serialization/deserialization, and binary compression/decompression logic. Both applications link directly against this library with no duplicated algorithm code.
- **`HuffmanCLI` (`console/`):** Interactive terminal menu preserving the original console workflow.
- **`HuffmanGUI` (`gui/`):** Cross-platform Qt 6 Widgets desktop interface featuring drag-and-drop file selection, Compress/Decompress mode switching, overwrite protection, background worker execution, compression statistics, an expandable Huffman Code Table, and three persisted dark themes (*Graphite*, *Midnight*, *Evergreen*).

## Project Structure

```text
├── core/               # Shared HuffmanCore library (Node, MinHeap, HuffmanTree, Compression, Decompression)
├── console/            # Console application entry point (main.cpp)
├── gui/                # Qt 6 Widgets desktop application (HuffmanGUI)
├── tests/              # Automated core & GUI verification tests + sample fixtures
├── docs/               # Detailed build instructions (BUILDING.md)
├── HuffmanProject/     # Legacy Visual Studio console solution (.slnx / .vcxproj)
└── CMakeLists.txt      # CMake build for HuffmanCore, HuffmanCLI, HuffmanGUI, and tests
```

## Quick Build (CMake)

Requires a **C++20 compiler**, **CMake 3.16+**, and **Qt 6.2 or newer** (`Qt6Widgets`). See [docs/BUILDING.md](docs/BUILDING.md) for full Linux and Windows instructions.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Run either application from the build directory:

```bash
./build/HuffmanGUI    # Desktop GUI
./build/HuffmanCLI    # Console app
```

Run the automated test suite:

```bash
ctest --test-dir build --output-on-failure
```

*(To build only the console target with a standalone C++20 compiler without Qt, compile `console/main.cpp` together with `core/*.cpp` and `-Icore`.)*

## Platform Requirements & Current Limitations

- **Supported Qt Versions:** Targets **Qt 6.2+** (`Qt6Widgets`, optional `Qt6Test` for GUI tests). Verified on Linux (CachyOS KDE with GCC 16.2.1, CMake 4.5.0, Qt 6.12.0).
- **Linux:** For broad distribution across Ubuntu 22.04+, Debian 12+, Fedora, and Arch-based distros, release builds should target an older baseline such as Ubuntu 22.04 (`glibc 2.35`, Qt 6.2.4+) rather than rolling-release system libraries. Standalone AppImage packaging has not been built yet.
- **Windows:** Configured for Windows 10/11 x64 (MSVC + Qt 6 + CMake) and UTF-8 `std::filesystem::path` handling, **but Windows builds and deployment have not yet been compiled or tested on a Windows machine**.
- **Format & Size Notes:**
  - The compressed format has no magic-byte signature (preserving compatibility with existing compressed files).
  - Byte frequencies in `Node` / `Compression` use signed 32-bit `int`, limiting any single byte's frequency to `2,147,483,647` (~2 GB).
  - Compression and decompression use an indeterminate busy indicator rather than percentage progress.
