include(ExternalProject)

find_program(MESON_EXECUTABLE meson)
find_program(NINJA_EXECUTABLE ninja)

if (NOT MESON_EXECUTABLE)
    message(NOTICE "meson can be found here 'https://github.com/mesonbuild/meson'")
    message(FATAL_ERROR "meson not found (needed to compile picolibc)")
endif()

if(NOT NINJA_EXECUTABLE)
    message(NOTICE "ninja can be found here 'https://github.com/ninja-build/ninja'")
    message(FATAL_ERROR "ninja not found (needed to compile picolibc)")
endif()

if(NOT TPL_PICOLIBC_CROSS_FILE)
  message(FATAL_ERROR "TPL_PICOLIBC_CROSS_FILE must be set for picolibc backend")
endif()

# Map CMake build type to meson optimization level / debug flag
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(_picolibc_optimization "s") # 0 is too large lol
  set(_picolibc_debug "true")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(_picolibc_optimization "2")
  set(_picolibc_debug "true")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(_picolibc_optimization "2")
  set(_picolibc_debug "false")
else()
  set(_picolibc_optimization "s")
  set(_picolibc_debug "false")
endif()

set(_picolibc_prefix "${CMAKE_BINARY_DIR}/_picolibc")
set(_picolibc_build "${_picolibc_prefix}/build")
set(_picolibc_install "${_picolibc_prefix}/install")

if(TPL_USE_SEMIHOST)
  set(_picolibc_semihost "true")
  set(_picolibc_byproducts
      "${_picolibc_install}/lib/libc.a"
      "${_picolibc_install}/lib/libsemihost.a"
      "${_picolibc_install}/lib/libcrt0.a"
      "${_picolibc_install}/lib/picolibc.specs")
else()
  set(_picolibc_semihost "false")
  set(_picolibc_byproducts
      "${_picolibc_install}/lib/libc.a"
	  # TODO: libcrt0.a yes or no?
      "${_picolibc_install}/lib/picolibc.specs")
endif()

ExternalProject_Add(
  picolibc_ext
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/picolibc"
  BINARY_DIR "${_picolibc_build}"
  CONFIGURE_COMMAND
    "${MESON_EXECUTABLE}" setup "${_picolibc_build}" "${CMAKE_SOURCE_DIR}/picolibc" #
        --cross-file "${TPL_PICOLIBC_CROSS_FILE}" #
        --prefix "${_picolibc_install}" #
        -Dmultilib=false #
        -Dsemihost=${_picolibc_semihost} #
        -Dpicocrt=true #
        -Dpicocrt-lib=true #
        -Denable-malloc=true #
        -Dthread-local-storage=false #
        -Dthread-local-storage-api=false #
        -Dspecsdir=${_picolibc_install}/lib #
        -Doptimization=${_picolibc_optimization} #
        -Ddebug=${_picolibc_debug} #
  BUILD_COMMAND "${MESON_EXECUTABLE}" compile -C "${_picolibc_build}"
  INSTALL_COMMAND "${MESON_EXECUTABLE}" install -C "${_picolibc_build}"
  BUILD_BYPRODUCTS ${_picolibc_byproducts}
 )

set(PICOLIBC_INSTALL_DIR
    "${_picolibc_install}"
    CACHE INTERNAL "")
set(PICOLIBC_SPECS_FILE
    "${_picolibc_install}/lib/picolibc.specs"
    CACHE INTERNAL "")
