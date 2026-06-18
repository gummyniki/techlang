cmake_minimum_required(VERSION 3.20)
project(techlang)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    IRReader
    Analysis
    Target
    MC
    CodeGen
)

message(STATUS "LLVM_LIBS = ${LLVM_LIBS}")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# grab all .cpp files in src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(techlang ${SOURCES})

target_include_directories(techlang PRIVATE
	src/
	${LLVM_INCLUDE_DIRS}
)


# nice compiler warnings to help catch bugs early
target_compile_options(techlang PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
