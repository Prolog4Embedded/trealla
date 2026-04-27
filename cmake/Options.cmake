option(TPL_LTO "Enable link-time optimization (IPO/LTO)" OFF)

# Backend / libc (naming kind of weird, backend means linux or baremetal, libc_backend means which libc to use)

if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
  set(_tpl_backend_default "baremetal")
else()
  set(_tpl_backend_default "linux")
endif()

set(TPL_BACKEND
    "${_tpl_backend_default}"
    CACHE STRING "Runtime backend (linux|baremetal)")
set_property(CACHE TPL_BACKEND PROPERTY STRINGS linux baremetal)

if(NOT DEFINED TPL_LIBC_BACKEND)
  set(TPL_LIBC_BACKEND
      "system"
      CACHE STRING "C library backend (system|none|picolibc)")
elseif((NOT DEFINED TPL_LIBC_BACKEND OR TPL_LIBC_BACKEND STREQUAL "system") AND TPL_BACKEND STREQUAL "baremetal")
	message(WARNING "Backend is set to 'baremetal', but trying to use 'system' libc!")
endif()
set_property(CACHE TPL_LIBC_BACKEND PROPERTY STRINGS system none picolibc)

# Embedded Prolog content (libraries and/or embedded program)

set(TPL_EMBEDDED_LIBRARIES
    ""
    CACHE STRING
          "Library *.pl stems to embed, semicolon-separated (empty = none)")

set(TPL_PROGRAM_PL
    ""
    CACHE FILEPATH
          "Path to a Prolog file to embed as the program (sets program_pl / program_pl_len in tpl.c)")

# Build customization

set(TPL_EXTRA_CFLAGS
    ""
    CACHE STRING "Extra C compiler flags")
set(TPL_EXTRA_LDFLAGS
    ""
    CACHE STRING "Extra linker flags")

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(_default_diag_format "clang")
else()
  set(_default_diag_format "text")
endif()
set(COMPILE_DIAGNOSTICS_FORMAT
    "${_default_diag_format}"
    CACHE STRING "Compiler diagnostics format (GCC: text|json|json-file; Clang: clang|msvc|vi)")

# Bare-metal / embedded options

set(TPL_USE_SEMIHOST
    ON
    CACHE BOOL "Enable semihosting support")
set(TPL_ENABLE_POSIX_STUBS
    ON
    CACHE BOOL "Link POSIX-like stub implementations for bare metal")

set(TPL_BOARD_CMAKE
    ""
    CACHE FILEPATH
          "Per-board cmake file; appends to TPL_BOARD_SRCS and TPL_BOARD_INCLUDE_DIRS")

# Picolibc (only used when TPL_LIBC_BACKEND=picolibc)

set(TPL_PICOLIBC_CROSS_FILE
    ""
    CACHE FILEPATH "Meson cross file for building picolibc")
