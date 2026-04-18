cmake_minimum_required(VERSION 3.20)
project(POOProject)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE SOURCES "Program_source/*.cpp")
set(SQLITE_SOURCE "Program_source/external/sqlite3.c")


add_executable(ATMosFILE ${SOURCES} ${SQLITE_SOURCE})


set_target_properties(ATMosFILE PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
)


target_include_directories(ATMosFILE PRIVATE
    Program_source/external
    Program_source/interfaces
    Program_source/filesystem
    Program_source/users
    Program_source/permissions
    Program_source/logger
    Program_source/services
    Program_source/services/exceptions
)