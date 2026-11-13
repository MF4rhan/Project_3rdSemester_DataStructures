Huffman File Compression Studio — Linux x86-64 Package
========================================================

Contents
--------
- HuffmanStudio-x86_64.AppImage : Portable desktop GUI application
- HuffmanCLI                    : Console application (terminal menu)

System Compatibility Baseline
-----------------------------
- Architecture: x86-64 (64-bit Linux)
- Minimum C library baseline: glibc 2.35 or newer (built on Ubuntu 22.04 LTS)
- Target systems: Ubuntu 22.04+ and other Linux distributions with compatible
  system libraries (OpenGL/Wayland/X11/glibc 2.35+ and standard libstdc++).
- Display backends: The AppImage includes Qt platform plugins for both
  X11/XWayland and native Wayland sessions (Qt selects the appropriate platform
  automatically). Actual desktop compatibility still depends on the host
  graphics environment.
- Compatibility with older glibc baselines or non-x86-64 systems is not
  guaranteed.

Running the Desktop GUI (AppImage)
----------------------------------
1. Make the AppImage executable if needed:
     chmod +x HuffmanStudio-x86_64.AppImage
2. Launch it from your file manager or terminal:
     ./HuffmanStudio-x86_64.AppImage

If FUSE is unavailable on your host (for example, missing libfuse2 on newer
Ubuntu/Debian installs or inside restricted containers), run the AppImage with
the documented extract-and-run fallback:
     ./HuffmanStudio-x86_64.AppImage --appimage-extract-and-run

Running the Console Application (HuffmanCLI)
--------------------------------------------
1. Ensure executable permissions:
     chmod +x HuffmanCLI
2. Launch from a terminal:
     ./HuffmanCLI
   Follow the interactive menu (1 = Compress, 2 = Decompress, 3 = Exit).
   HuffmanCLI dynamically links the host system's standard C/C++ libraries
   (glibc 2.35+ and libstdc++).
