%{
/* Copyright (c) 2015 Nicolas Hake <nh@nosebud.de>
 *
 * Permission to use, copy, modify, and / or distribute this software
 * for any purpose with or without fee is hereby granted, provided that
 * the above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "gtrP.h"

#define YYMALLOC malloc
#define YYFREE free

%}

%no-lines
%require "2.4"
%name-prefix "_gtr_plural"
%define api.pure
%expect 14
%debug

%lex-param {struct _gtr_plural_parser *arg}
%parse-param {struct _gtr_plural_parser *arg}

%union {
	int n;
	libgtr_plural_expr_t *expr;
}
%destructor { libgtr_plural_expr_free($$); } <expr>

%printer { fprintf (yyoutput, "%d", $$); } <n>

%{
static int yylex(YYSTYPE *lval, struct _gtr_plural_parser *arg);
static void yyerror(struct _gtr_plural_parser *arg, const char *str);	
%}

%right PLP_TOK_QMARK /* ? : */
%left  PLP_TOK_OR /* || */
%left  PLP_TOK_AND /* && */
%left  PLP_TOK_EQ /* == */ PLP_TOK_NEQ /* != */
%left  PLP_TOK_LT /* < */ PLP_TOK_LTE /* <= */ PLP_TOK_GT /* > */ PLP_TOK_GTE /* >= */
%left  PLP_TOK_ADD /* + */ PLP_TOK_SUB /* - */
%left  PLP_TOK_MUL /* * */ PLP_TOK_DIV /* / */ PLP_TOK_MOD /* % */
%right PLP_TOK_NOT /* ! */

%token PLP_TOK_COLON PLP_TOK_OPAREN PLP_TOK_CPAREN
%token PLP_TOK_VAR
%token <n> PLP_TOK_NUM

%type <expr> expr
%{
static libgtr_plural_expr_t *expr(libgtr_plural_eval_operation op,
	int argc, libgtr_plural_expr_t **argv)
{
	/* Sanity-check all arguments. */
	for (int i = 0; i < argc; ++i)
	{
		if (argv[i] == NULL)
			return NULL;
	}

	libgtr_plural_expr_t *e = calloc(1, sizeof(libgtr_plural_expr_t));
	if (e == NULL)
	{
		return NULL;
	}

	e->op = op;
	for (int i = 0; i < argc; ++i)
	{
		e->args[i] = argv[i];
	}

	return e;
}
static libgtr_plural_expr_t *expr0(libgtr_plural_eval_operation op)
{
	return expr(op, 0, NULL);
}
static libgtr_plural_expr_t *expr1(libgtr_plural_eval_operation op,
	libgtr_plural_expr_t *arg0)
{
	return expr(op, 1, &arg0);
}
static libgtr_plural_expr_t *expr2(libgtr_plural_eval_operation op,
	libgtr_plural_expr_t *arg0, libgtr_plural_expr_t *arg1)
{
	libgtr_plural_expr_t *args[2] = { arg0, arg1 };
	return expr(op, 2, args);
}
static libgtr_plural_expr_t *expr3(libgtr_plural_eval_operation op,
	libgtr_plural_expr_t *arg0, libgtr_plural_expr_t *arg1,
	libgtr_plural_expr_t *arg2)
{
	libgtr_plural_expr_t *args[3] = { arg0, arg1, arg2 };
	return expr(op, 3, &arg0);
}
%}

%%

start:    expr
          {
          	if ($1 == NULL)
          	  YYABORT;
          	  arg->result = $1;
          }
        ;

expr:     expr PLP_TOK_QMARK expr PLP_TOK_COLON expr
          { $$ = expr3(GPEO_TERN, $1, $3, $5); }
        | expr PLP_TOK_OR expr
          { $$ = expr2(GPEO_OR, $1, $3); }
        | expr PLP_TOK_AND expr
          { $$ = expr2(GPEO_AND, $1, $3); }
        | expr PLP_TOK_EQ expr
          { $$ = expr2(GPEO_EQ, $1, $3); }
        | expr PLP_TOK_NEQ expr
          { $$ = expr2(GPEO_NEQ, $1, $3); }
        | expr PLP_TOK_LT expr
          { $$ = expr2(GPEO_LT, $1, $3); }
        | expr PLP_TOK_LTE expr
          { $$ = expr2(GPEO_LTE, $1, $3); }
        | expr PLP_TOK_GT expr
          { $$ = expr2(GPEO_GT, $1, $3); }
        | expr PLP_TOK_GTE expr
          { $$ = expr2(GPEO_GTE, $1, $3); }
        | expr PLP_TOK_ADD expr
          { $$ = expr2(GPEO_ADD, $1, $3); }
        | expr PLP_TOK_SUB expr
          { $$ = expr2(GPEO_SUB, $1, $3); }
        | expr PLP_TOK_MUL expr
          { $$ = expr2(GPEO_MUL, $1, $3); }
        | expr PLP_TOK_DIV expr
          { $$ = expr2(GPEO_DIV, $1, $3); }
        | expr PLP_TOK_MOD expr
          { $$ = expr2(GPEO_MOD, $1, $3); }
        | PLP_TOK_NOT expr
          { $$ = expr1(GPEO_NOT, $2); }
        | PLP_TOK_VAR
          { $$ = expr0(GPEO_VAR); }
        | PLP_TOK_NUM
          {
          	if (($$ = expr0(GPEO_INT)) != NULL)
          	{
          		$$->val = $1;
          	}
          }
        | PLP_TOK_OPAREN expr PLP_TOK_CPAREN
          { $$ = $2; }
        ;
%%

static int yylex (YYSTYPE *lval, struct _gtr_plural_parser *arg)
{
#define EMIT(t, len) do { arg->cursor += len; return t; } while(0)
	for (;;)
	{
		if (arg->cursor[0] == '\0')
			return YYEOF;
		/* Skip whitespace */
		if (arg->cursor[0] != ' ' && arg->cursor[0] != '\t')
			break;
		++arg->cursor;
	}

	switch (arg->cursor[0])
	{
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		lval->n = strtoul(arg->cursor, (char**)&arg->cursor, 10);
		return PLP_TOK_NUM;
	case '?': EMIT(PLP_TOK_QMARK, 1);
	case ':': EMIT(PLP_TOK_COLON, 1);
	case '|':
		if (arg->cursor[1] == '|')
			EMIT(PLP_TOK_OR, 2);
		else
			return YYERRCODE;
	case '&':
		if (arg->cursor[1] == '&')
			EMIT(PLP_TOK_AND, 2);
		else
			return YYERRCODE;
	case '=':
		if (arg->cursor[1] == '=')
			EMIT(PLP_TOK_EQ, 2);
		else
			return YYERRCODE;
	case '!':
		if (arg->cursor[1] == '=')
			EMIT(PLP_TOK_NEQ, 2);
		else
			EMIT(PLP_TOK_NOT, 1);
	case '<':
		if (arg->cursor[1] == '=')
			EMIT(PLP_TOK_LTE, 2);
		else
			EMIT(PLP_TOK_LT, 1);
	case '>':
		if (arg->cursor[1] == '=')
			EMIT(PLP_TOK_GTE, 2);
		else
			EMIT(PLP_TOK_GT, 1);
	case '+': EMIT(PLP_TOK_ADD, 1);
	case '-': EMIT(PLP_TOK_SUB, 1);
	case '*': EMIT(PLP_TOK_MUL, 1);
	case '/': EMIT(PLP_TOK_DIV, 1);
	case '%': EMIT(PLP_TOK_MOD, 1);
	case 'n': EMIT(PLP_TOK_VAR, 1);
	case '(': EMIT(PLP_TOK_OPAREN, 1);
	case ')': EMIT(PLP_TOK_CPAREN, 1);

	case '\n': case ';': EMIT(YYEOF, 0);
	}
	return YYERRCODE;
#undef EMIT
}

static void yyerror(struct _gtr_plural_parser *arg, const char *str)
{
	
}
