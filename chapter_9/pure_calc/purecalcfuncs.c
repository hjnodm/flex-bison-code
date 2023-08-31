#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "purecalc.tab.h"
#include "purecalc.lex.h"
#include "purecalc.h"

static unsigned symhash(char *sym)
{
    unsigned int hash = 0;
    char c;

    while(c = *sym++)
        hash = hash*9 ^c;
    
    return hash;
}

struct symbol *lookup(struct pcdata *pp, char *sym)
{
    struct symbol *sp = &(pp->stmtab)[symhash(sym)%NHASH];
    int scount = NHASH;

    while(--scount >= 0) {
        if (sp->name && !strcmp(sp->name, sym)) {
            return sp;
        }

        if (!sp->name) {
            sp->name = strdup(sym);
            sp->value = 0;
            sp->func = NULL;
            sp->syms = NULL;
            return sp;
        }

        if (++sp >= pp->stmtab + NHASH) sp = pp->stmtab;
    }

    yyerror(pp, "symbol table overflow\n");
    abort();
}

struct ast *newast(struct pcdata *pp, int nodetype, struct ast *l, struct ast *r)
{
    struct ast *a = NULL;

    a = malloc(sizeof(struct ast));
    if (!a) {
        yyerror(pp, "out of space\n");
        exit(-1);
    }
    a->nodetype = nodetype;
    a->l = l;
    a->r = r;
    return a;
}

struct ast *newnum(struct pcdata *pp, double d)
{
    struct numval *a = NULL;

    a = malloc(sizeof(struct numval));
    if (!a) {
        yyerror(pp,"out of space\n");
        exit(-1);
    }

    a->nodetype = 'K';
    a->number = d;

    return (struct ast *)a;
}

struct ast *newcmp(struct pcdata *pp, int cmptype, struct ast *l, struct ast *r)
{
    struct ast *a = NULL;

    a = malloc(sizeof(struct ast));
    if (!a) {
        yyerror(pp, "out of space\n");
        exit(-1);
    }

    a->nodetype = '0' + cmptype;
    a->l = l;
    a->r = r;

    return a;
}

struct ast *newfunc(struct pcdata *pp, int functype, struct ast *l)
{
    struct fncall *a = NULL;

    a = malloc(sizeof(struct fncall));
    if (!a) {
        yyerror(pp, "out of space\n");
        exit(-1);
    }

    a->nodetype = 'F';
    a->l = l;
    a->functype = functype;
    return (struct ast *)a;
}

struct ast *newcall(struct pcdata *pp, struct symbol *s, struct ast *l)
{
    struct ufncall *a = NULL;

    a = malloc(sizeof(struct ufncall));
    if (!a) {
        yyerror(pp, "out of space");
        exit(-1);
    }
    a->nodetype = 'C';
    a->l = l;
    a->s = s;

    return (struct ast *)a;
}

struct ast *newref(struct pcdata *pp, struct pcdata *pp, struct symbol *s)
{
    struct symref *a = NULL;

    a = malloc(sizeof(struct symref));
    if (!a) {
        yyerror(pp, "out of space");
        exit(-1);
    } 

    a->nodetype = 'N';
    a->s = s;
    return (struct ast *)a;
}

struct ast *newasgn(struct pcdata *pp, struct symbol *s, struct ast *v)
{
    struct symasgn *a = NULL;

    a = malloc(sizeof(struct symasgn));
    if (!a) {
        yyerror(pp, "out of space");
        exit(-1);
    }

    a->nodetype = '=';
    a->s = s;
    a->v = v;
    return (struct ast *)a;
}

struct ast *newflow(struct pcdata *pp, int nodetype, struct ast *cond, struct ast *tl, struct ast *el)
{
    struct flow *a = NULL;

    a = malloc(sizeof(struct flow));
    if (!a) {
        yyerror(pp, "out of space");
        exit(-1);
    }

    a->nodetype = nodetype;
    a->cond = cond;
    a->tl = tl;
    a->el = el;

    return (struct ast *)a;
}

void treefree(struct pcdata *pp, struct ast *a)
{
    switch(a->nodetype) {
        case '+': 
        case '-':
        case '*':
        case '/':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case 'L':
            treefree(pp, a->r);

        case '|':
        case 'M':
        case 'C':
        case 'F':
            treefree(pp, a->l);
        case 'K':
        case 'N': 
            break;
        case '=':
            free(((struct symasgn *)a)->v);
            break;
        case 'I': 
        case 'W':
            free(((struct flow *)a)->cond);
            if (((struct flow *)a)->tl)
                treefree(((struct flow *)a)->tl);
            if (((struct flow *)a)->el)
                treefree(((struct flow *)a)->el);
            break;
    default: 
            printf("internal error: free bad node %c\n", a->nodetype);
    }
    free(a);
}

struct symlist *newsymlist(struct pcdata *pp, struct symbol *sym, struct symlist *next)
{
    struct symlist *sl = NULL;

    sl = malloc(sizeof(struct symlist));
    if (!sl) {
        yyerror(pp, "out of space");
        exit(-1);
    }

    sl->sym = sym;
    sl->next = next;
    return sl;
}

void symlistfree(struct pcdata *pp, struct symlist *sl)
{
    struct symlist *nsl;

    while(sl) {
        nsl = sl->next;
        free(sl);
        sl = nsl;
    }
}

static double callbuiltin(struct pcdata *pp,struct fncall *tmp);
static double calluser(struct pcdata *pp,struct ufncall *tmp);

