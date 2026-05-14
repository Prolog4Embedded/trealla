#include <stdio.h>
void board_init() {
  //
}


void serial_putc(char c) {
	putc(c, stdout);
}

int serial_getc(void) {
	return getc(stdin);
}
