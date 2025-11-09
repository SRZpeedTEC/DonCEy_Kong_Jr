cmake_minimum_required(VERSION 3.16)
project(DonCEy_Kong_Jr C)
set(CMAKE_C_STANDARD 11)

# Toma todos los .c bajo client-C, pero luego filtramos
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
     "${CMAKE_SOURCE_DIR}/client-C/*.c")

# Excluir archivos temporales comunes (VS Code, etc.)
list(FILTER SRC EXCLUDE REGEX "tempCodeRunnerFile\\.c$")

# Elegir backend de red correcto por plataforma y excluir el opuesto
if (WIN32)
  # Excluye POSIX en Windows
  list(FILTER SRC EXCLUDE REGEX "netPOSIX\\.c$")
  # Asegura que el Win32 esté incluido (si existe con otro nombre, ajústalo)
  # Nada que hacer si ya está en SRC (p.ej., client-C/client-Player/netWin32.c)
else()
  # En POSIX excluye el backend Win32
  list(FILTER SRC EXCLUDE REGEX "netWin32\\.c$")
endif()

# (Opcional) Si el spectator es otro ejecutable, quítalo de player
# list(FILTER SRC EXCLUDE REGEX "client-Spectator/.*\\.c$")

add_executable(player ${SRC})

# Includes: añade todas las carpetas que contienen .c (para encontrar sus .h)
set(INC_DIRS)
foreach(f ${SRC})
  get_filename_component(d "${f}" DIRECTORY)
  list(APPEND INC_DIRS "${d}")
endforeach()
list(REMOVE_DUPLICATES INC_DIRS)
target_include_directories(player PRIVATE ${INC_DIRS})

# Raylib
if (WIN32)
  # Ruta típica de MSYS2/ucrt64
  set(raylib_DIR "C:/msys64/ucrt64/lib/cmake/raylib" CACHE STRING "raylib cmake dir")
  find_package(raylib CONFIG)
  if (raylib_FOUND)
    target_link_libraries(player PRIVATE raylib)
  else()
    # Fallback manual si no encuentra el package config
    target_link_libraries(player PRIVATE raylib opengl32 gdi32 winmm ws2_32)
  endif()

  target_link_libraries(player PRIVATE ws2_32)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(RAYLIB raylib)
  if (RAYLIB_FOUND)
    target_include_directories(player PRIVATE ${RAYLIB_INCLUDE_DIRS})
    target_link_libraries(player PRIVATE ${RAYLIB_LIBRARIES})
  else()
    target_link_libraries(player PRIVATE raylib m dl pthread)
  endif()
endif()


