# Copyright (c) 2015 Nicolas Hake <nh@nosebud.de>
#
# Permission to use, copy, modify, and / or distribute this software
# for any purpose with or without fee is hereby granted, provided that
# the above copyright notice and this permission notice appear in all
# copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS.IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
# OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

cmake_minimum_required(VERSION 3.0)
project(libgtr C)

find_package(BISON REQUIRED)
BISON_TARGET(PluralEvaluator
	"${CMAKE_CURRENT_SOURCE_DIR}/src/plurals.y"
	"${CMAKE_CURRENT_BINARY_DIR}/plurals.y.c")

add_library(libgtr
	# Interface
	libgtr/gtr.h

	# Private
	src/gtr.c
	src/gtrP.h
	src/uthash.h
	src/plurals.inl

	${BISON_PluralEvaluator_OUTPUTS}
	)
target_include_directories(libgtr
	PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/libgtr"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_BINARY_DIR}")

if (CMAKE_C_COMPILER_ID MATCHES "GNU")
	set_property(TARGET libgtr APPEND PROPERTY COMPILE_OPTIONS "-std=c99")
endif()
