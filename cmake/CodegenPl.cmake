set(_tpl_needs_xxd FALSE)
if(TPL_EMBEDDED_LIBRARIES OR TPL_PROGRAM_PL)
  set(_tpl_needs_xxd TRUE)
endif()

find_program(XXD_EXECUTABLE xxd)
if(_tpl_needs_xxd AND NOT XXD_EXECUTABLE)
  message(FATAL_ERROR "xxd not found (needed to embed Prolog files).")
endif()

file(GLOB TPL_LIBRARY_PL "${CMAKE_SOURCE_DIR}/library/*.pl")

set(TPL_LIBRARY_GEN_C)
set(_tpl_lib_externs "")
set(_tpl_lib_entries "")

foreach(pl ${TPL_LIBRARY_PL})
  get_filename_component(stem "${pl}" NAME_WE)

  if(NOT TPL_EMBEDDED_LIBRARIES)
    continue()
  endif()

  list(FIND TPL_EMBEDDED_LIBRARIES "${stem}" _idx)
  if(_idx EQUAL -1)
    continue()
  endif()

  set(gen_c "${CMAKE_BINARY_DIR}/generated/library/${stem}.c")
  list(APPEND TPL_LIBRARY_GEN_C "${gen_c}")

  set(sym "library_${stem}_pl")

  add_custom_command(
    OUTPUT "${gen_c}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/generated/library"
    COMMAND ${CMAKE_COMMAND} -E echo "#include <stddef.h>" > "${gen_c}"
    COMMAND "${XXD_EXECUTABLE}" -i -n "${sym}" "${pl}" >> "${gen_c}"
    DEPENDS "${pl}"
    VERBATIM)

  string(APPEND _tpl_lib_externs
         "extern unsigned char ${sym}[];\nextern unsigned int ${sym}_len;\n")
  string(APPEND _tpl_lib_entries "    {\"${stem}\", ${sym}, &${sym}_len},\n")
endforeach()

# Always generate library_table.c (empty table when no libs selected)
set(_tpl_table_c "${CMAKE_BINARY_DIR}/generated/library_table.c")
list(APPEND TPL_LIBRARY_GEN_C "${_tpl_table_c}")

file(
  GENERATE
  OUTPUT "${_tpl_table_c}"
  CONTENT
    "#include \"library.h\"\n${_tpl_lib_externs}\nlibrary g_libs[] = {\n${_tpl_lib_entries}    {0}\n};\n"
)

if(TPL_PROGRAM_PL)
  set(_tpl_program_gen_c "${CMAKE_BINARY_DIR}/generated/program_pl.c")
  set(_tpl_program_tmp_c "${CMAKE_BINARY_DIR}/generated/program_pl.tmp.c")

  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

  execute_process(
    COMMAND "${XXD_EXECUTABLE}" -i -n "program_pl" "${TPL_PROGRAM_PL}"
    OUTPUT_FILE "${_tpl_program_tmp_c}" COMMAND_ERROR_IS_FATAL ANY)

  file(READ "${_tpl_program_tmp_c}" _tpl_program_c)

  string(
    REPLACE
      "unsigned char program_pl[]"
      "__attribute__((section(\".rodata.embedded_program\"), used, aligned(4)))\nconst unsigned char program_pl[]"
      _tpl_program_c
      "${_tpl_program_c}")

  string(
    REPLACE
      "unsigned int program_pl_len"
      "__attribute__((section(\".rodata.embedded_program_meta\"), used))\nconst unsigned int program_pl_len"
      _tpl_program_c
      "${_tpl_program_c}")

  file(WRITE "${_tpl_program_gen_c}" "#include <stddef.h>\n\n${_tpl_program_c}")

  list(APPEND TPL_LIBRARY_GEN_C "${_tpl_program_gen_c}")
endif()
