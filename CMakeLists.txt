cmake_minimum_required(VERSION 3.20)
project(rgb-controller LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(HIDAPI REQUIRED hidapi-hidraw)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

add_executable(rgb-controller
  src/main.cpp
  src/rgb_controller_hid.cpp
  src/gui.cpp
)

target_include_directories(rgb-controller PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${HIDAPI_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
)

target_compile_options(rgb-controller PRIVATE
  -Wall -Wextra -Wpedantic
  ${HIDAPI_CFLAGS_OTHER}
  ${GTKMM_CFLAGS_OTHER}
)

target_link_directories(rgb-controller PRIVATE
  ${HIDAPI_LIBRARY_DIRS}
  ${GTKMM_LIBRARY_DIRS}
)

target_link_libraries(rgb-controller PRIVATE
  ${HIDAPI_LIBRARIES}
  ${GTKMM_LIBRARIES}
)
