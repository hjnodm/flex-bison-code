extern int yylineno;
void yyerror(char *s, ...);

#define NHASH 9997

struct pcdata{
	scan_t scaninfo;
	struct stmbol *symtab;
	struct ast *ast;

};

enum bifs {
    B_sqrt = 1,
    B_exp,
    B_log,
    B_print,
};

struct ast {
    int nodetype;
    struct ast *l;
    struct ast *r;
};

/* symbol */
struct symbol {
    char *name;
    double value;
    struct ast *func;
    struct symlist *syms; 
};

struct fncall {
    int nodetype;
    struct ast *l;
    struct symbol *s;
    int functype;
};

struct ufncall {
    int nodetype;
    struct ast *l;
    struct symbol *s;
};

struct symlist {
    struct symbol *sym;
    struct symlist *next;
};

struct flow {
    int nodetype;
    struct ast *cond;
    struct ast *tl;
    struct ast *el;
};

struct numval {
    int nodetype;
    double number;
};

struct symref {
    int nodetype;
    struct symbol *s;
};

struct symasgn {
    int nodetype;
    struct symbol *s;
    struct ast *v;
};

struct symbol symtab[NHASH];
struct symbol *lookup(struct pcdata *, char *name);


struct symlist *newsymlist(struct pcdata *, struct symbol *sym, struct symlist *next);
void symlistfree(struct pcdata *, struct symlist *sl);

struct ast *newast(struct pcdata *, int nodetype, struct ast *l, struct ast *r);
struct ast *newcmp(struct pcdata *, int cmptype, struct ast *l, struct ast  *r);
struct ast *newfunc(struct pcdata *, int functype, struct ast *l);
struct ast *newcall(struct pcdata *, struct symbol *s, struct ast *l);
struct ast *newref(struct pcdata *, struct symbol *s);
struct ast *newasgn(struct pcdata *, struct symbol *s, struct ast *v);
struct ast *newnum(struct pcdata *, double d);
struct ast *newflow(struct pcdata *, int nodetype, struct ast *cond, struct ast *tl, struct ast *tr);

void dodef(struct pcdata *, struct symbol *name, struct symlist *syms, struct ast *stmts);

double eval(struct pcdata *, struct ast *node);

void treefree(struct pcdata *, struct ast *node);

void yyerror(struct pcdata *pp, char *s, ...);
