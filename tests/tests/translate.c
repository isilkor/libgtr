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

#include "clar.h"

#include "gtr.h"

static libgtr_t *gtr;

void test_translate__initialize(void)
{
	cl_assert(NULL != (gtr = libgtr_new()));

	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"basic", CLAR_RESOURCES "/basic.mo")
		);
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"header", CLAR_RESOURCES "/header.mo")
		);
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"plurals-1", CLAR_RESOURCES "/plurals-1.mo")
		);
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"plurals-2", CLAR_RESOURCES "/plurals-2.mo")
		);
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"plurals-3", CLAR_RESOURCES "/plurals-3.mo")
		);
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"plurals-complex", CLAR_RESOURCES "/plurals-complex.mo")
		);
}

void test_translate__cleanup(void)
{
	libgtr_destroy(gtr);
	gtr = NULL;
}

void test_translate__singulars(void)
{
	cl_assert_equal_s("test 1 translation",
		libgtr_get_translation(gtr, "basic", "test 1", 1)
		);
	cl_assert_equal_s("test 2 translation",
		libgtr_get_translation(gtr, "basic", "test 2", 1)
		);
	cl_assert_equal_s("test 3 translation 0",
		libgtr_get_translation(gtr, "plurals-3", "test 3", 1)
		);
	cl_assert_equal_s("test 3 translation 0",
		libgtr_get_translation(gtr, "plurals-complex", "test 3", 1)
		);


}

void test_translate__plurals(void)
{
	cl_assert_equal_s("test 3 translation 1",
		libgtr_get_translation(gtr, "plurals-2", "test 3", 0));

	cl_assert_equal_s("test 3 translation 1",
		libgtr_get_translation(gtr, "plurals-2", "test 3", 2));

	cl_assert_equal_s("test 3 translation 1",
		libgtr_get_translation(gtr, "plurals-complex", "test 3", 2));

	cl_assert_equal_s("test 3 translation 1",
		libgtr_get_translation(gtr, "plurals-complex", "test 3", 4));

	cl_assert_equal_s("test 3 translation 1",
		libgtr_get_translation(gtr, "plurals-complex", "test 3", 22));

	cl_assert_equal_s("test 3 translation 2",
		libgtr_get_translation(gtr, "plurals-complex", "test 3", 12));
}
