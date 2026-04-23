cmake_minimum_required(VERSION 3.25)

project(tpl LANGUAGES C ASM)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(
    WARNING "No build or configuration type defined, defaulting to 'Release'")
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Build type" FORCE)
endif()

# Backend / libc configuration

include(Options)
include(Version)

# Board API — must run before Picolibc so board-declared hardware variables are visible.

if(TPL_BOARD_CMAKE)
  include(BoardAPI)
  include("${TPL_BOARD_CMAKE}")
endif()

# Optional generated sources / external dependencies

include(CodegenPl)

if(TPL_BACKEND STREQUAL "linux")
  include(Testing)
elseif(TPL_LIBC_BACKEND STREQUAL "picolibc")
  include(Picolibc)
endif()

# Language / diagnostics

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

separate_arguments(_tpl_extra_cflags UNIX_COMMAND "${TPL_EXTRA_CFLAGS}")
separate_arguments(_tpl_extra_ldflags UNIX_COMMAND "${TPL_EXTRA_LDFLAGS}")

# Source files

set(TPL_CORE_SRC
    tpl.c
    src/base64.c
    src/bif_atts.c
    src/bif_bboard.c
    src/bif_control.c
    src/bif_csv.c
    src/bif_database.c
    src/bif_ffi.c
    src/bif_format.c
    src/bif_functions.c
    src/bif_maps.c
    src/bif_os.c
    src/bif_posix.c
    src/bif_predicates.c
    src/bif_sort.c
    src/bif_sregex.c
    src/bif_streams.c
    src/bif_tasks.c
    src/bif_threads.c
    src/compile.c
    src/heap.c
    src/history.c
    src/list.c
    src/module.c
    src/network.c
    src/parser.c
    src/print.c
    src/prolog.c
    src/query.c
    src/skiplist.c
    src/terms.c
    src/toplevel.c
    src/unify.c
    src/utf8.c
    src/version.c
    src/board.c)

set(TPL_VENDOR_SRC src/imath/imath.c src/imath/imrat.c src/sre/re.c)

set(TPL_PLATFORM_COMMON_SRC src/platform/common/pl4bm_debug.c)

set(TPL_LINUX_SRC
    src/platform/linux/memory_linux.c
    src/platform/common/memory_stats.c)

set(TPL_NOLIBC_SRC src/platform/baremetal/memory_baremetal.c)
set(TPL_PICOLIBC_SRC src/platform/baremetal/memory_baremetal.c)

set(TPL_BAREMETAL_STUBS_SRC src/platform/baremetal/stubs.c)

include(Formatting)
include(Linting)
include(Hooks)

file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
     "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC)

# Collect target sources / include dirs

set(TPL_ALL_SRC ${TPL_CORE_SRC} ${TPL_VENDOR_SRC} ${TPL_PLATFORM_COMMON_SRC})

list(APPEND TPL_ALL_SRC ${TPL_BOARD_SRCS})

list(APPEND TPL_ALL_SRC ${TPL_LIBRARY_GEN_C})

set(TPL_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/src/platform"
    "${CMAKE_SOURCE_DIR}/src/platform/api" "${CMAKE_BINARY_DIR}/generated"
    ${TPL_BOARD_INCLUDE_DIRS})

if(TPL_BACKEND STREQUAL "linux")
  list(APPEND TPL_ALL_SRC ${TPL_LINUX_SRC})

