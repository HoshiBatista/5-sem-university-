cmake_minimum_required(VERSION 3.10)
project(RGZ_Graphics)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки компилятора
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Поиск необходимых библиотек
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)

# Включаем директории
include_directories(${SDL2_INCLUDE_DIRS})

# Создание исполняемого файла
add_executable(rgz main.cpp)

# Линковка библиотек
target_link_libraries(rgz
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    GLEW
    GL
)

