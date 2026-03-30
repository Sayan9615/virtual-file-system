cmake_minimum_required(VERSION 3.20)
project(POOProject)

# 1. Configurații de bază
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Colectăm toate sursele din subfoldere
file(GLOB_RECURSE SOURCES "Program_source/*.cpp")

# 3. Creăm executabilul
add_executable(ATMosFILE ${SOURCES})

# 4. FORȚĂM locația de output în folderul principal de build (pentru Windows/Visual Studio)
set_target_properties(ATMosFILE PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
)

# 5. Adăugăm folderele de include
target_include_directories(ATMosFILE PRIVATE 
    Program_source/interfaces
    Program_source/filesystem
    Program_source/users
    Program_source/permissions
    Program_source/logger
    Program_source/services
    Program_source/services/exceptions
)