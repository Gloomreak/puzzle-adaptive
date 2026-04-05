cmake_minimum_required(VERSION 3.16)

# Найти Crow (веб-фреймворк)
find_package(Crow QUIET)

# Если не найден — используем submodule (позже настроим)

add_executable(puzzle_server
    src/main.cpp
    src/game/Game.cpp
    src/game/Puzzle.cpp
    src/adapter/Adapter.cpp
    src/server/Server.cpp
    src/database/Database.cpp
)

target_include_directories(puzzle_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# SQLite
find_package(SQLite3 REQUIRED)
target_link_libraries(puzzle_server PRIVATE SQLite::SQLite3)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(puzzle_server PRIVATE Threads::Threads)