elseif(TPL_BACKEND STREQUAL "baremetal")
  if(TPL_LIBC_BACKEND STREQUAL "picolibc")
    list(APPEND TPL_ALL_SRC ${TPL_PICOLIBC_SRC})
  else()
    list(APPEND TPL_ALL_SRC ${TPL_NOLIBC_SRC})

    if(TPL_LIBC_BACKEND STREQUAL "none")
      list(APPEND TPL_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/compat")
    endif()
  endif()

  if(TPL_ENABLE_POSIX_STUBS)
    list(APPEND TPL_ALL_SRC ${TPL_BAREMETAL_STUBS_SRC})
  endif()
else()
  message(FATAL_ERROR "Unsupported TPL_BACKEND='${TPL_BACKEND}'")
endif()

# Target

add_executable(tpl ${TPL_ALL_SRC})

target_include_directories(tpl PRIVATE ${TPL_INCLUDE_DIRS})

target_compile_definitions(tpl PRIVATE _GNU_SOURCE VERSION="${TPL_GIT_VERSION}"
                                       BACKEND="${TPL_BACKEND}")

if(TPL_PROGRAM_PL)
  target_compile_definitions(tpl PRIVATE TPL_HAS_PROGRAM_PL)
endif()

if(TPL_BOARD_NAME)
  target_compile_definitions(tpl PRIVATE BOARD_NAME="${TPL_BOARD_NAME}")
endif()
if(TPL_BOARD_DESCRIPTION)
  target_compile_definitions(tpl PRIVATE BOARD_DESCRIPTION="${TPL_BOARD_DESCRIPTION}")
endif()

if(TPL_BACKEND STREQUAL "linux")
  target_compile_definitions(tpl PRIVATE TPL_BACKEND_LINUX)
elseif(TPL_BACKEND STREQUAL "baremetal")
  target_compile_definitions(tpl PRIVATE TPL_BACKEND_BAREMETAL USE_MMAP=0)

# TODO: Dont like all these hardcoded flags!
target_compile_options(tpl PRIVATE
  -Os
  -g
  -ffunction-sections
  -fdata-sections
)

target_link_options(tpl PRIVATE
  -Wl,--gc-sections
  -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/tpl.map
)

  if(TPL_LIBC_BACKEND STREQUAL "picolibc")
    target_compile_definitions(tpl PRIVATE TPL_LIBC_BACKEND_PICOLIBC)

    if(TPL_USE_SEMIHOST)
      target_compile_definitions(tpl PRIVATE TPL_USE_SEMIHOST=1)
    endif()
  elseif(TPL_LIBC_BACKEND STREQUAL "none")
    target_compile_definitions(tpl PRIVATE TPL_LIBC_BACKEND_NONE)
  endif()
endif()

# Compile / link options

target_compile_options(
  tpl
  PRIVATE -Wall
          -Wextra
          -Wno-unused-but-set-variable
          -Wno-unused-parameter
          -Wno-unused-variable
          -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
          -fdiagnostics-format=${COMPILE_DIAGNOSTICS_FORMAT}
          -fdiagnostics-color=auto
          ${_tpl_extra_cflags})

target_link_options(tpl PRIVATE ${_tpl_extra_ldflags})

if(TPL_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT _ipo_ok OUTPUT _ipo_msg)
  if(_ipo_ok)
    set_property(TARGET tpl PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "IPO/LTO not supported: ${_ipo_msg}")
  endif()
endif()

# Linux link setup

if(TPL_BACKEND STREQUAL "linux")
  target_link_libraries(tpl PRIVATE m readline)
endif()

# Picolibc integration

if(TPL_LIBC_BACKEND STREQUAL "picolibc")
  add_dependencies(tpl picolibc_ext)

  target_include_directories(tpl PRIVATE "${PICOLIBC_INSTALL_DIR}/include")

  target_link_directories(tpl PRIVATE "${PICOLIBC_INSTALL_DIR}/lib")

  target_compile_options(tpl PRIVATE "--specs=${PICOLIBC_SPECS_FILE}")

  target_link_options(tpl PRIVATE "--specs=${PICOLIBC_SPECS_FILE}")

  target_link_libraries(tpl PRIVATE c gcc)

  if(NOT TPL_ENTRY)
    target_link_libraries(tpl PRIVATE crt0-minimal)
  endif()

  if(TPL_USE_SEMIHOST)
    target_link_libraries(tpl PRIVATE semihost)
    if(NOT TPL_ENTRY)
      target_link_libraries(tpl PRIVATE crt0-semihost)
    endif()
  endif()

endif()

# Bare-metal setup

if(TPL_BACKEND STREQUAL "baremetal")
  set_target_properties(tpl PROPERTIES SUFFIX ".elf")

  if(TPL_ENTRY)
    target_sources(tpl PRIVATE "${TPL_ENTRY}")
  else()
    if(TPL_LIBC_BACKEND STREQUAL "picolibc")
      message(STATUS "Using picolibc startup code")
    else()
      message(WARNING "Baremetal backend: no TPL_ENTRY startup source defined.")
    endif()
  endif()

  if(TPL_LINKER_SCRIPT)
    set_target_properties(tpl PROPERTIES LINK_DEPENDS "${TPL_LINKER_SCRIPT}")

    target_link_options(tpl PRIVATE "-T" "${TPL_LINKER_SCRIPT}")
  else()
    message(WARNING "Baremetal backend: no TPL_LINKER_SCRIPT defined.")
  endif()

  target_link_options(tpl PRIVATE "LINKER:-Map=${CMAKE_BINARY_DIR}/tpl.map")

  set(_tpl_bin "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_BASE_NAME:tpl>.bin")

  set_target_properties(tpl PROPERTIES ADDITIONAL_CLEAN_FILES "${_tpl_bin}")

  add_custom_command(
    TARGET tpl
    POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" ARGS -O binary $<TARGET_FILE:tpl>
            $<TARGET_FILE_BASE_NAME:tpl>.bin
    VERBATIM)
endif()
