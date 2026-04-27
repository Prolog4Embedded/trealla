tpl_board(
  NAME
  "netduinoplus2"
  DESCRIPTION
  "Netduino Plus 2, Arm Cortex M4 @ 168 MHz"
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
  "${CMAKE_CURRENT_LIST_DIR}/hal/io/usart.c"
  INCLUDE_DIRS
  "${CMAKE_CURRENT_LIST_DIR}")
