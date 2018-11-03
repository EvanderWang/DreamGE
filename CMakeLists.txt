cmake_minimum_required(VERSION 3.12.3)

#SET (CMAKE_C_COMPILER             "/opt/local/bin/clang")
#SET (CMAKE_C_FLAGS                "-Wall -std=c99")
#SET (CMAKE_C_FLAGS_DEBUG          "-g")
#SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
#SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
#SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_CXX_COMPILER             "/opt/local/bin/clang++")
SET (CMAKE_CXX_FLAGS                "-Wall -std=c++17")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_AR      "/opt/local/bin/llvm-ar")
SET (CMAKE_LINKER  "/opt/local/bin/llvm-ld")
SET (CMAKE_NM      "/opt/local/bin/llvm-nm")
SET (CMAKE_OBJDUMP "/opt/local/bin/llvm-objdump")
SET (CMAKE_RANLIB  "/opt/local/bin/llvm-ranlib")

project(DreamGE)

add_executable(DreamGE main.cpp)