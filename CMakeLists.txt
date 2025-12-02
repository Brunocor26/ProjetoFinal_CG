cmake_minimum_required(VERSION 3.16)
project(maze C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ajudar CMake a encontrar libs via Homebrew (Apple Silicon)
list(APPEND CMAKE_PREFIX_PATH /opt/homebrew /opt/homebrew/lib/cmake)

# Dependências
find_package(glfw3 REQUIRED)   # alvo: glfw
find_package(GLEW REQUIRED)    # GLEW required for host/client

# ==========================================
# Target: maze.exe (Main App - Uses GLAD)
# ==========================================
add_executable(maze
  src_prototipo/main.cpp
  src/objloader.cpp
  src/glad.c
)

configure_file(configuration/root_directory.h.in configuration/root_directory.h)

target_include_directories(maze PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/common
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/configuration
)

target_link_libraries(maze PRIVATE dl)
target_link_libraries(maze PRIVATE glfw)

if (APPLE)
  target_compile_definitions(maze PRIVATE GL_SILENCE_DEPRECATION)
  target_link_libraries(maze PRIVATE "-framework OpenGL")
elseif (WIN32)
  target_link_libraries(maze PRIVATE opengl32)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(maze PRIVATE OpenGL::GL)
endif()

set_target_properties(maze PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  OUTPUT_NAME "maze.exe"
)

# ==========================================
# Target: host.exe (Server - Uses GLAD)
# ==========================================
add_executable(host
  src_prototipo/host.cpp
  src/objloader.cpp
  src/glad.c
)

target_include_directories(host PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/common
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/configuration
)

target_link_libraries(host PRIVATE dl)
target_link_libraries(host PRIVATE glfw)

if (APPLE)
  target_compile_definitions(host PRIVATE GL_SILENCE_DEPRECATION)
  target_link_libraries(host PRIVATE "-framework OpenGL")
elseif (WIN32)
  target_link_libraries(host PRIVATE opengl32)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(host PRIVATE OpenGL::GL)
endif()

set_target_properties(host PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  OUTPUT_NAME "host.exe"
)

# ==========================================
# Target: client.exe (Client - Uses GLEW)
# ==========================================
add_executable(client
  src_prototipo/cliente.cpp
  src/objloader.cpp
  src/shader.cpp
)

target_include_directories(client PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/common
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/configuration
  ${GLEW_INCLUDE_DIRS}
)

target_link_libraries(client PRIVATE dl)
target_link_libraries(client PRIVATE glfw)
target_link_libraries(client PRIVATE ${GLEW_LIBRARIES})

if (APPLE)
  target_compile_definitions(client PRIVATE GL_SILENCE_DEPRECATION)
  target_link_libraries(client PRIVATE "-framework OpenGL")
elseif (WIN32)
  target_link_libraries(client PRIVATE opengl32)
else()
  target_link_libraries(client PRIVATE OpenGL::GL)
endif()

set_target_properties(client PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  OUTPUT_NAME "client.exe"
)

# ==========================================
# Custom Commands (Assets)
# ==========================================

# Copiar shaders/ para a build
add_custom_command(TARGET maze POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    ${CMAKE_BINARY_DIR}/shaders
    COMMENT "A copiar pasta 'shaders/' para a build..."
)

# Copiar deer.obj para a diretoria de build
add_custom_command(TARGET maze POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/assets/deer.obj
    ${CMAKE_BINARY_DIR}/deer.obj
    COMMENT "A copiar 'deer.obj' para a build..."
)

# Copiar deer.mtl para a diretoria de build
add_custom_command(TARGET maze POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/assets/deer.mtl
    ${CMAKE_BINARY_DIR}/deer.mtl
    COMMENT "A copiar 'deer.mtl' para a build..."
)