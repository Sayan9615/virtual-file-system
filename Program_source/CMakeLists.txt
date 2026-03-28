cmake_minimum_required(VERSION 3.20)
project(POOProject)

# Setăm standardul C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    Program_source/interfaces
    Program_source/filesystem
    Program_source/users
    Program_source/permissions
    Program_source/logger
    Program_source/services
)

# Adăugăm toate fișierele sursă din proiect
file(GLOB_RECURSE SOURCES "Program_source/*.cpp")

# Cream executabilul
add_executable(ATMosFILE ${SOURCES})