cmake_minimum_required(VERSION 3.20)
project(POOProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Forțează același runtime ca libsodium static (Release = MultiThreaded)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

file(GLOB_RECURSE SOURCES "Program_source/*.cpp")
set(SQLITE_SOURCE "Program_source/external/sqlite3.c")
set(HASH_SOURCE "Program_source/external/PasswordHasher.cpp")

add_executable(ATMosFILE ${SOURCES} ${SQLITE_SOURCE} ${HASH_SOURCE})

target_sources(ATMosFILE PRIVATE
    Program_source/external/PasswordHasher.cpp
)

set_target_properties(ATMosFILE PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
)

target_include_directories(ATMosFILE PRIVATE
    Program_source/external/libsodium/include      # <-- sodium.h
    Program_source/external
    Program_source/interfaces
    Program_source/filesystem
    Program_source/users
    Program_source/permissions
    Program_source/logger
    Program_source/services
    Program_source/services/exceptions
)

# Linkare libsodium static
target_link_libraries(ATMosFILE PRIVATE
    "${CMAKE_SOURCE_DIR}/Program_source/external/libsodium/x64/Release/v143/static/libsodium.lib"
)

# Necesar pentru linking static pe Windows
target_compile_definitions(ATMosFILE PRIVATE SODIUM_STATIC)