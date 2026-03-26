cmake_minimum_required(VERSION 3.25)

project(tpl C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(Options)
include(Version)
include(CodegenLibraryPl)
include(Dependencies)
include(Testing)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(TPL_PLATFORM "linux" CACHE STRING "Target platform backend")
set_property(CACHE TPL_PLATFORM PROPERTY STRINGS linux baremetal)

add_compile_definitions(
  _GNU_SOURCE
  VERSION="${TPL_GIT_VERSION}"
  TARGET="${TPL_PLATFORM}"
)

add_compile_options(
  -Wall -Wextra
  -Wno-unused-but-set-variable
  -Wno-unused-parameter
  -Wno-unused-variable
)

separate_arguments(_TPL_EXTRA_CFLAGS UNIX_COMMAND "${TPL_EXTRA_CFLAGS}")
separate_arguments(_TPL_EXTRA_LDFLAGS UNIX_COMMAND "${TPL_EXTRA_LDFLAGS}")

set(TPL_CORE_SRC
  src/core/compile.c
  src/core/heap.c
  src/core/module.c
  src/core/parser.c
  src/core/print.c
  src/core/prolog.c
  src/core/query.c
  src/core/terms.c
  src/core/unify.c
  src/version.c
)

set(TPL_BUILTINS_SRC
  src/builtins/bif_atts.c
  src/builtins/bif_bboard.c
  src/builtins/bif_control.c
  src/builtins/bif_csv.c
  src/builtins/bif_database.c
  src/builtins/bif_ffi.c
  src/builtins/bif_format.c
  src/builtins/bif_functions.c
  src/builtins/bif_maps.c
  src/builtins/bif_os.c
  src/builtins/bif_posix.c
  src/builtins/bif_predicates.c
  src/builtins/bif_sort.c
  src/builtins/bif_sregex.c
  src/builtins/bif_streams.c
  src/builtins/bif_tasks.c
  src/builtins/bif_threads.c
)

set(TPL_UTIL_SRC
  src/util/base64.c
  src/util/list.c
  src/util/skiplist.c
  src/util/utf8.c
)

set(TPL_SUPPORT_SRC
  src/support/history.c
  src/support/library.c
  src/support/network.c
  src/support/toplevel.c
)

set(TPL_VENDOR_SRC
  src/vendor/imath/imath.c
  src/vendor/imath/imrat.c
  src/vendor/sre/re.c
)

set(TPL_PLATFORM_COMMON_SRC
	# src/platform/common/...
)

if(TPL_PLATFORM STREQUAL "linux")
	set(TPL_PLATFORM_SRC
		# src/platform/linux/...
	)
elseif(TPL_PLATFORM STREQUAL "baremetal")
	set(TPL_PLATFORM_SRC
		# src/platform/baremetal/...
	)
	#TODO: Board specific implementations
else()
	message(FATAL_ERROR("Unsupported TPL_PLATFORM='${TPL_PLATFORM}'"))
endif()

add_library(tpl_core OBJECT
  ${TPL_CORE_SRC}
  ${TPL_BUILTINS_SRC}
  ${TPL_UTIL_SRC}
  ${TPL_SUPPORT_SRC}
  ${TPL_VENDOR_SRC}
  ${TPL_PLATFORM_COMMON_SRC}
  ${TPL_PLATFORM_SRC}
  ${TPL_LIBRARY_GEN_C}
)

# I would prefer to only have src/ and src/platform/api as include directories
# and then reference e.g. with include "core/polog.c", but that would mean having
# to go through all sources and updating imports.
set(TPL_INCLUDE_DIRS
	"${CMAKE_SOURCE_DIR}/src"
	"${CMAKE_SOURCE_DIR}/src/builtins"
	"${CMAKE_SOURCE_DIR}/src/core"
	"${CMAKE_SOURCE_DIR}/src/support"
	"${CMAKE_SOURCE_DIR}/src/util"
	"${CMAKE_SOURCE_DIR}/src/platform/api"
	"${CMAKE_SOURCE_DIR}/src/vendor"
	"${CMAKE_SOURCE_DIR}/generated"
)

target_include_directories(tpl_core PRIVATE ${TPL_INCLUDE_DIRS})

target_compile_options(tpl_core PRIVATE ${_TPL_EXTRA_CFLAGS})
target_link_options(tpl_core PRIVATE ${_TPL_EXTRA_LDFLAGS})

add_executable(tpl
	tpl.c
	$<TARGET_OBJECTS:tpl_core>
)

target_link_libraries(tpl PRIVATE ${TPL_LINK_LIBS})
target_include_directories(tpl PRIVATE ${TPL_INCLUDE_DIRS})


set(MAIN_C "${CMAKE_BINARY_DIR}/generated/main.c")

if(MAIN_PL)
  add_custom_command(
    OUTPUT "${MAIN_C}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/generated"
    COMMAND ${CMAKE_COMMAND} -E echo "#include <stddef.h>" > "${MAIN_C}"
    COMMAND "${XXD_EXECUTABLE}" -i "${MAIN_PL}" >> "${MAIN_C}"
    DEPENDS "${MAIN_PL}"
    VERBATIM
  )

  add_library(tpl_core OBJECT
    ${TPL_SRC}
    ${TPL_LIBRARY_GEN_C}
  )
  target_include_directories(tpl_core PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/generated"
  )
  target_compile_options(tpl_core PRIVATE ${_TPL_EXTRA_CFLAGS})

  add_executable(compile_main
    $<TARGET_OBJECTS:tpl_core>
    "${MAIN_C}"
  )
  target_compile_definitions(compile_main PRIVATE USE_MAIN=1)
  target_link_options(compile_main PRIVATE ${_TPL_EXTRA_LDFLAGS})
  target_link_libraries(compile_main PRIVATE ${TPL_LINK_LIBS})

  add_custom_target(compile DEPENDS compile_main)
else()
  add_custom_target(compile
    COMMAND ${CMAKE_COMMAND} -E echo "Set -DMAIN_PL=/abs/path/to/main.pl to enable compile_main"
  )
endif()

# include(GNUInstallDirs)
# install(TARGETS tpl RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
