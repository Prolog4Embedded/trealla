#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "module.h"
#include "parser.h"
#include "prolog.h"
#include "query.h"


static void db_log(query *q, rule *r, enum log_type l)
{
    FILE *fp = q->pl->logfp;

    if (!fp)
        return;

    char tmpbuf[256];
    char *dst;
    q->quoted = 2;

    switch (l) {
    case LOG_ASSERTA:
        dst = print_term_to_strbuf(q, r->cl.cells, q->st.cur_ctx, 1);
        uuid_to_buf(&r->u, tmpbuf, sizeof(tmpbuf));
        fprintf(fp, "%s:'$a_'((%s),'%s').\n", q->st.m->name, dst, tmpbuf);
        free(dst);
        break;
    case LOG_ASSERTZ:
        dst = print_term_to_strbuf(q, r->cl.cells, q->st.cur_ctx, 1);
        uuid_to_buf(&r->u, tmpbuf, sizeof(tmpbuf));
        fprintf(fp, "%s:'$z_'((%s),'%s').\n", q->st.m->name, dst, tmpbuf);
        free(dst);
        break;
    case LOG_ERASE:
        uuid_to_buf(&r->u, tmpbuf, sizeof(tmpbuf));
        fprintf(fp, "%s:'$e_'('%s').\n", q->st.m->name, tmpbuf);
        break;
    }

    q->quoted = 0;
}

static void predicate_purge_dirty_list(predicate *pr)
{
    unsigned cnt = 0;
    rule *r;

    while ((r = list_pop_front(&pr->dirty)) != NULL) {
        predicate_delink(pr, r);
        clear_clause(&r->cl);
        free(r);
        cnt++;
    }
}

bool do_retract(query *q, cell *p1, pl_ctx p1_ctx, enum clause_type is_retract)
{
    if (!q->retry) {
        cell *head = deref(q, get_head(p1), p1_ctx);

        if (is_var(head))
            return throw_error(q, head, q->latest_ctx, "instantiation_error",
                               "not_sufficiently_instantiated");

        if (!is_callable(head))
            return throw_error(q, head, q->latest_ctx, "type_error", "callable");
    }

    bool match;

    if (is_a_rule(p1) && get_logical_body(p1)) {
        match = match_rule(q, p1, p1_ctx, is_retract);
    } else {
        p1 = get_head(p1);
        match = match_clause(q, p1, p1_ctx, is_retract);
    }

    if (!match || q->did_throw)
        return match;

    rule *r = q->st.dbe;
    db_log(q, r, LOG_ERASE);
    retract_from_db(r->owner->m, r);
    bool last_match = (is_retract == DO_RETRACT) && !has_next_key(q);
    stash_frame(q, &r->cl, last_match);
    return true;
}

bool do_abolish(query *q, cell *c_orig, cell *c_pi, bool hard)
{
    predicate *pr = search_predicate(q->st.m, c_pi, NULL);
    if (!pr)
        return true;

    if (!pr->is_dynamic)
        return throw_error(q, c_orig, q->st.cur_ctx, "permission_error", "modify,static_procedure");

    for (rule *r = pr->head; r; r = r->next)
        retract_from_db(r->owner->m, r);

    if (!pr->refcnt) {
        predicate_purge_dirty_list(pr);
    } else {
        rule *r;

        while ((r = list_pop_front(&pr->dirty)) != NULL)
            list_push_back(&q->dirty, r);
    }

    sl_destroy(pr->idx2);
    sl_destroy(pr->idx1);
    pr->idx1 = pr->idx2 = NULL;
    pr->is_processed = false;
    pr->head = pr->tail = NULL;
    pr->cnt = 0;

    if (hard)
        pr->is_abolished = true;

    return true;
}

bool do_erase(module *m, const char *str)
{
    uuid u;
    uuid_from_buf(str, &u);
    erase_from_db(m, &u);
    return true;
}

