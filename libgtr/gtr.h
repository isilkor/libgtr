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

#ifndef _LIBGTR_GTR_H
#define _LIBGTR_GTR_H

/* libgtr: a partial replacement for gettext-runtime ("libintl.a") that
can load message catalogs (.mo files) from memory or arbitrary
filesystem locations.
*/

/* Thread safety: Unless specified otherwise, operations on the same
libgtr_t instance must be externally serialized. Operations on
different libgtr_t instances may be issued simultaneously. */

#include <stddef.h>

/*
Potential error return codes from the library.
*/

/* No error */
#define GTREOK 0
/* Permission denied */
#define GTREPERM (-1)
/* File not found */
#define GTRENOENT (-2)
/* Out of memory */
#define GTRENOMEM (-12)
/* Permission denied */
#define GTREACCES (-13)
/* Entity already exists */
#define GTREEXIST (-17)
/* Invalid argument */
#define GTREINVAL (-22)
/* Not supported */
#define GTRENOTSUPP (-129)

#ifdef __cplusplus
extern "C"
{
#endif

/* Handle to a library instance. */
typedef struct libgtr libgtr_t;

/* Create a new instance of libgtr */
libgtr_t *libgtr_new(void);
/* Free allocated resources of libgtr */
void libgtr_destroy(libgtr_t*);

/*
Load a message catalog into a domain from a file. If the domain did not
exist before, create it.
Returns 0 if the message catalog was successfully loaded, or nonzero in
case of error.
NOTE: Attaching multiple message catalogs to a domain is not supported
at the moment. If you try to attach a message catalog to a domain that
already has a catalog attached, the function will return GTREEXIST.
*/
int libgtr_load_msgcat_file(libgtr_t*, const char *domain,
	const char *file);
/*
Load a message catalog into a domain from a memory buffer. If the domain
did not exist before, create it.
Returns 0 if the message catalog was successfully loaded, or nonzero in
case of error.
NOTE: Attaching multiple message catalogs to a domain is not supported
at the moment. If you try to attach a message catalog to a domain that
already has a catalog attached, the function will return GTREEXIST.
*/
int libgtr_load_msgcat_mem(libgtr_t*, const char *domain, size_t size,
	const void *data);

/*
Unload a domain. This will remove all attached message catalogs. If an
on-demand domain loader is registered, it will be invoked next time a
translation request for this domain is handled.
Returns 0 if no domain with this name exists after the call, or nonzero
in case of error.
*/
int libgtr_unload_domain(libgtr_t*, const char *domain);

/*
Signature of a domain loader callback.
Should return 0 if the domain was successfully loaded, or nonzero in
case of error.
*/
typedef int (*libgtr_domain_load_cb)(libgtr_t*, const char *domain,
	void *opaque);
/*
Define a callback to be invoked when libgtr needs to load specific
domain. The callback should use one of the libgtr_load_msgcat_*
functions to load the catalog. If no callback is installed, libgtr will
not load domains on demand.
Returns 0 if the callback was successfully installed, or nonzero in case
of error.
*/
int libgtr_set_msgcat_loader(libgtr_t*, libgtr_domain_load_cb callback,
	void *opaque);

/*
Return a translation from the libgtr instance. If the requested domain
is not loaded, and a loader callback is set, the library will call that
callback and try again. For message catalogs with plural forms, n will
be used to determine the plural form. If no matching translation can be
found, the return value is NULL.
*/
const char *libgtr_get_translation(libgtr_t*, const char *domain,
	const char *msgid, int n);

#ifdef __cplusplus
}
#endif

#endif
