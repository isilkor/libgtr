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

#define _POSIX_C_SOURCE 200809L

#include "../libgtr/gtr.h"
#include "gtrP.h"

#include <assert.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__unix__)
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef _MSC_VER
#define strdup _strdup
#endif

#define MO_MAGIC 0x950412de
#define MO_MAGIC_REVERSE 0xde120495

#define MO_HDR_0_0_SIZE (7 * sizeof(uint32_t))
#define MO_HDR_0_1_SIZE (12 * sizeof(uint32_t))

#define MO_HDR_0_0_OFF_REVISION (1 * sizeof(uint32_t))
#define MO_HDR_0_0_OFF_STRCOUNT (2 * sizeof(uint32_t))
#define MO_HDR_0_0_OFF_OSTRTABLE (3 * sizeof(uint32_t))
#define MO_HDR_0_0_OFF_TSTRTABLE (4 * sizeof(uint32_t))

#define MO_STRDESC_SIZE (2 * sizeof(uint32_t))

#define GTR_MAX_SANE_PLURAL_COUNT 32

#define checked_mul(prod, fac1, fac2) \
	((ULONG_MAX / fac2 < fac1 ? \
	false : (*(prod) = fac1 * fac2)) || true)

/* Plural evaluation */
#include "plurals.inl"

/* Internal domain handling functions */
/* Create and initialize a new, empty domain. */
static libgtr_domain_t *_domain_new(const char *name)
{
	libgtr_domain_t *dom = calloc(1, sizeof(libgtr_domain_t));
	if (!dom)
		return NULL;
	dom->name = strdup(name);
	if (!dom->name)
	{
		/* out of memory for name */
		free(dom);
		return NULL;
	}

	return dom;
}

/* Free all resources allocated for a domain. */
static void _domain_free(libgtr_domain_t *domain)
{
	if (!domain)
		return;

	libgtr_plural_expr_free(domain->plural_expr);

	if (domain->mmaped)
	{
#if defined(_WIN32)
		UnmapViewOfFile(domain->data);
#elif defined(UNIX)
		munmap(domain->data, domain->data_size);
#endif
	}
	else
	{
		free(domain->data);
	}

	free(domain->name);
	HASH_CLEAR(hh, domain->strings);
	free(domain->string_descriptor_block);
	free(domain);
}

/* Parse the plural specification out of a message catalog header. */
static int _domain_parse_plurals(const char *str,
	uint32_t *plural_count, libgtr_plural_expr_t **plural_expr)
{
#define NPLURALS "nplurals="
#define PLURALS "plural="
	const char *nplural_str = strstr(str, NPLURALS);
	const char *plurals_str = strstr(str, PLURALS);
	if (nplural_str != NULL && plurals_str != NULL)
	{
		*plural_count = strtoul(nplural_str + sizeof(NPLURALS) - 1,
			NULL, 10);
		*plural_expr = _plural_expr_parse(
			plurals_str + sizeof(PLURALS) - 1);
	}
	if (*plural_count == 0 || *plural_expr == NULL)
	{
		*plural_count = 2;
		*plural_expr = NULL;
	}
	return GTREOK;
#undef PLURALS
#undef NPLURALS
}

#define READ_DOM_DATA(type, off) \
	(*((type*)((char*)domain->data + (off))))
#define READ_DOM_STR(off) \
	(memchr((char*)domain->data + (off), 0, domain->data_size - (off))\
	? (char*)domain->data + (off) : NULL)
#define READ_DOM_STR_DESC(off, s, d) \
	do { \
		uint32_t soffs = READ_DOM_DATA(uint32_t, (off) + sizeof(uint32_t)); \
		s = READ_DOM_DATA(uint32_t, (off)); \
		d = READ_DOM_STR(soffs); \
	} while(0)
