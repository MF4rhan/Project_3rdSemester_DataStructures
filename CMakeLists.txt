cmake_minimum_required(VERSION 3.16)

project(HuffmanStudio VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

option(BUILD_TESTS "Build automated verification tests" ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

# Shared Huffman compression/decompression backend library
add_library(HuffmanCore STATIC
    core/Node.cpp
    core/Node.h
    core/MinHeap.cpp
    core/MinHeap.h
    core/HuffmanTree.cpp
    core/HuffmanTree.h
    core/Compression.cpp
    core/Compression.h
    core/Decompression.cpp
    core/Decompression.h
)

target_include_directories(HuffmanCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

if(MSVC)
    target_compile_options(HuffmanCore PRIVATE /W3 /utf-8)
else()
    target_compile_options(HuffmanCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Console application target
add_executable(HuffmanCLI
    console/main.cpp
)

target_link_libraries(HuffmanCLI PRIVATE
    HuffmanCore
)

if(MSVC)
    target_compile_options(HuffmanCLI PRIVATE /W3 /utf-8)
else()
    target_compile_options(HuffmanCLI PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Qt Widgets desktop GUI application target
add_executable(HuffmanGUI WIN32
    gui/gui_main.cpp
    gui/Theme.cpp
    gui/Theme.h
    gui/FileDropZone.cpp
    gui/FileDropZone.h
    gui/HuffmanWorker.cpp
    gui/HuffmanWorker.h
    gui/MainWindow.cpp
    gui/MainWindow.h
)

target_include_directories(HuffmanGUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

target_link_libraries(HuffmanGUI PRIVATE
    HuffmanCore
    Qt6::Widgets
)

if(MSVC)
    target_compile_options(HuffmanGUI PRIVATE /W3 /utf-8)
else()
    target_compile_options(HuffmanGUI PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTS)
    enable_testing()

    add_executable(HuffmanCoreTests
        tests/test_core.cpp
    )

    target_link_libraries(HuffmanCoreTests PRIVATE
        HuffmanCore
    )

    target_compile_definitions(HuffmanCoreTests PRIVATE
        HUFFMAN_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )

    if(MSVC)
        target_compile_options(HuffmanCoreTests PRIVATE /W3 /utf-8)
    else()
        target_compile_options(HuffmanCoreTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME HuffmanCoreTests COMMAND HuffmanCoreTests)

    find_package(Qt6 6.2 QUIET COMPONENTS Test)
    if(Qt6Test_FOUND)
        add_executable(HuffmanGUITests
            tests/test_gui.cpp
            gui/Theme.cpp
            gui/Theme.h
            gui/FileDropZone.cpp
            gui/FileDropZone.h
            gui/HuffmanWorker.cpp
            gui/HuffmanWorker.h
            gui/MainWindow.cpp
            gui/MainWindow.h
        )

        target_include_directories(HuffmanGUITests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/gui
        )

        target_link_libraries(HuffmanGUITests PRIVATE
            HuffmanCore
            Qt6::Widgets
            Qt6::Test
        )

        target_compile_definitions(HuffmanGUITests PRIVATE
            HUFFMAN_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        )

        if(MSVC)
            target_compile_options(HuffmanGUITests PRIVATE /W3 /utf-8)
        else()
            target_compile_options(HuffmanGUITests PRIVATE -Wall -Wextra -Wpedantic)
        endif()

        add_test(NAME HuffmanGUITests COMMAND HuffmanGUITests)
        set_tests_properties(HuffmanGUITests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
endif()
