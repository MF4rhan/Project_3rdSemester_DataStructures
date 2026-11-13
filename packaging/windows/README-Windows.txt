Huffman File Compression Studio — Windows x64 Package
=====================================================

Contents
--------
- HuffmanGUI.exe    : Desktop GUI application (Qt 6 Widgets)
- HuffmanCLI.exe    : Console application (interactive terminal menu)
- Qt6*.dll & plugins: Bundled Qt 6 runtime libraries (including platforms\qwindows.dll)
- vc_redist.x64.exe : Official Microsoft Visual C++ 2015-2022 Redistributable (x64)

Prerequisites
-------------
- Windows 10 / 11 (64-bit x64).
- No Qt SDK, Visual Studio, or CMake installation is required.
- If launching HuffmanGUI.exe or HuffmanCLI.exe reports a missing Visual C++
  runtime DLL (such as VCRUNTIME140.dll or MSVCP140.dll), run vc_redist.x64.exe
  once to install the Microsoft Visual C++ x64 runtime.

Running the Desktop GUI
-----------------------
Extract the entire ZIP archive to a folder (keeping all DLLs and the
platforms\ subfolder alongside HuffmanGUI.exe), then double-click:
  HuffmanGUI.exe

Running the Console Application
-------------------------------
Open Command Prompt or PowerShell in the extracted folder and run:
  HuffmanCLI.exe
Follow the interactive menu (1 = Compress, 2 = Decompress, 3 = Exit).
