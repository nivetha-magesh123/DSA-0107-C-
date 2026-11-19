cmake_minimum_required(VERSION 3.20)
project(SmartFoodRedistribution LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Crow CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_executable(smart_food_server
    src/main.cpp
    src/database.cpp
)

target_include_directories(smart_food_server PRIVATE src)
target_link_libraries(smart_food_server PRIVATE Crow::Crow unofficial::sqlite3::sqlite3)

if(WIN32)
    target_link_libraries(smart_food_server PRIVATE ws2_32)
endif()

file(COPY web DESTINATION ${CMAKE_BINARY_DIR})
