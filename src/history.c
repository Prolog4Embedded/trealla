#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#if defined(USE_ISOCLINE)
#include "isocline/include/isocline.h"
#endif

#if defined(USE_EDITLINE)
#include <histedit.h>
#include <editline/readline.h>
#endif

#if !defined(USE_ISOCLINE) && !defined(USE_EDITLINE)
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <termios.h>

#include "internal.h"
#include "history.h"
#include "prolog.h"

#include "platform/linux/panic.h"

int history_getch_fd(int fd)
{
	not_implemented(__func__);
}

int history_getch(void)
{
	return history_getch_fd(STDIN_FILENO);
}

static char g_filename[1024];


#if !USE_ISOCLINE
char *history_readline_eol(prolog *pl, const char *prompt, char eol)
{
	not_implemented(__func__);
}

static char *functor_name_generator(const char *text, int state)
{
	not_implemented(__func__);
}

static char **functor_name_completion(const char *text, int start, int end)
{
	not_implemented(__func__);
}

void history_load(const char *filename)
{
	not_implemented(__func__);
}

void history_save(void)
{
	not_implemented(__func__);
}
#endif

#if USE_ISOCLINE
char *history_readline_eol(prolog *pl, const char *prompt, char eol)
{
	not_implemented(__func__);
}

void history_load(const char *filename)
{
	not_implemented(__func__);
}

void history_save(void)
{
	not_implemented(__func__);
}
#endif
