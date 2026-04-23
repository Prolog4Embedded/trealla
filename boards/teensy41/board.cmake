tpl_board(
  NAME                "teensy41"
  DESCRIPTION         "Teensy 4.1, ARM Cortex-M7, picolibc"
  LINKER_SCRIPT       "${CMAKE_CURRENT_LIST_DIR}/linker_script.ld"
  PICOLIBC_CROSS_FILE "${CMAKE_CURRENT_LIST_DIR}/picolibc-cross.txt"
  USE_SEMIHOST        OFF
)
