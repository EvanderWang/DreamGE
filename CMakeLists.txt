cmake_minimum_required(VERSION 3.12.3)

set(CMAKE_GENERATOR "Unix Makefiles")

if(WIN32)
    set(CLANG_FOLDER "C:/Program Files/LLVM/bin")
elseif(APPLE)
    set(CLANG_FOLDER "/opt/local/bin")
endif()

SET (CMAKE_C_COMPILER             "${CLANG_FOLDER}/clang.exe")
SET (CMAKE_C_FLAGS                "-Wall -std=c99")
SET (CMAKE_C_FLAGS_DEBUG          "-g")
SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_CXX_COMPILER             "${CLANG_FOLDER}/clang++.exe")
SET (CMAKE_CXX_FLAGS                "-Wall -std=c++17")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_AR      "${CLANG_FOLDER}/llvm-ar")
SET (CMAKE_LINKER  "${CLANG_FOLDER}/llvm-ld")
SET (CMAKE_NM      "${CLANG_FOLDER}/llvm-nm")
SET (CMAKE_OBJDUMP "${CLANG_FOLDER}/llvm-objdump")
SET (CMAKE_RANLIB  "${CLANG_FOLDER}/llvm-ranlib")

project(DreamGE)

add_executable(DreamGE main.cpp)