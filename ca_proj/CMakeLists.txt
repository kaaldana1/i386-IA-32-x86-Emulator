cmake_minimum_required(VERSION 3.24)
project(hybrid_binary_translator LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep this experiment separate from the emulator's existing C Makefile.
# Fetch one known AsmJit revision into the build directory, with no global install.
include(FetchContent)
set(ASMJIT_STATIC ON)
set(ASMJIT_TEST OFF)
FetchContent_Declare(asmjit
    GIT_REPOSITORY https://github.com/asmjit/asmjit.git
    GIT_TAG 677a10bac2c24f7e068cf1f9e675bf51c7b534ea
)
FetchContent_MakeAvailable(asmjit)

add_executable(hybrid_binary_translator
    hybrid_binary_translator.cpp
    ../src/core/internal/decoder.c
    ../src/tables/decode_tables.c
)
target_include_directories(hybrid_binary_translator PRIVATE ../include)
# Keep third-party header extensions out of this target's pedantic warnings.
target_include_directories(hybrid_binary_translator SYSTEM PRIVATE "${asmjit_SOURCE_DIR}")
target_compile_features(hybrid_binary_translator PRIVATE cxx_std_20 c_std_11)
set_target_properties(hybrid_binary_translator PROPERTIES
    C_EXTENSIONS OFF
    CXX_EXTENSIONS OFF
)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hybrid_binary_translator PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_link_libraries(hybrid_binary_translator PRIVATE asmjit::asmjit)