/* Read the message catalog string descriptor table. */
static int _domain_parse_string_table(libgtr_domain_t *domain,
	uint32_t count, uint32_t ost_offset, uint32_t tst_offset)
{
	assert(domain);

	uint32_t *ost = (uint32_t*)((char*)domain->data + ost_offset);
	uint32_t *tst = (uint32_t*)((char*)domain->data + tst_offset);
	const char *end = (char*)domain->data + domain->data_size;

	/* We know how many plural forms there are, and we know how many
	messages to parse. Allocate one big memory block to store the hash
	table. */
	size_t descriptor_size = sizeof(libgtr_string_descriptor_t) +
		sizeof(char*) * domain->plurals;
	
	char *descriptor_block = calloc(count, descriptor_size);
	domain->string_descriptor_block = descriptor_block;
	
	int result = GTREOK;
	/* Fill the hash table. */
	for (uint32_t i = 0; i < count; ++i)
	{
		libgtr_string_descriptor_t *descriptor =
			(libgtr_string_descriptor_t*)descriptor_block;
		
		/* untranslated string */
		uint32_t os_size = ost[i * 2 + 0];
		const char *os_data = (char*)domain->data + ost[i * 2 + 1];
		/* The length of the untranslated string contains the
		untranslated plural form. That form is only of interest for
		decompiling the .mo file, which we don't do. */
		os_size = strnlen(os_data, os_size);
		descriptor->msgid = os_data;

		/* translated strings */
		uint32_t ts_size = tst[i * 2 + 0];
		const char *ts_base = (char*)domain->data + tst[i * 2 + 1];
		const char *ts_data = ts_base;

		for (uint32_t p = 0; p < domain->plurals; ++p)
		{
			/* Put pointers to all translated plurals into the
			descriptor. This wastes a bit of space for files with few
			plural-aware strings, but saves us walking the plural list
			on lookup. */
			descriptor->msgstr[p] = ts_data;
			ts_data = memchr(ts_data, '\0', ts_base + ts_size - ts_data);
			if (!ts_data)
			{
				/* We ran out of plurals. Fill the rest of the plural
				table with a reference to the first form. */
				for (uint32_t p2 = p + 1; p2 < domain->plurals; ++p2)
				{
					descriptor->msgstr[p2] = ts_base;
				}
				break;
			}
			++ts_data;
			assert(ts_data < end);
		}

		/* Add the descriptor to the hashtable. */
		libgtr_string_descriptor_t *prev;
		HASH_FIND(hh, domain->strings, os_data, os_size, prev);
		if (prev)
		{
			/* There is already a string with this msgid. That is not
			legal in .mo files. */
			result = GTREINVAL;
			goto _domain_parse_string_table_err;
		}
		HASH_ADD_KEYPTR(hh, domain->strings,
			os_data, os_size,
			descriptor);

		descriptor_block += descriptor_size;
	}

	return result;

_domain_parse_string_table_err:
	HASH_CLEAR(hh, domain->strings);
	free(descriptor_block);
	domain->string_descriptor_block = NULL;
	return result;
}

/* Validate the header of the data block, then parse it into the string
descriptor table. */
static int _domain_parse_data(libgtr_domain_t *domain)
{
	assert(domain->data);

	/* The .mo file format specifies that all strings have to end with
	a NUL byte, even though we have an explicit length. This means we
	don't have to copy every string to append a NUL. */
	
	/* First, check that we have enough data for the bare header */
	if (domain->data_size < MO_HDR_0_0_SIZE)
	{
		/* Even the revision 0 header has 28 bytes worth of fields */
		return GTREINVAL;
	}
	/* Check the file magic */
	if (READ_DOM_DATA(uint32_t, 0) == MO_MAGIC)
	{
		/* Standard byte order, everything fine. */
	}
	else
	{
		/* Some magic we don't know about. Abort. */
		return GTREINVAL;
	}

	/* Read the revision. */
	uint32_t mo_revision =
		READ_DOM_DATA(uint32_t, MO_HDR_0_0_OFF_REVISION);

	/* We currently only support files with major == 0. */
	if (((mo_revision & 0xFFFF0000) >> 16) != 0)
	{
		return GTRENOTSUPP;
	}
	/* We know about minor == 1, but we don't read the data from that.
	We won't check the header data, but we'll check that the size is
	okay. */
	if (
		(mo_revision & 0x0000FFFF) >= 1 &&
		domain->data_size < MO_HDR_0_1_SIZE
		)
	{
		return GTREINVAL;
	}

	uint32_t strings =
		READ_DOM_DATA(uint32_t, MO_HDR_0_0_OFF_STRCOUNT);
	uint32_t ost_offset =
		READ_DOM_DATA(uint32_t, MO_HDR_0_0_OFF_OSTRTABLE);
	uint32_t tst_offset =
		READ_DOM_DATA(uint32_t, MO_HDR_0_0_OFF_TSTRTABLE);

	/* Make sure the string descriptor tables are inside the data
	block */
	if (domain->data_size < ost_offset + strings * MO_STRDESC_SIZE ||
		domain->data_size < tst_offset + strings * MO_STRDESC_SIZE)
	{
		return GTREINVAL;
	}

	/* All basic sanity checks succeeded. Time to start parsing. */

	/* Scan through the string table, looking for a string descriptor
	with an empty msgid. The corresponding msgstr will be the file
	header. If there is no header, assume english-style plurals: two
	forms, singular if n == 1, plural otherwise. */
	domain->plurals = 2;

	for (uint32_t i = 0; i < strings; ++i)
	{
		uint32_t msgid_size = READ_DOM_DATA(uint32_t,
			ost_offset + i * MO_STRDESC_SIZE);
		if (msgid_size == 0)
		{
			/* This is the header. */
			uint32_t msgstr_size;
			const char *msgstr_data;
			READ_DOM_STR_DESC(tst_offset + i * MO_STRDESC_SIZE,
				msgstr_size, msgstr_data);
			if (msgstr_data == NULL)
			{
				/* The header lies outside of the file. That's invalid.
				*/
				return GTREINVAL;
			}
			if (_domain_parse_plurals(msgstr_data,
				&domain->plurals, &domain->plural_expr) < 0)
			{
				return GTREINVAL;
			}
			break;
		}
	}

	assert(domain->plurals > 0);

	/* If there's more than a handful of plural forms, somebody is
	probably trying to overflow somewhere. */
	if (domain->plurals > GTR_MAX_SANE_PLURAL_COUNT)
	{
		return GTREINVAL;
	}

	/* We know how many plural forms there are: go and parse the string
	tables. */
	return _domain_parse_string_table(domain, strings,
		ost_offset, tst_offset);

#undef READ_DOM_STR_DESC
#undef READ_DOM_STR
#undef READ_DOM_DATA
}

