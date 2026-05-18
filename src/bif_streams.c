#define _XOPEN_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <unistd.h>

#include <spawn.h>
#include <sys/wait.h>

#ifndef USE_MMAP
#define USE_MMAP 1
#endif
#if USE_MMAP
#include <sys/mman.h>
#endif

#include "history.h"
#include "module.h"
#include "network.h"
#include "parser.h"
#include "prolog.h"
#include "query.h"

#define MAX_ARGS 128
#define PROMPT ""

#define NEWLINE_MODE "posix"

// FIXME: this is too slow. There should be one overall
// alias map, not one per stream.

int get_named_stream(prolog *pl, const char *name, size_t len)
{
    prolog_lock(pl);

    for (int i = 0; i < MAX_STREAMS; i++) {
        stream *str = &pl->streams[i];

        if (!str->fp || str->ignore || !str->alias)
            continue;

        if (sl_get(str->alias, name, NULL)) {
            prolog_unlock(pl);
            return i;
        }

        if (str->filename && (strlen(str->filename) == len) && !strncmp(str->filename, name, len)) {
            prolog_unlock(pl);
            return i;
        }
    }

    prolog_unlock(pl);
    return -1;
}

int get_stream(query *q, cell *p1)
{
    if (is_atom(p1)) {
        int n = get_named_stream(q->pl, C_STR(q, p1), C_STRLEN(q, p1));

        if (n < 0)
            return -1;

        return n;
    }

    if (p1->tag != TAG_INT)
        return -1;

    if (!(p1->flags & FLAG_INT_STREAM))
        return -1;

    int n = get_smallint(p1);

    if (!q->pl->streams[n].fp)
        return -1;

    return n;
}

int new_stream(prolog *pl)
{
    prolog_lock(pl);

    for (int i = 0; i < MAX_STREAMS; i++) {
        unsigned n = pl->str_cnt++ % MAX_STREAMS;

        if (n < 4)
            continue;

        stream *str = &pl->streams[n];

        if (!str->fp && !str->ignore) {
            str->n = n;
            prolog_unlock(pl);
            return n;
        }
    }

    prolog_unlock(pl);
    return -1;
}

bool is_closed_stream(prolog *pl, cell *p1)
{
    if (!(p1->flags & FLAG_INT_STREAM))
        return false;

    if (pl->streams[get_smallint(p1)].fp)
        return false;

    return true;
}


static bool del_stream_properties(query *q, int n)
{
    cell *tmp = alloc_heap(q, 3);
    checked(tmp);
    make_atom(tmp + 0, g_sys_stream_property_s);
    make_int(tmp + 1, n);
    int vnbr = create_vars(q, 1);
    checked(vnbr != -1);
    make_ref(tmp + 2, vnbr, q->st.cur_ctx);
    tmp->num_cells = 3;
    tmp->arity = 2;
    q->retry = QUERY_OK;

    while (do_retract(q, tmp, q->st.cur_ctx, DO_RETRACTALL)) {
        if (q->did_throw)
            return false;
        q->retry = QUERY_RETRY;
        retry_choice(q);
    }

    q->retry = QUERY_OK;
    return true;
}

void convert_path(char *filename)
{
    char *src = filename;

    while (*src) {
        if (*src == '\\')
            *src = '/';

        src++;
    }
}

bool valid_list(query *q, cell *c, pl_ctx c_ctx)
{
    while (is_iso_list(c)) {
        c = c + 1;
        c += c->num_cells;
        c = deref(q, c, c_ctx);
        c_ctx = q->latest_ctx;

        if (!is_iso_list_or_nil_or_var(c))
            return false;
    }

    return true;
}

