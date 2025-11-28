cmake_minimum_required(VERSION 3.16)
project(DonCEy_Kong_Jr C)
set(CMAKE_C_STANDARD 11)

# ----------------------------------------------------------
# 1. Toma todos los .c bajo clientC, pero luego filtramos
# ----------------------------------------------------------
file(GLOB_RECURSE ALL_CLIENT_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_SOURCE_DIR}/clientC/*.c")

# Elegir backend de red correcto por plataforma y excluir el opuesto
if (WIN32)
  list(FILTER ALL_CLIENT_SOURCES EXCLUDE REGEX "netPOSIX\\.c$")
else()
  list(FILTER ALL_CLIENT_SOURCES EXCLUDE REGEX "netWin32\\.c$")
endif()

# ----------------------------------------------------------
# 2. Fuentes comunes del cliente (SIN mains)
#    Excluimos clientPlayer.c y clientSpectator.c
# ----------------------------------------------------------
set(CLIENT_COMMON_SOURCES "")
foreach(src ${ALL_CLIENT_SOURCES})
  if (src MATCHES "launcher/launcher\\.c$" OR
      src MATCHES "clientPlayer/clientPlayerMain\\.c$" OR
      src MATCHES "clientSpectator/clientSpectatorMain\\.c$")
    # No los metemos en las fuentes comunes
  else()
    list(APPEND CLIENT_COMMON_SOURCES "${src}")
  endif()
endforeach()

# ----------------------------------------------------------
# 3. Ejecutable player (tiene main en clientPlayer.c)
# ----------------------------------------------------------
set(PLAYER_SOURCES
    ${CLIENT_COMMON_SOURCES}
    "${CMAKE_SOURCE_DIR}/clientC/clientPlayer/clientPlayerMain.c"
)

add_executable(player ${PLAYER_SOURCES})

# ----------------------------------------------------------
# 4. Ejecutable spectator (tiene main en clientSpectator.c)
# ----------------------------------------------------------
set(SPECTATOR_SOURCES
    ${CLIENT_COMMON_SOURCES}
    "${CMAKE_SOURCE_DIR}/clientC/clientSpectator/clientSpectatorMain.c"
)

add_executable(spectator ${SPECTATOR_SOURCES})

# ----------------------------------------------------------
# 4b. Ejecutable launcher (UI con Raylib)
# ----------------------------------------------------------
set(LAUNCHER_SOURCES
    ${CLIENT_COMMON_SOURCES}
    "${CMAKE_SOURCE_DIR}/clientC/launcher/launcher.c"
)

add_executable(launcher ${LAUNCHER_SOURCES})

# ----------------------------------------------------------
# 5. Includes: añade todas las carpetas que contienen .c
# ----------------------------------------------------------
set(INC_DIRS)
foreach(f ${ALL_CLIENT_SOURCES})
  get_filename_component(d "${f}" DIRECTORY)
  list(APPEND INC_DIRS "${d}")
endforeach()
list(REMOVE_DUPLICATES INC_DIRS)

target_include_directories(player    PRIVATE ${INC_DIRS})
target_include_directories(spectator PRIVATE ${INC_DIRS})
target_include_directories(launcher  PRIVATE ${INC_DIRS})

# ----------------------------------------------------------
# 6. Raylib
# ----------------------------------------------------------
if (WIN32)
  # Ruta típica de MSYS2/ucrt64
  set(raylib_DIR "C:/msys64/ucrt64/lib/cmake/raylib" CACHE STRING "raylib cmake dir")
  find_package(raylib CONFIG)
  if (raylib_FOUND)
    target_link_libraries(player    PRIVATE raylib)
    target_link_libraries(spectator PRIVATE raylib)
    target_link_libraries(launcher  PRIVATE raylib)
  else()
    # Fallback manual si no encuentra el package config
    target_link_libraries(player    PRIVATE raylib opengl32 gdi32 winmm ws2_32)
    target_link_libraries(spectator PRIVATE raylib opengl32 gdi32 winmm ws2_32)
    target_link_libraries(launcher  PRIVATE raylib opengl32 gdi32 winmm ws2_32)
  endif()

  target_link_libraries(player    PRIVATE ws2_32)
  target_link_libraries(spectator PRIVATE ws2_32)
  target_link_libraries(launcher  PRIVATE ws2_32)

else()
  if(APPLE)
    # --- macOS: usa raylib instalado con Homebrew ---
    find_package(raylib CONFIG REQUIRED)
    target_link_libraries(player    PRIVATE raylib)
    target_link_libraries(spectator PRIVATE raylib)
    target_link_libraries(launcher  PRIVATE raylib)

  else()  # Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED raylib)
    target_include_directories(player    PRIVATE ${RAYLIB_INCLUDE_DIRS})
    target_include_directories(spectator PRIVATE ${RAYLIB_INCLUDE_DIRS})
    target_include_directories(launcher  PRIVATE ${RAYLIB_INCLUDE_DIRS})
    target_link_libraries(player    PRIVATE ${RAYLIB_LIBRARIES})
    target_link_libraries(spectator PRIVATE ${RAYLIB_LIBRARIES})
    target_link_libraries(launcher  PRIVATE ${RAYLIB_LIBRARIES})
  endif()
endif()

# ----------------------------------------------------------
# 7. Platform-specific settings for better executables
# ----------------------------------------------------------
if (WIN32)
    # Hide console for launcher GUI on Windows
    set_target_properties(launcher PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()