/* Implementation follows */

libgtr_t *libgtr_new(void)
{
	libgtr_t *gtr = calloc(1, sizeof(libgtr_t));
	return gtr;
}

void libgtr_destroy(libgtr_t *gtr)
{
	/* Make sure passing NULL doesn't break anything. (cf. free()) */
	if (gtr == NULL)
		return;

	/* Clear all loaded domains. */
	libgtr_domain_t *domain, *domain_tmp;
	HASH_ITER(hh, gtr->domains, domain, domain_tmp)
	{
		HASH_DEL(gtr->domains, domain);
		_domain_free(domain);
	}

	free(gtr);
}

int libgtr_unload_domain(libgtr_t *gtr, const char *domain)
{
	if (!gtr || domain == NULL)
		return GTREINVAL;

	libgtr_domain_t *dom;
	HASH_FIND_STR(gtr->domains, domain, dom);

	if (dom)
	{
		HASH_DEL(gtr->domains, dom);
		_domain_free(dom);
	}

	return GTREOK;
}

int libgtr_load_msgcat_mem(libgtr_t *gtr, const char *domain,
	size_t size, const void *data)
{
	if (gtr == NULL || domain == NULL || size == 0 || data == NULL)
		return GTREINVAL;

	libgtr_domain_t *dom = _domain_new(domain);
	void *data_clone = malloc(size);
	
	if (!dom || !data_clone)
	{
		/* Not enough memory. Clean up and return error. */
		_domain_free(dom);
		free(data_clone);
		return GTRENOMEM;
	}

	dom->data = data_clone;
	dom->data_size = size;
	return _domain_parse_data(dom);
}

#ifdef _WIN32
static int _msgcat_map_file_w32(libgtr_t *gtr, libgtr_domain_t *dom,
	HANDLE file)
{
	/* Retrieve the size of the file. */
	LARGE_INTEGER size;
	if (!GetFileSizeEx(file, &size))
	{
		return GTRENOMEM;
	}
	if (size.HighPart != 0)
	{
		/* We're not supporting files >4G for the moment. */
		return GTRENOMEM;
	}
	dom->data_size = size.LowPart;

	/* Map the data into memory. We may want to use a copy-on-write 
	mapping later in case we want to do endianness fixups, but those are
	unsupported at the moment. */
	HANDLE mapping = CreateFileMapping(file, NULL,
		PAGE_READONLY, 0, 0, NULL);
	dom->data = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);

	/* Mapping the file into memory increases the reference count on the
	file mapping object, so we don't have to keep a handle to it around.
	*/
	CloseHandle(mapping);

	dom->mmaped = true;
	
	return GTREOK;
}
#endif

