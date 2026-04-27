tpl_board(
  NAME         "generic"
  DESCRIPTION  "Generic ARM Cortex-M4 template board"
  LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/linker_script.ld"
  SOURCES      "${CMAKE_CURRENT_LIST_DIR}/time.c"
  INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}"
)