bool stream_close(query *q, int n)
{
    stream *str = &q->pl->streams[n];
    parser_destroy(str->p);
    str->p = NULL;

    if ((str->fp == stdin) || (str->fp == stdout) || (str->fp == stderr))
        return true;

    if ((int)q->pl->current_input == n)
        q->pl->current_input = 0;

    if ((int)q->pl->current_output == n)
        q->pl->current_output = 1;

    if ((int)q->pl->current_error == n)
        q->pl->current_error = 2;

    if (sl_get(str->alias, "user_input", NULL)) {
        stream *str2 = &q->pl->streams[0];
        sl_app(str2->alias, strdup("user_input"), NULL);
    }

    if (sl_get(str->alias, "user_output", NULL)) {
        stream *str2 = &q->pl->streams[1];
        sl_app(str2->alias, strdup("user_output"), NULL);
    }

    if (sl_get(str->alias, "user_error", NULL)) {
        stream *str2 = &q->pl->streams[2];
        sl_app(str2->alias, strdup("user_error"), NULL);
    }

    if (!str->socket && !str->is_mutex && !str->is_queue)
        del_stream_properties(q, n);

    bool ok = true;

    if (str->is_alias) {
    } else if (str->is_map) {
        sl_destroy(str->keyval);
    } else if (str->is_engine) {
        query_destroy(str->engine);
    } else if (str->is_queue || str->is_mutex) {
    } else
        ok = !net_close(str);

    sl_destroy(str->alias);
    str->alias = NULL;
    str->fp = NULL;
    // str->alias = sl_create((void*)fake_strcmp, (void*)keyfree, NULL);
    free(str->mode);
    str->mode = NULL;
    free(str->filename);
    str->filename = NULL;
    free(str->data);
    str->data = NULL;
    str->at_end_of_file = true;

    if (!ok)
        return throw_error(q, q->st.instr, q->st.cur_ctx, "io_error", strerror(errno));

    return true;
}

bool bif_iso_close_1(query *q)
{
    GET_FIRST_ARG(pstr, stream);
    int n = get_stream(q, pstr);
    return stream_close(q, n);
}

static bool parse_read_params(query *q, stream *str, cell *c, pl_ctx c_ctx, cell **vars,
                              pl_ctx *vars_ctx, cell **varnames, pl_ctx *varnames_ctx, cell **sings,
                              pl_ctx *sings_ctx)
{
    parser *p = str->p;

    if (!is_compound(c)) {
        throw_error(q, c, c_ctx, "domain_error", "read_option");
        return false;
    }

    cell *c1 = deref(q, FIRST_ARG(c), c_ctx);
    pl_ctx c1_ctx = q->latest_ctx;

    if (!CMP_STRING_TO_CSTR(q, c, "character_escapes")) {
        if (is_interned(c1))
            p->flags.character_escapes = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "json")) {
        if (is_interned(c1))
            p->flags.json = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "var_prefix")) {
        if (is_interned(c1))
            p->flags.var_prefix = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "double_quotes")) {
        if (is_interned(c1)) {
            if (!CMP_STRING_TO_CSTR(q, c1, "atom")) {
                p->flags.double_quote_codes = p->flags.double_quote_chars = false;
                p->flags.double_quote_atom = true;
            } else if (!CMP_STRING_TO_CSTR(q, c1, "chars")) {
                p->flags.double_quote_atom = p->flags.double_quote_codes = false;
                p->flags.double_quote_chars = true;
            } else if (!CMP_STRING_TO_CSTR(q, c1, "codes")) {
                p->flags.double_quote_atom = p->flags.double_quote_chars = false;
                p->flags.double_quote_codes = true;
            }
        }
    } else if (!CMP_STRING_TO_CSTR(q, c, "variables")) {
        if ((is_iso_list_or_nil_or_var(c1) && valid_list(q, c1, c1_ctx)) || true) {
            if (vars)
                *vars = c1;
            if (vars_ctx)
                *vars_ctx = c1_ctx;
        } else {
            throw_error(q, c, c_ctx, "domain_error", "read_option");
            return false;
        }
    } else if (!CMP_STRING_TO_CSTR(q, c, "variable_names")) {
        if ((is_iso_list_or_nil_or_var(c1) && valid_list(q, c1, c1_ctx)) || true) {
            if (varnames)
                *varnames = c1;
            if (varnames_ctx)
                *varnames_ctx = c1_ctx;
        } else {
            throw_error(q, c, c_ctx, "domain_error", "read_option");
            return false;
        }
    } else if (!CMP_STRING_TO_CSTR(q, c, "singletons")) {
        if ((is_iso_list_or_nil_or_var(c1) && valid_list(q, c1, c1_ctx)) || true) {
            if (sings)
                *sings = c1;
            if (sings_ctx)
                *sings_ctx = c1_ctx;
        } else {
            throw_error(q, c, c_ctx, "domain_error", "read_option");
            return false;
        }
    } else if (!CMP_STRING_TO_CSTR(q, c, "positions") && (c->arity == 2) && str->fp) {
        p->pos_start = ftello(str->fp);
    } else if (!CMP_STRING_TO_CSTR(q, c, "line_counts") && (c->arity == 2)) {
    } else {
        throw_error(q, c, c_ctx, "domain_error", "read_option");
        return false;
    }

    return true;
}

