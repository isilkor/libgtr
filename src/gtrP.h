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

#ifndef _LIBGTR_GTRP_H
#define _LIBGTR_GTRP_H

#include "../libgtr/gtr.h"
#include "uthash.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
	GPEO_INT,	/* integer constant */
	GPEO_VAR,	/* variable ref (i.e. n) */

	GPEO_ADD,	/* addition (+) */
	GPEO_SUB,	/* subtraction (-) */
	GPEO_MUL,	/* multiplication (*) */
	GPEO_DIV,	/* division (/) */
	GPEO_MOD,	/* modulus (%) */

	GPEO_EQ,	/* equal (==) */
	GPEO_NEQ,	/* not equal (!=) */
	GPEO_LT,	/* less than (<) */
	GPEO_LTE,	/* less than/equal (<=) */
	GPEO_GT,	/* greater than (>) */
	GPEO_GTE,	/* greater than/equal (>=) */

	GPEO_AND,	/* logical and (&&) */
	GPEO_OR,	/* logical or (||) */
	GPEO_NOT,	/* logical not (!) */

	GPEO_TERN	/* ternary operator (?:) */
} libgtr_plural_eval_operation;
typedef struct libgtr_plural_expr
{
	libgtr_plural_eval_operation op;
	union
	{
		/* constant integer value for GPEO_INT */
		int val;
		/* argument vector for other operations */
		struct libgtr_plural_expr *args[3];
	};
} libgtr_plural_expr_t;
struct _gtr_plural_parser
{
	const char *cursor;
	libgtr_plural_expr_t *result;
};

int libgtr_plural_expr_eval(libgtr_plural_expr_t *expr, int n);
void libgtr_plural_expr_free(libgtr_plural_expr_t *expr);

typedef struct libgtr_string_descriptor
{
	UT_hash_handle hh;

	const char *msgid;
	const char *msgstr[0];
} libgtr_string_descriptor_t;

typedef struct libgtr_domain
{
	char *name;

	/* parsed data */
	unsigned int plurals;
	libgtr_plural_expr_t *plural_expr;

	libgtr_string_descriptor_t *strings;
	void *string_descriptor_block;

	/* raw data */
	size_t data_size;
	void *data;

#if defined(_WIN32) || defined(__unix__)
	/* if data was mmap'ed, use munmap (not free) */
	bool mmaped;
#endif

	UT_hash_handle hh;
} libgtr_domain_t;

struct libgtr
{
	libgtr_domain_t *domains;
	struct
	{
		libgtr_domain_load_cb dom_loader;
		void *dom_loader_opaque;
	};
};

#endif
