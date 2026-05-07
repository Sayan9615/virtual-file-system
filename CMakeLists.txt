cmake_minimum_required(VERSION 3.20)
project(POOProject)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/msvc2022_64")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

file(GLOB_RECURSE SOURCES "Program_source/*.cpp" "Program_source/*.h" "Program_source/*.hpp")
set(SQLITE_SOURCE "Program_source/external/sqlite3.c")

add_executable(ATMosFILE ${SOURCES} ${SQLITE_SOURCE})


set_target_properties(ATMosFILE PROPERTIES
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(ATMosFILE PRIVATE
    Program_source/external/libsodium/include
    Program_source/external
    Program_source/interfaces
    Program_source/filesystem
    Program_source/users
    Program_source/permissions
    Program_source/logger
    Program_source/services
    Program_source/exceptions
    Program_source/gui
)

target_link_libraries(ATMosFILE PRIVATE
    Qt6::Widgets
    Qt6::Core
    "${CMAKE_SOURCE_DIR}/Program_source/external/libsodium/x64/Release/v143/static/libsodium.lib"
)

target_compile_definitions(ATMosFILE PRIVATE SODIUM_STATIC)

find_program(WINDEPLOYQT windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
if(WINDEPLOYQT)
    add_custom_command(TARGET ATMosFILE POST_BUILD
        COMMAND "${WINDEPLOYQT}" "$<TARGET_FILE:ATMosFILE>"
        COMMENT "Deploying Qt DLLs..."
    )
endif()

add_custom_command(TARGET ATMosFILE POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/Program_source/data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Program_source/data"
        "${CMAKE_BINARY_DIR}/data"
    COMMENT "Copiind baza de date in folderul de executie..."
)