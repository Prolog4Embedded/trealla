tpl_board(
  NAME                "mps3-an524"
  DESCRIPTION         "ARM Cortex-M33, QEMU mps3-an524, picolibc + semihosting"
  LINKER_SCRIPT       "${CMAKE_CURRENT_LIST_DIR}/linker_script.ld"
  PICOLIBC_CROSS_FILE "${CMAKE_CURRENT_LIST_DIR}/picolibc-cross.txt"
  USE_SEMIHOST        ON
  SOURCES             "${CMAKE_CURRENT_LIST_DIR}/time.c"
)
