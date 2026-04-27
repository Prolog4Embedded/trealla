__flash         = 0x08000000;
__flash_size    = 1M;

__ram           = 0x20000000;
__ram_size      = 128K;

__stack_size    = 16K;
__heap_size_min = 64K;

INCLUDE picolibc.ld