bool do_read_term(query *q, stream *str, cell *p1, pl_ctx p1_ctx, cell *p2, pl_ctx p2_ctx,
                  char *src)
{
    if (!str->p) {
        str->p = parser_create(q->st.m);
        checked(str->p);
        str->p->flags = q->st.m->flags;
        str->p->fp = str->fp;
        if (q->top)
            str->p->no_fp = q->top->no_fp;
    } else
        parser_reset(str->p);

    str->p->do_read_term = true;
    str->p->one_shot = true;
    cell *vars = NULL, *varnames = NULL, *sings = NULL;
    pl_ctx vars_ctx = 0, varnames_ctx = 0, sings_ctx = 0;
    cell *p21 = p2;
    pl_ctx p21_ctx = p2_ctx;

    LIST_HANDLER(p21);

    while (is_list(p21)) {
        cell *h = LIST_HEAD(p21);
        h = deref(q, h, p21_ctx);
        pl_ctx h_ctx = q->latest_ctx;

        if (is_var(h))
            return throw_error(q, p2, p2_ctx, "instantiation_error", "read_option");

        if (!parse_read_params(q, str, h, h_ctx, &vars, &vars_ctx, &varnames, &varnames_ctx, &sings,
                               &sings_ctx))
            return true;

        p21 = LIST_TAIL(p21);
        p21 = deref(q, p21, p21_ctx);
        p21_ctx = q->latest_ctx;
    }

    if (is_var(p21))
        return throw_error(q, p2, p2_ctx, "instantiation_error", "read_option");

    if (!is_nil(p21))
        return throw_error(q, p2, p2_ctx, "type_error", "list");

    if (!src && !str->p->srcptr && str->fp) {
        if (str->p->no_fp || getline(&str->p->save_line, &str->p->n_line, str->fp) == -1) {
            if (q->is_task && !feof(str->fp) && ferror(str->fp)) {
                clearerr(str->fp);
                return do_yield(q, 1);
            }

            str->p->srcptr = "";
        } else
            str->p->srcptr = str->p->save_line;
    }

    if (str->p->srcptr) {
        char *src = (char *)eat_space(str->p);

        if (str->p->error)
            return throw_error(q, q->st.instr, q->st.cur_ctx, "syntax_error",
                               str->p->error_desc ? str->p->error_desc : "read_term");

        str->p->line_num_start = str->p->line_num;
        str->p->srcptr = src;
    }

    for (;;) {

        if (!src && (!str->p->srcptr || !*str->p->srcptr || (*str->p->srcptr == '\n'))) {
            if (str->p->srcptr && (*str->p->srcptr == '\n'))
                str->p->line_num++;

            if (str->fp &&
                (str->p->no_fp || getline(&str->p->save_line, &str->p->n_line, str->fp) == -1)) {
                if (q->is_task && !feof(str->fp) && ferror(str->fp)) {
                    clearerr(str->fp);
                    return do_yield(q, 1);
                }

                str->p->srcptr = "";
                str->at_end_of_file = str->eof_action != eof_action_reset;

                if (str->eof_action == eof_action_reset)
                    clearerr(str->fp);

                if (vars)
                    if (!unify(q, vars, vars_ctx, make_nil(), q->st.cur_ctx))
                        return false;

                if (varnames)
                    if (!unify(q, varnames, varnames_ctx, make_nil(), q->st.cur_ctx))
                        return false;

                if (sings)
                    if (!unify(q, sings, sings_ctx, make_nil(), q->st.cur_ctx))
                        return false;

                cell *p22 = p2;
                pl_ctx p22_ctx = p2_ctx;
                LIST_HANDLER(p22);

                while (is_list(p22)) {
                    cell *h = LIST_HEAD(p22);
                    h = deref(q, h, p22_ctx);
                    pl_ctx h_ctx = q->latest_ctx;

                    if (is_var(h))
                        return throw_error(q, p2, p2_ctx, "instantiation_error", "read_option");

                    if (!CMP_STRING_TO_CSTR(q, h, "positions") && (h->arity == 2)) {
                        cell *p = h + 1;
                        p = deref(q, p, h_ctx);
                        pl_ctx p_ctx = q->latest_ctx;
                        cell tmp;
                        make_int(&tmp, str->p->pos_start);
                        unify(q, p, p_ctx, &tmp, q->st.cur_ctx);
                        p = h + 2;
                        p = deref(q, p, h_ctx);
                        p_ctx = q->latest_ctx;
                        make_int(&tmp, ftello(str->fp));
                        unify(q, p, p_ctx, &tmp, q->st.cur_ctx);
                    } else if (!CMP_STRING_TO_CSTR(q, h, "line_counts") && (h->arity == 2)) {
                        cell *p = h + 1;
                        p = deref(q, p, h_ctx);
                        pl_ctx p_ctx = q->latest_ctx;
                        cell tmp;
                        make_int(&tmp, str->p->line_num_start);
                        unify(q, p, p_ctx, &tmp, q->st.cur_ctx);
                        p = h + 2;
                        p = deref(q, p, h_ctx);
                        p_ctx = q->latest_ctx;
                        make_int(&tmp, str->p->line_num);
                        unify(q, p, p_ctx, &tmp, q->st.cur_ctx);
                    }

                    p22 = LIST_TAIL(p22);
                    p22 = deref(q, p22, p22_ctx);
                    p22_ctx = q->latest_ctx;
                }

                cell tmp;
                make_atom(&tmp, g_eof_s);
                return unify(q, p1, p1_ctx, &tmp, q->st.cur_ctx);
            }

            // if (!*p->save_line || (*p->save_line == '\r') || (*p->save_line == '\n'))
            //	continue;

            str->p->srcptr = str->p->save_line;
        } else if (src)
            str->p->srcptr = src;

        break;
    }

    if (str->p->did_getline)
        q->is_input = true;

    frame *f = GET_CURR_FRAME();
    str->p->read_term_slots = f->actual_slots;
    tokenize(str->p, false, false);
    str->p->read_term_slots = 0;

    if (str->p->error || !str->p->end_of_term) {
        str->p->error = false;

        if (!str->p->fp || !isatty(fileno(str->p->fp))) {
            void *save_fp = str->p->fp;
            str->p->fp = NULL;

            while (get_token(str->p, false, false) && SB_strlen(str->p->token) &&
                   SB_strcmp(str->p->token, ".")) {
            }

            str->p->fp = save_fp;
            str->p->did_getline = false;
        }

        str->p->do_read_term = false;
        return throw_error(q, make_nil(), q->st.cur_ctx, "syntax_error",
                           str->p->error_desc ? str->p->error_desc : "read_term");
    }

    str->p->do_read_term = false;

    cell *p22 = p2;
    pl_ctx p22_ctx = p2_ctx;
    LIST_HANDLER(p22);

    while (is_list(p22)) {
        cell *h = LIST_HEAD(p22);
        h = deref(q, h, p22_ctx);
        pl_ctx h_ctx = q->latest_ctx;

        if (is_var(h))
            return throw_error(q, p2, p2_ctx, "instantiation_error", "read_option");

        if (!CMP_STRING_TO_CSTR(q, h, "positions") && (h->arity == 2)) {
            cell *p = h + 1;
            p = deref(q, p, h_ctx);
            pl_ctx p_ctx = q->latest_ctx;
            cell tmp;
            make_int(&tmp, str->p->pos_start);

            if (!unify(q, p, p_ctx, &tmp, q->st.cur_ctx))
                return false;

            p = h + 2;
            p = deref(q, p, h_ctx);
            p_ctx = q->latest_ctx;
            make_int(&tmp, ftello(str->fp));

            if (!unify(q, p, p_ctx, &tmp, q->st.cur_ctx))
                return false;
        } else if (!CMP_STRING_TO_CSTR(q, h, "line_counts") && (h->arity == 2)) {
            cell *p = h + 1;
            p = deref(q, p, h_ctx);
            pl_ctx p_ctx = q->latest_ctx;
            cell tmp;
            make_int(&tmp, str->p->line_num_start);

            if (!unify(q, p, p_ctx, &tmp, q->st.cur_ctx))
                return false;

            p = h + 2;
            p = deref(q, p, h_ctx);
            p_ctx = q->latest_ctx;
            make_int(&tmp, str->p->line_num);

            if (!unify(q, p, p_ctx, &tmp, q->st.cur_ctx))
                return false;
        }

        p22 = LIST_TAIL(p22);
        p22 = deref(q, p22, p22_ctx);
        p22_ctx = q->latest_ctx;
    }

    if (!str->p->cl->cidx) {
        cell tmp;
        make_atom(&tmp, g_eof_s);
        return unify(q, p1, p1_ctx, &tmp, q->st.cur_ctx);
    }

    process_clause(str->p->m, str->p->cl, NULL);

    if (str->p->num_vars) {
        if (create_vars(q, str->p->num_vars) < 0)
            return throw_error(q, p1, p1_ctx, "resource_error", "stack");
    }

    q->tab_idx = 0;

    if (str->p->num_vars)
        collect_vars(q, str->p->cl->cells, q->st.cur_ctx);

    if (vars) {
        unsigned cnt = q->tab_idx;
        unsigned idx = 0;

        if (cnt) {
            checked(init_tmp_heap(q));
            cell *tmp = alloc_tmp(q, (cnt * 2) + 1);
            checked(tmp);
            unsigned done = 0;

            for (unsigned i = 0; i < q->tab_idx; i++) {
                make_atom(tmp + idx, g_dot_s);
                tmp[idx].arity = 2;
                tmp[idx++].num_cells = ((cnt - done) * 2) + 1;
                cell v;
                make_ref(&v, q->pl->tabs[i].var_num, q->st.cur_ctx);
                tmp[idx++] = v;
                done++;
            }

            make_atom(tmp + idx++, g_nil_s);
            tmp[0].arity = 2;
            tmp[0].num_cells = idx;

            cell *save = tmp;
            tmp = alloc_heap(q, idx);
            checked(tmp);
            dup_cells(tmp, save, idx);
            tmp->num_cells = idx;
            if (!unify(q, vars, vars_ctx, tmp, q->st.cur_ctx))
                return false;
        } else {
            if (!unify(q, vars, vars_ctx, make_nil(), q->st.cur_ctx))
                return false;
        }
    }

    if (varnames) {
        unsigned cnt = 0;
        checked(init_tmp_heap(q));
        unsigned idx = 0;

        for (unsigned i = 0; i < q->tab_idx; i++) {
            if (q->pl->tabs[i].is_anon)
                continue;

            cnt++;
        }

        if (cnt) {
            cell *tmp = alloc_tmp(q, (cnt * 4) + 1);
            checked(tmp);
            unsigned done = 0;

            for (unsigned i = 0; i < q->tab_idx; i++) {
                if (q->pl->tabs[i].is_anon)
                    continue;

                make_atom(tmp + idx, g_dot_s);
                tmp[idx].arity = 2;
                tmp[idx++].num_cells = ((cnt - done) * 4) + 1;
                cell v;
                make_instr(&v, g_unify_s, bif_iso_unify_2, 2, 2);
                SET_OP(&v, OP_XFX);
                tmp[idx++] = v;
                make_atom(&v, q->pl->tabs[i].val_off);
                tmp[idx++] = v;
                make_ref(&v, q->pl->tabs[i].var_num, q->st.cur_ctx);
                tmp[idx++] = v;
                done++;
            }

            make_atom(tmp + idx++, g_nil_s);
            tmp[0].arity = 2;
            tmp[0].num_cells = idx;

            cell *save = tmp;
            tmp = alloc_heap(q, idx);
            checked(tmp);
            dup_cells(tmp, save, idx);
            tmp->num_cells = idx;
            if (!unify(q, varnames, varnames_ctx, tmp, q->st.cur_ctx))
                return false;
        } else {
            if (!unify(q, varnames, varnames_ctx, make_nil(), q->st.cur_ctx))
                return false;
        }
    }

    if (sings) {
        unsigned cnt = 0;
        checked(init_tmp_heap(q));
        unsigned idx = 0;

        for (unsigned i = 0; i < q->tab_idx; i++) {
            if (q->pl->tabs[i].cnt != 1)
                continue;

            if (varnames && (q->pl->tabs[i].is_anon))
                continue;

            cnt++;
        }

        if (cnt) {
            cell *tmp = alloc_tmp(q, (cnt * 4) + 1);
            checked(tmp);
            unsigned done = 0;

            for (unsigned i = 0; i < q->tab_idx; i++) {
                if (q->pl->tabs[i].cnt != 1)
                    continue;

                if (varnames && (q->pl->tabs[i].is_anon))
                    continue;

                make_atom(tmp + idx, g_dot_s);
                tmp[idx].arity = 2;
                tmp[idx++].num_cells = ((cnt - done) * 4) + 1;
                cell v;
                make_instr(&v, g_unify_s, bif_iso_unify_2, 2, 2);
                SET_OP(&v, OP_XFX);
                tmp[idx++] = v;
                make_atom(&v, q->pl->tabs[i].val_off);
                tmp[idx++] = v;
                make_ref(&v, q->pl->tabs[i].var_num, q->st.cur_ctx);
                tmp[idx++] = v;
                done++;
            }

            make_atom(tmp + idx++, g_nil_s);
            tmp[0].arity = 2;
            tmp[0].num_cells = idx;

            cell *save = tmp;
            tmp = alloc_heap(q, idx);
            checked(tmp);
            dup_cells(tmp, save, idx);
            tmp->num_cells = idx;
            if (!unify(q, sings, sings_ctx, tmp, q->st.cur_ctx))
                return false;
        } else {
            if (!unify(q, sings, sings_ctx, make_nil(), q->st.cur_ctx))
                return false;
        }
    }

    cell *tmp = alloc_heap(q, str->p->cl->cidx - 1);
    checked(tmp);
    dup_cells(tmp, str->p->cl->cells, str->p->cl->cidx - 1);
    bool ok = unify(q, p1, p1_ctx, tmp, q->st.cur_ctx);
    clear_clause(str->p->cl);
    return ok;
}

