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

/* plural form evaluation for libgtr */

#include "../libgtr/gtr.h"
#include "gtrP.h"

#include "plurals.y.h"

#include <assert.h>

static int _plural_expr_eval(libgtr_plural_expr_t *expr, int n)
{
	if (!expr)
		return n != 1;
	assert(expr != NULL);
#define EVAL_ARG(arg) _plural_expr_eval(expr->args[arg], n)
#define BINOP(optag, op) case optag: return EVAL_ARG(0) op EVAL_ARG(1)

	switch (expr->op)
	{
	case GPEO_INT:
		return expr->val;
	case GPEO_VAR:
		return n;
		
	BINOP(GPEO_ADD, +);
	BINOP(GPEO_SUB, -);
	BINOP(GPEO_MUL, *);
	BINOP(GPEO_DIV, /);
	BINOP(GPEO_MOD, %);

	BINOP(GPEO_EQ, ==);
	BINOP(GPEO_NEQ, !=);
	BINOP(GPEO_LT, <);
	BINOP(GPEO_LTE, <=);
	BINOP(GPEO_GT, >);
	BINOP(GPEO_GTE, >=);

	BINOP(GPEO_AND, &&);
	BINOP(GPEO_OR, ||);

	case GPEO_NOT:
		return !EVAL_ARG(0);

	case GPEO_TERN:
		return EVAL_ARG(EVAL_ARG(0) ? 1 : 2);
	}
#undef BINOP
#undef EVAL_ARG
	assert(!"Unknown plural expression opcode encountered!");
	return 0;
}

int _gtr_pluralparse(struct _gtr_plural_parser *arg);
extern int _gtr_pluraldebug;
static libgtr_plural_expr_t *_plural_expr_parse(const char *spec)
{
	/*_gtr_pluraldebug = 1;*/
	struct _gtr_plural_parser parser =
	{
		spec, NULL
	};
	if (_gtr_pluralparse(&parser) == 0)
		return parser.result;
	return NULL;
}

void libgtr_plural_expr_free(libgtr_plural_expr_t *expr)
{
	if (expr == NULL)
		return;
	if (expr->op != GPEO_INT)
	{
		for (int i = 0; i < 3; ++i)
			libgtr_plural_expr_free(expr->args[i]);
	}
	free(expr);
}
