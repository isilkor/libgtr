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

if (BUILD_CLAR)
	find_package(PythonInterp REQUIRED)
	find_package(Gettext REQUIRED)

	set(CLAR_FIXTURES "${CMAKE_CURRENT_SOURCE_DIR}/tests/resources/")
	set(CLAR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests")
	set(CLAR_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/resources" CACHE PATH "Path to test resources.")
	add_definitions(-DCLAR_FIXTURE_PATH=\"${CLAR_FIXTURES}\")
	add_definitions(-DCLAR_RESOURCES=\"${CLAR_RESOURCES}\")
	add_definitions(-DCLAR_TMPDIR=\"libgtr_tests\")

	file(COPY
		"${CMAKE_CURRENT_SOURCE_DIR}/clar/clar.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/clar/clar.h"
		DESTINATION
		"${CLAR_PATH}/"
		)
	file(COPY
		"${CMAKE_CURRENT_SOURCE_DIR}/clar/clar/"
		DESTINATION
		"${CLAR_PATH}/clar/"
		)

	include_directories("${CLAR_PATH}")
	include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libgtr")
	file(GLOB_RECURSE SRC_TEST ${CLAR_PATH}/tests/*.c ${CLAR_PATH}/tests/*.h)
	set(SRC_CLAR "${CLAR_PATH}/main.c" "${CLAR_PATH}/clar.c")

	file(GLOB_RECURSE RES_TEST "${CLAR_RESOURCES}/*.po")
	foreach(RES_PO ${RES_TEST})
		string(REGEX REPLACE "\\.po$" ".mo" RES_MO "${RES_PO}")
		add_custom_command(
			OUTPUT "${RES_MO}"
			COMMAND
				"${GETTEXT_MSGFMT_EXECUTABLE}"
				"-o" "${RES_MO}"
				"${RES_PO}"
			WORKING_DIRECTORY "${CLAR_RESOURCES}"
			VERBATIM
			)

		set_property(SOURCE "${CLAR_PATH}/main.c"
			APPEND PROPERTY OBJECT_DEPENDS "${RES_MO}")
	endforeach()

	add_custom_command(
		OUTPUT "${CLAR_PATH}/tests/clar.suite"
		COMMAND
			"${PYTHON_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/clar/generate.py"
			-f .
		DEPENDS ${SRC_TEST}
		WORKING_DIRECTORY "${CLAR_PATH}/tests"
		VERBATIM
	)
	include_directories("${CLAR_PATH}/tests")

	set_source_files_properties(
		${CLAR_PATH}/clar.c
		PROPERTIES OBJECT_DEPENDS ${CLAR_PATH}/tests/clar.suite)

	add_executable(libgtr_clar ${SRC_CLAR} ${SRC_TEST})
	target_link_libraries(libgtr_clar libgtr)

	enable_testing()
	add_test(libgtr_clar libgtr_clar -v)
endif()
