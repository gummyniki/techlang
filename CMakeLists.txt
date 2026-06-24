cmake_minimum_required(VERSION 3.20)
project(techlang)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find llvm-config — adjust the path/version if needed
find_program(LLVM_CONFIG
    NAMES llvm-config llvm-config-17 llvm-config-18 llvm-config-19
    PATHS
        /usr/lib/llvm/17/bin
        /usr/lib/llvm/18/bin
        /usr/lib/llvm/21/bin
        /usr/bin
)

if(NOT LLVM_CONFIG)
    message(FATAL_ERROR "llvm-config not found!")
endif()

message(STATUS "Using llvm-config: ${LLVM_CONFIG}")

# get flags directly from llvm-config
execute_process(
    COMMAND ${LLVM_CONFIG} --cxxflags
    OUTPUT_VARIABLE LLVM_CXX_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${LLVM_CONFIG} --ldflags
    OUTPUT_VARIABLE LLVM_LD_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${LLVM_CONFIG} --libs core support object target mc mcparser passes native nativecodegen irreader passes x86asmparser x86codegen x86desc x86info
    OUTPUT_VARIABLE LLVM_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${LLVM_CONFIG} --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(techlang ${SOURCES})

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/std.o
    COMMAND gcc -c ${CMAKE_SOURCE_DIR}/std.c -o ${CMAKE_BINARY_DIR}/std.o -lm
    DEPENDS ${CMAKE_SOURCE_DIR}/std.c
    COMMENT "Compiling standard library..."
)

add_custom_target(stdlib ALL DEPENDS ${CMAKE_BINARY_DIR}/std.o)
add_dependencies(techlang stdlib)

target_include_directories(techlang PRIVATE src/)

# apply flags as strings
set_target_properties(techlang PROPERTIES
    COMPILE_FLAGS "${LLVM_CXX_FLAGS}"
    LINK_FLAGS "${LLVM_LD_FLAGS}"
)

# link against LLVM
target_link_libraries(techlang
    ${LLVM_LIBRARIES}
    ${LLVM_SYSTEM_LIBS}
)
