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
#include "../src/gtrP.h"

static libgtr_t *gtr;

void test_moparse__initialize(void)
{
	cl_assert(NULL != (gtr = libgtr_new()));
}

void test_moparse__cleanup(void)
{
	libgtr_destroy(gtr);
	gtr = NULL;
}

void test_moparse__load_basic_file(void)
{
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"moparse", CLAR_RESOURCES "/basic.mo")
		);
	cl_assert_equal_i(1, HASH_COUNT(gtr->domains));
}

void test_moparse__load_file_with_header(void)
{
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"moparse", CLAR_RESOURCES "/header.mo")
		);
	cl_assert_equal_i(1, HASH_COUNT(gtr->domains));
}

void test_moparse__load_file_with_1_plural(void)
{
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"moparse", CLAR_RESOURCES "/plurals-1.mo")
		);
	cl_assert_equal_i(1, HASH_COUNT(gtr->domains));
	cl_assert_equal_i(1, gtr->domains->plurals);
}

void test_moparse__load_file_with_2_plurals(void)
{
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"moparse", CLAR_RESOURCES "/plurals-2.mo")
		);
	cl_assert_equal_i(1, HASH_COUNT(gtr->domains));
	cl_assert_equal_i(2, gtr->domains->plurals);
}

void test_moparse__load_file_with_3_plurals(void)
{
	cl_must_pass(libgtr_load_msgcat_file(gtr,
		"moparse", CLAR_RESOURCES "/plurals-3.mo")
		);
	cl_assert_equal_i(1, HASH_COUNT(gtr->domains));
	cl_assert_equal_i(3, gtr->domains->plurals);
}
