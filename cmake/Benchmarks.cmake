option(TPL_ENABLE_BENCHMARKS "Build benchmark binaries" OFF)

set(TPL_BENCH_WARMUP_ITERS
    "100"
    CACHE STRING "Benchmark warmup iterations")
set(TPL_BENCH_MEASURE_ITERS
    "1000"
    CACHE STRING "Benchmark measured iterations")

set(TPL_BENCH_MAIN
    "${CMAKE_SOURCE_DIR}/bench/main.c"
    CACHE FILEPATH "Benchmark main.c")
set(TPL_BENCH_PROGRAM
    "${CMAKE_SOURCE_DIR}/bench/program.c"
    CACHE FILEPATH "Generated benchmark program.c")

function(tpl_add_benchmark_target target_name bench_mode)
  set(_bench_sources ${TPL_SRC_NO_MAIN} ${TPL_BENCH_MAIN} ${TPL_BENCH_PROGRAM})

  # Old tree: generated library C files are separate. New tree: no libaries are
  # generated
  if(DEFINED TPL_LIBRARY_GEN_C)
    list(APPEND _bench_sources ${TPL_LIBRARY_GEN_C})
  endif()

  add_executable(${target_name} ${_bench_sources})

  target_include_directories(
    ${target_name}
    PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}/generated"
            "${CMAKE_SOURCE_DIR}/bench")

  if(DEFINED TPL_INCLUDE_DIRS)
    target_include_directories(${target_name} PRIVATE ${TPL_INCLUDE_DIRS})
  endif()

  target_compile_definitions(
    ${target_name}
    PRIVATE _GNU_SOURCE VERSION="${TPL_GIT_VERSION}" ${bench_mode}
            BENCH_WARMUP_ITERS=${TPL_BENCH_WARMUP_ITERS}
            BENCH_MEASURE_ITERS=${TPL_BENCH_MEASURE_ITERS})

  if(DEFINED TPL_BACKEND)
    target_compile_definitions(${target_name} PRIVATE BACKEND="${TPL_BACKEND}")
  endif()

  if(DEFINED TPL_PROGRAM_PL AND TPL_PROGRAM_PL)
    target_compile_definitions(${target_name} PRIVATE TPL_HAS_PROGRAM_PL)
  endif()

  if(DEFINED TPL_BOARD_NAME AND TPL_BOARD_NAME)
    target_compile_definitions(${target_name}
                               PRIVATE BOARD_NAME="${TPL_BOARD_NAME}")
  endif()

  if(DEFINED TPL_BOARD_DESCRIPTION AND TPL_BOARD_DESCRIPTION)
    target_compile_definitions(
      ${target_name} PRIVATE BOARD_DESCRIPTION="${TPL_BOARD_DESCRIPTION}")
  endif()

  target_compile_options(
    ${target_name} PRIVATE -Wall -Wextra -Wno-unused-but-set-variable
                           -Wno-unused-parameter -Wno-unused-variable)

  if(DEFINED COMPILE_DIAGNOSTICS_FORMAT)
    target_compile_options(
      ${target_name} PRIVATE -fdiagnostics-format=${COMPILE_DIAGNOSTICS_FORMAT})
  endif()

  target_compile_options(
    ${target_name} PRIVATE -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
                           -fdiagnostics-color=auto)

  if(DEFINED _tpl_extra_cflags)
    target_compile_options(${target_name} PRIVATE ${_tpl_extra_cflags})
  endif()

  if(DEFINED _TPL_EXTRA_CFLAGS)
    target_compile_options(${target_name} PRIVATE ${_TPL_EXTRA_CFLAGS})
  endif()

  if(DEFINED _tpl_extra_ldflags)
    target_link_options(${target_name} PRIVATE ${_tpl_extra_ldflags})
  endif()

  if(DEFINED _TPL_EXTRA_LDFLAGS)
    target_link_options(${target_name} PRIVATE ${_TPL_EXTRA_LDFLAGS})
  endif()

  # Old tree dependency accumulator.
  if(DEFINED TPL_LINK_LIBS)
    target_link_libraries(${target_name} PRIVATE ${TPL_LINK_LIBS})
  endif()

  # New tree may not use TPL_LINK_LIBS. Also harmless if already linked.
  if(UNIX)
    target_link_libraries(${target_name} PRIVATE m)
  endif()

  if(DEFINED TPL_BACKEND AND TPL_BACKEND STREQUAL "linux")
    target_compile_definitions(${target_name} PRIVATE TPL_BACKEND_LINUX)

    if(DEFINED TPL_LINUX_SRC)
      target_sources(${target_name} PRIVATE ${TPL_LINUX_SRC})
    endif()

    if(EXISTS "${CMAKE_SOURCE_DIR}/src/platform/linux/board.c")
      target_sources(${target_name}
                     PRIVATE "${CMAKE_SOURCE_DIR}/src/platform/linux/board.c")
    endif()

    target_link_libraries(${target_name} PRIVATE readline)
  endif()
endfunction()

if(TPL_ENABLE_BENCHMARKS)
  tpl_add_benchmark_target(tpl-bench-cycles BENCH_MODE_CYCLES)
  tpl_add_benchmark_target(tpl-bench-runtime BENCH_MODE_RUNTIME)
  tpl_add_benchmark_target(tpl-bench-memory BENCH_MODE_MEMORY)

  target_compile_options(tpl-bench-cycles PRIVATE -O3 -DNDEBUG -march=native
                                                  -fno-omit-frame-pointer)

  target_compile_options(tpl-bench-runtime PRIVATE -O3 -DNDEBUG -march=native)

  target_compile_options(tpl-bench-memory PRIVATE -O2 -g -DNDEBUG
                                                  -fno-omit-frame-pointer)

  target_link_options(tpl-bench-cycles PRIVATE -Wl,--as-needed)
  target_link_options(tpl-bench-runtime PRIVATE -Wl,--as-needed)
  target_link_options(tpl-bench-memory PRIVATE -Wl,--as-needed)
endif()