int libgtr_load_msgcat_file(libgtr_t *gtr, const char *domain,
	const char *file)
{
	if (gtr == NULL || domain == NULL || file == NULL)
		return GTREINVAL;

	libgtr_domain_t *dom = _domain_new(domain);
	if (!dom)
		return GTRENOMEM;

	int result = GTREOK;

#if defined(_WIN32)
	/* Convert the file name from UTF-8 to a wide string. */
	int file_w_sz = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		file, -1, NULL, 0);
	if (file_w_sz == 0)
	{
		result = GTREINVAL;
		goto libgtr_load_msgcat_file_cleanup;
	}

	LPWSTR file_w = malloc(sizeof(WCHAR) * file_w_sz);
	if (!file_w)
	{
		result = GTRENOMEM;
		goto libgtr_load_msgcat_file_cleanup;
	}
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		file, -1, file_w, file_w_sz);

	/* Open the file - in case we have to convert endianness we'll set
	the mapping to copy-on-write later, so we don't have to set that
	flag here. */
	HANDLE fh = CreateFileW(file_w, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, 0, NULL);
	free(file_w);
	if (fh == INVALID_HANDLE_VALUE)
	{
		result = GTRENOENT;
		goto libgtr_load_msgcat_file_cleanup;
	}

	result = _msgcat_map_file_w32(gtr, dom, fh);
	CloseHandle(fh);
#elif defined(__unix__)
	int fh = open(file, O_RDONLY);
	if (fh < 0)
	{
		switch (errno)
		{
		case EACCES: result = GTREACCES; break;
		default:
		case ENOENT: result = GTRENOENT; break;
		}
		goto libgtr_load_msgcat_file_cleanup;
	}
	/* Unfortunately there isn't a flag that says "map the whole file",
	so we have to figure out how big it is first. */
	struct stat st;
	if (fstat(fh, &st) == -1)
	{
		close(fh);
		result = GTREACCES;
		goto libgtr_load_msgcat_file_cleanup;
	}
	dom->data_size = st.st_size;

	/* Map the file. If we want to do endianness fixups later, we'd have
	to make this mapping copy-on-write. */
	dom->data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fh, 0);
	close(fh);
	if (dom->data == MAP_FAILED)
	{
		dom->data = NULL;
		result = GTRENOMEM;
		goto libgtr_load_msgcat_file_cleanup;
	}
	dom->mmaped = true;
#else
#error Some code for non-win32/non-UNIX systems should go here.
#endif

	result = _domain_parse_data(dom);
	if (result != GTREOK)
	{
		goto libgtr_load_msgcat_file_cleanup;
	}

	HASH_ADD_KEYPTR(hh, gtr->domains,
		dom->name, strlen(dom->name), dom);

libgtr_load_msgcat_file_cleanup:
	if (result != GTREOK)
	{
		_domain_free(dom);
	}
	return result;
}

/* Find requested domain. If the domain is not loaded yet, try to load.
If loading fails, cache this failure. */
libgtr_domain_t *_gtr_get_domain(libgtr_t *gtr, const char *domain)
{
	libgtr_domain_t *dom;
	HASH_FIND_STR(gtr->domains, domain, dom);
	if (!dom && gtr->dom_loader != NULL)
	{
		/* Hand off to loader callback. */
		if (gtr->dom_loader(gtr, domain, gtr->dom_loader_opaque) != 0)
		{
			/* If the callback fails, add a dummy domain to cache the
			failure. */
			dom = _domain_new(domain);
			if (dom == NULL)
				return NULL;
			HASH_ADD_KEYPTR(hh, gtr->domains,
				dom->name, strlen(dom->name), dom);
		}
	}
	if (dom == NULL || dom->data == NULL)
		return NULL;

	return dom;
}

const char *libgtr_get_translation(libgtr_t *gtr, const char *domain,
	const char *msgid, int n)
{
	if (gtr == NULL)
		return NULL;

	/* Find the bound domain. */
	libgtr_domain_t *dom = _gtr_get_domain(gtr, domain);
	if (dom == NULL)
	{
		return NULL;
	}

	/* Find the requested string inside the domain. */
	libgtr_string_descriptor_t *str;
	HASH_FIND_STR(dom->strings, msgid, str);
	if (str == NULL)
		return NULL;
	assert(strcmp(str->msgid, msgid) == 0);

	/* Run the plural form evaluator. */
	uint32_t plural_form = _plural_expr_eval(dom->plural_expr, n);
	/* If the evaluation resulted in an index that's out of bounds, 
	bail. */
	if (plural_form >= dom->plurals)
		return NULL;
	return str->msgstr[plural_form];
}

int libgtr_set_msgcat_loader(libgtr_t* gtr,
	libgtr_domain_load_cb callback, void *opaque)
{
	if (gtr == NULL)
		return GTREINVAL;

	gtr->dom_loader = callback;
	gtr->dom_loader_opaque = opaque;
	return GTREOK;
}
