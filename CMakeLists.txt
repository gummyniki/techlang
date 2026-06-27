cmake_minimum_required(VERSION 3.20)
project(techlang)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    COMMAND ${LLVM_CONFIG} --libs core support object target mc mcparser
            passes native nativecodegen irreader
            x86asmparser x86codegen x86desc x86info
            nvptx nvptxcodegen nvptxdesc nvptxinfo
    OUTPUT_VARIABLE LLVM_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${LLVM_CONFIG} --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# compile stdlib at configure time
execute_process(
    COMMAND gcc -c ${CMAKE_SOURCE_DIR}/stdlib/std.c
            -o ${CMAKE_BINARY_DIR}/stdlib.o -lm
    RESULT_VARIABLE STDLIB_RESULT
)

if(NOT STDLIB_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to compile standard library!")
else()
  message(STATUS "Standard library compiled successfully")
endif()

separate_arguments(LLVM_CXX_FLAGS_LIST UNIX_COMMAND "${LLVM_CXX_FLAGS}")
separate_arguments(LLVM_LD_FLAGS_LIST UNIX_COMMAND "${LLVM_LD_FLAGS}")

# ✅ Techlang compiler — exclude vectec files
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*vectec_main\\.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*gpu_generator\\.cpp")

add_executable(techlang ${SOURCES})

target_include_directories(techlang PRIVATE
    src/
    src/frontend/
    src/analysis/
    src/backend/
)

target_compile_options(techlang PRIVATE ${LLVM_CXX_FLAGS_LIST} -fexceptions)
target_link_options(techlang PRIVATE ${LLVM_LD_FLAGS_LIST})
target_link_libraries(techlang
    ${LLVM_LIBRARIES}
    ${LLVM_SYSTEM_LIBS}
)

# copy std.tec to build directory after build
add_custom_command(
    TARGET techlang POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/stdlib/std.tec
        ${CMAKE_BINARY_DIR}/std.tec
    COMMENT "Copying std.tec to build directory..."
)

# ✅ VecTec compiler
file(GLOB_RECURSE VECTEC_SOURCES
    "src/frontend/lexer.cpp"
    "src/frontend/parser.cpp"
    "src/frontend/import_resolver.cpp"
    "src/analysis/semantic_analyzer.cpp"
    "src/backend/gpu_generator.cpp"
"src/backend/gpu_runtime_generator.cpp"
    "src/vectec_main.cpp"
)

add_executable(vectec ${VECTEC_SOURCES})

target_include_directories(vectec PRIVATE
    src/
    src/frontend/
    src/analysis/
    src/backend/
)

target_compile_options(vectec PRIVATE ${LLVM_CXX_FLAGS_LIST} -fexceptions)
target_link_options(vectec PRIVATE ${LLVM_LD_FLAGS_LIST})
target_link_libraries(vectec
    ${LLVM_LIBRARIES}
    ${LLVM_SYSTEM_LIBS}
)