double eval(struct pcdata *pp, struct ast *a)
{
    double v;
    if(!a) {
        yyerror(pp, "internal error, null eval\n");
        return 0.0;
    }

    switch(a->nodetype) {
        case 'K': 
            v = ((struct numval *)a)->number;
            break;
        case 'N':
            v = ((struct symasgn *)a)->s->value;
            break;
        case '=': 
            v = ((struct symasgn *)a)->s->value = eval(((struct symasgn *)a)->v);
            break;
        case '+':
            v = eval(pp, a->l) + eval(pp, a->r);
            break;
        case '-':
            v = eval(pp, a->l) - eval(pp, a->r);
            break;
        case '*':
            v = eval(pp, a->l) * eval(pp, a->r);
            break;
        case '/':
            v = eval(pp, a->l) / eval(pp, a->r);
            break;
        case '|':
            v = fabs(eval(pp, a->l));
            break;
        case 'M':
            v = -eval(pp, a->l);
            break;
        case '1':
            v = (eval(pp, a->l) > eval(pp, a->r))? 1:0;
            break;
        case '2':
            v = (eval(pp, a->l) < eval(pp, a->r))? 1: 0;
            break;
        case '3':
            v = (eval(pp, a->l) != eval(pp, a->r))? 1:0;
            break;
        case '4': 
            v = (eval(pp, a->l) == eval(pp, a->r))? 1:0;
            break;
        case '5':
            v = (eval(pp, a->l) >= eval(pp, a->r))? 1:0;
            break;
        case '6':
            v = (eval(pp, a->l) <= eval(pp, a->r))? 1:0;
            break;
        case 'I':
            if (eval(pp, ((struct flow *)a)->cond) != 0) {
                if (((struct flow *)a)->tl) {
                    v = eval(pp, ((struct flow *)a)->tl);
                } else {
                    v = 0.0;
                }
            } else {
                if (((struct flow *)a)->el) {
                    v = eval(pp, ((struct flow *)a)->el);
                } else v = 0.0;
            }
            break;
        case 'W': 
            v = 0.0;
            if (((struct flow *)a)->tl) {
                while (eval(pp, ((struct flow *)a)->cond) != 0)
                    v = eval(pp, ((struct flow *)a)->tl);
            }
            break;
        case 'L': 
            eval(pp, a->l);
            v = eval(pp, a->r);
            break;
        case 'F': 
            v = callbuiltin(pp, (struct fncall *)a);
            break;
        case 'C': 
            v = calluser(pp, (struct ufncall *)a);
            break;
        default: printf("internal error: bad node %c\n", a->nodetype);
    }
    return v;
}

static double callbuiltin(struct pcdata *pp, struct fncall *f)
{
    int functype = f->functype;
    double v = eval(pp, f->l);

    switch(functype) {
        case B_sqrt:
            return sqrt(v);
        case B_exp:
            return exp(v);
        case B_log: 
            return log(v);
        case B_print: 
            printf("= %4.4g\n", v);
            return v;
        default:
            yyerror(pp, "unknown builtin function %d\n", functype);
            return 0.0;
    }
}

void dodef(struct pcdata *pp, struct symbol *name, struct symlist *syms, struct ast *func)
{
    if (name->syms) 
        symlistfree(pp, name->syms);

    if (name->func)
        treefree(pp, name->func);

    name->syms = syms;
    name->func = func;
}

static double calluser(struct pcdata *pp, struct ufncall *f)
{
    struct symbol *fn = f->s;
    struct symlist *sl;
    struct ast *args = f->l;
    double *oldval, *newval;
    double v;
    int nargs;
    int i;

    if (!fn->func) {
        yyerror(pp, "call to undefined function\n", fn->name);
        return 0;
    }

    sl = fn->syms;
    for (nargs = 0; sl; sl = sl->next)
        nargs ++;

    oldval = (double*) malloc(nargs * sizeof(double));
    newval = (double *)malloc(nargs * sizeof(double));
    if (!oldval||!newval) {
        yyerror(pp, "out of space in %s\n", fn->name);
        return 0.0;
    }

    for (i = 0; i < nargs; i ++) {
        if (!args) {
            yyerror(pp, "too few args in call to %s\n", fn->name);
            free(oldval);
            free(newval);
            return 0.0;
        }

        if (args->nodetype == 'L') {
            newval[i] = eval(pp, args->l);
            args = args->r;
        } else {
            newval[i] = eval(pp, args);
            args = NULL;
        }
    }

        sl = fn->syms;
        for (i = 0; i < nargs; i ++) {
            struct symbol *s = sl->sym;

            oldval[i] = s->value;
            s->value = newval[i];
            sl = sl->next;
        }

        free(newval);

        v = eval(pp, fn->func);
        sl = fn->syms;
        for (i = 0; i < nargs; i++) {
            struct symbol *s = sl->sym;
            s->value = oldval[i];
            sl = sl->next;
        }

        free(oldval);
        return v;
}

void yyerror(struct pcdata *pp,char *s, ...)
{
    va_list ap;
    va_start(ap, s);

    fprintf(stderr, "%d: error:", yyget_lineno(pp->scaninfo));
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
    struct pcdata p = {NULL, 0, NULL};
    if(yylen_init_extra(&p, &p.scaninfo)) {
    	perror("init alloc failed");
    	return 1;
    }
    
    if(!(p.symtab = calloc(NHASH, sizeof(struct symbol)))){
	perror("sym alloc failed");
    	return 1;
    }
    for(;;){
        printf("> ");
        yyparse(&p);
        if(p.ast){
        printf("= %4.4g\n", eval(&p, p.ast));
        treefree(&p, p.ast);
        p.ast = 0;
    }
}