bool parse_write_params(query *q, cell *c, pl_ctx c_ctx, cell **vnames, pl_ctx *vnames_ctx)
{
    if (is_var(c)) {
        throw_error(q, c, c_ctx, "instantiation_error", "write_option");
        return false;
    }

    if (!is_interned(c) || !is_compound(c)) {
        throw_error(q, c, c_ctx, "domain_error", "write_option");
        return false;
    }

    cell *c1 = deref(q, FIRST_ARG(c), c_ctx);
    pl_ctx c1_ctx = q->latest_ctx;

    if (!CMP_STRING_TO_CSTR(q, c, "max_depth")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (is_integer(c1) && (get_smallint(c1) >= 0))
            q->max_depth = get_smallint(c1);
        else {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }
    } else if (!CMP_STRING_TO_CSTR(q, c, "fullstop")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_interned(c1) ||
            (CMP_STRING_TO_CSTR(q, c1, "true") && CMP_STRING_TO_CSTR(q, c1, "false"))) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        q->fullstop = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "portrayed")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_interned(c1) ||
            (CMP_STRING_TO_CSTR(q, c1, "true") && CMP_STRING_TO_CSTR(q, c1, "false"))) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        q->portrayed = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "nl")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_interned(c1) ||
            (CMP_STRING_TO_CSTR(q, c1, "true") && CMP_STRING_TO_CSTR(q, c1, "false"))) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        q->nl = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "json")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_interned(c1) ||
            (CMP_STRING_TO_CSTR(q, c1, "true") && CMP_STRING_TO_CSTR(q, c1, "false"))) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        q->json = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "quoted")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_interned(c1) ||
            (CMP_STRING_TO_CSTR(q, c1, "true") && CMP_STRING_TO_CSTR(q, c1, "false"))) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        q->quoted = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "double_quotes")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_interned(c1) ||
            (CMP_STRING_TO_CSTR(q, c1, "true") && CMP_STRING_TO_CSTR(q, c1, "false"))) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        q->double_quotes = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "varnames")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_interned(c1) ||
            (CMP_STRING_TO_CSTR(q, c1, "true") && CMP_STRING_TO_CSTR(q, c1, "false"))) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        q->varnames = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "ignore_ops")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_interned(c1) ||
            (CMP_STRING_TO_CSTR(q, c1, "true") && CMP_STRING_TO_CSTR(q, c1, "false"))) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        q->ignore_ops = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "numbervars")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_interned(c1) ||
            (CMP_STRING_TO_CSTR(q, c1, "true") && CMP_STRING_TO_CSTR(q, c1, "false"))) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        q->numbervars = !CMP_STRING_TO_CSTR(q, c1, "true");
    } else if (!CMP_STRING_TO_CSTR(q, c, "variable_names")) {
        if (is_var(c1)) {
            throw_error(q, c1, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_list_or_nil(c1)) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        cell *c1_orig = c1;
        pl_ctx c1_orig_ctx = c1_ctx;
        bool any1 = false, any2 = false;
        LIST_HANDLER(c1);

        while (is_list(c1)) {
            cell *h = LIST_HEAD(c1);
            pl_ctx h_ctx = c1_ctx;

            slot *e = NULL;
            uint32_t save_vgen = 0;
            int both = 0;
            DEREF_VAR(any1, both, save_vgen, e, e->vgen, h, h_ctx, q->vgen);

            if (is_var(h)) {
                throw_error(q, h, h_ctx, "instantiation_error", "write_option");
                return false;
            }

            if (!is_compound(h)) {
                throw_error(q, c, c_ctx, "domain_error", "write_option");
                return false;
            }

            if (CMP_STRING_TO_CSTR(q, h, "=")) {
                throw_error(q, c, c_ctx, "domain_error", "write_option");
                return false;
            }

            if (is_interned(h)) {
                cell *h1 = deref(q, h + 1, h_ctx);

                if (is_var(h1)) {
                    throw_error(q, c, c_ctx, "instantiation_error", "write_option");
                    return false;
                } else if (!is_atom(h1)) {
                    throw_error(q, c, c_ctx, "domain_error", "write_option");
                    return false;
                }
            }

            c1 = LIST_TAIL(c1);

            both = 0;
            DEREF_VAR(any2, both, save_vgen, e, e->vgen, c1, c1_ctx, q->vgen);

            if (both) {
                throw_error(q, c, c_ctx, "domain_error", "write_option");
                return false;
            }
        }

        if (is_var(c1)) {
            throw_error(q, c1_orig, c_ctx, "instantiation_error", "write_option");
            return false;
        }

        if (!is_nil(c1)) {
            throw_error(q, c, c_ctx, "domain_error", "write_option");
            return false;
        }

        if (vnames)
            *vnames = c1_orig;
        if (vnames_ctx)
            *vnames_ctx = c1_orig_ctx;
    } else {
        throw_error(q, c, c_ctx, "domain_error", "write_option");
        return false;
    }

    return true;
}

