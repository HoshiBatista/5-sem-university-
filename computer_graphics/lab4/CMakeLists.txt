cmake_minimum_required(VERSION 3.10)
project(OpenGLLab3)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)

# Поиск необходимых библиотек
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(OPENGL REQUIRED gl)

# Добавление исполняемого файла
add_executable(opengl_lab3 main.cpp)

# Настройка компилятора
target_compile_options(opengl_lab3 PRIVATE ${SDL2_CFLAGS_OTHER})
target_compile_options(opengl_lab3 PRIVATE ${GLEW_CFLAGS_OTHER})
target_compile_options(opengl_lab3 PRIVATE ${OPENGL_CFLAGS_OTHER})

# Настройка линковки
target_link_libraries(opengl_lab3 ${SDL2_LIBRARIES})
target_link_libraries(opengl_lab3 ${GLEW_LIBRARIES})
target_link_libraries(opengl_lab3 ${OPENGL_LIBRARIES})

# Добавление путей к include
target_include_directories(opengl_lab3 PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(opengl_lab3 PRIVATE ${GLEW_INCLUDE_DIRS})
target_include_directories(opengl_lab3 PRIVATE ${OPENGL_INCLUDE_DIRS})