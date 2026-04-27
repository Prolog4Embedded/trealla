tpl_board(
  NAME
  "mps2-an500"
  DESCRIPTION
  "mps2-an500, Arm Cortex M7"
  LINKER_SCRIPT
  "${CMAKE_CURRENT_LIST_DIR}/linker_script.ld"
  PICOLIBC_CROSS_FILE
  "${CMAKE_CURRENT_LIST_DIR}/picolibc-cross.txt"
  USE_SEMIHOST
  OFF
  SOURCES
  "${CMAKE_CURRENT_LIST_DIR}/startup.c"
  "${CMAKE_CURRENT_LIST_DIR}/hal/board.c"
  "${CMAKE_CURRENT_LIST_DIR}/hal/time.c"
  "${CMAKE_CURRENT_LIST_DIR}/hal/io/uart.c"
  INCLUDE_DIRS
  "${CMAKE_CURRENT_LIST_DIR}")