bool do_load_file(query *q, cell *p1, pl_ctx p1_ctx)
{
    if (is_atom(p1)) {
        char *src = DUP_STRING(q, p1);
        char *filename = relative_to(q->st.m->filename, src);
        convert_path(filename);
        unload_file(q->st.m, filename);
        free(src);

        if (!load_file(q->st.m, filename, false, true)) {
            free(filename);
            return throw_error(q, p1, p1_ctx, "existence_error", "source_sink");
        }

        free(filename);
        return true;
    }

    if (!is_compound(p1))
        return throw_error(q, p1, p1_ctx, "type_error", "atom");

    if (CMP_STRING_TO_CSTR(q, p1, ":"))
        return throw_error(q, p1, p1_ctx, "type_error", "atom");

    cell *mod = deref(q, p1 + 1, p1_ctx);
    cell *file = deref(q, p1 + 2, p1_ctx);

    if (!is_atom(mod) || !is_atom(file))
        return throw_error(q, p1, p1_ctx, "type_error", "atom");

    module *tmp_m = module_create(q->pl, C_STR(q, mod));
    char *src = DUP_STRING(q, file);
    char *filename = relative_to(q->st.m->filename, src);
    free(src);
    convert_path(filename);
    unload_file(tmp_m, filename);

    if (!load_file(tmp_m, filename, false, true)) {
        module_destroy(tmp_m);
        free(filename);
        return throw_error(q, p1, p1_ctx, "existence_error", "source_sink");
    }

    free(filename);
    return true;
}

