cmake_minimum_required(VERSION 3.16)

project(puzzle_server)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

find_package(Threads REQUIRED)

# SQLite3 paths
set(SQLite3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite3/include")
set(SQLite3_LIB_FILE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite3/lib/sqlite3.lib")
set(SQLite3_DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite3/bin")

# HTTPLIB path
set(HTTPLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/httplib")

# Проверка файлов
if(NOT EXISTS "${SQLite3_LIB_FILE}")
    message(FATAL_ERROR "SQLite3 library not found at: ${SQLite3_LIB_FILE}")
endif()

if(NOT EXISTS "${HTTPLIB_INCLUDE_DIR}/httplib.h")
    message(FATAL_ERROR "httplib.h not found at: ${HTTPLIB_INCLUDE_DIR}/httplib.h")
endif()

add_executable(puzzle_server
    src/main.cpp
    src/game/Game.cpp
    src/game/Puzzle.cpp
    src/adapter/Adapter.cpp
    src/server/Server.cpp
    src/database/Database.cpp
)

target_include_directories(puzzle_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SQLite3_INCLUDE_DIR}
    ${HTTPLIB_INCLUDE_DIR}
)

target_link_libraries(puzzle_server PRIVATE 
    "${SQLite3_LIB_FILE}"
    Threads::Threads
)

# Для Windows: сетевые библиотеки
if(WIN32)
    target_link_libraries(puzzle_server PRIVATE ws2_32 crypt32)
endif()

# Копируем DLL
add_custom_command(TARGET puzzle_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SQLite3_DLL_PATH}/sqlite3.dll"
    $<TARGET_FILE_DIR:puzzle_server>
)

# Копируем frontend
add_custom_command(TARGET puzzle_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/frontend"
    "$<TARGET_FILE_DIR:puzzle_server>/frontend"
)