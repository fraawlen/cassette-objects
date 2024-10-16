/**
 * Copyright © 2024 Fraawlen <fraawlen@posteo.net>
 *
 * This file is part of the Cassette Objects (COBJ) library.
 *
 * This library is free software; you can redistribute it and/or modify it either under the terms of the GNU
 * Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the
 * License or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
 * See the LGPL for the specific language governing rights and limitations.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this program. If not,
 * see <http://www.gnu.org/licenses/>.
 */

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

#include <cassette/cobj.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "safe.h"

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

struct slot
{
	void *ptr;
	unsigned int n_ref;
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

struct cref
{
	struct slot *slots;
	size_t n;
	size_t n_alloc;
	void *default_ptr;
	enum cerr err;
};

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

static bool grow (cref *, size_t) CREF_NONNULL(1);
static void pull (cref *, size_t) CREF_NONNULL(1);

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

cref cref_placeholder_instance = 
{
	.slots       = NULL,
	.n           = 0,
	.n_alloc     = 0,
	.default_ptr = NULL,
	.err         = CERR_INVALID,
};

/************************************************************************************************************/
/* PUBLIC ***************************************************************************************************/
/************************************************************************************************************/

void
cref_clear(cref *ref)
{
	if (ref->err)
	{
		return;
	}

	ref->n = 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

cref *
cref_clone(cref *ref)
{
	cref *ref_new;

	if (ref->err || !(ref_new = calloc(1, sizeof(cref))))
	{
		return CREF_PLACEHOLDER;
	}

	if (!grow(ref_new, ref->n_alloc))
	{
		free(ref_new);
		return CREF_PLACEHOLDER;
	}

	memcpy(ref_new->slots, ref->slots, ref->n * sizeof(struct slot));

	ref_new->n           = ref->n;
	ref_new->default_ptr = ref->default_ptr;
	ref_new->err         = CERR_NONE;

	return ref_new;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

unsigned int
cref_count(const cref *ref, size_t index)
{
	if (ref->err || index >= ref->n)
	{
		return 0;
	}

	return ref->slots[index].n_ref;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

cref *
cref_create(void)
{
	cref *ref;

	if (!(ref = calloc(1, sizeof(cref))))
	{
		return CREF_PLACEHOLDER;
	}

	if (!grow(ref, 1))
	{
		free(ref);
		return CREF_PLACEHOLDER;
	}

	ref->n           = 0;
	ref->default_ptr = NULL;
	ref->err         = CERR_NONE;

	return ref;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cref_destroy(cref *ref)
{
	if (ref == CREF_PLACEHOLDER)
	{
		return;
	}

	free(ref->slots);
	free(ref);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

enum cerr
cref_error(const cref *ref)
{
	return ref->err;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

unsigned int
cref_find(const cref *ref, void *ptr, size_t *index)
{
	if (ref->err)
	{
		return 0;
	}

	for (size_t i = 0; i < ref->n; i++)
	{
		if (ref->slots[i].ptr == ptr)
		{
			if (index)
			{
				*index = i;
			}
			return ref->slots[i].n_ref;
		}
	}

	return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

size_t
cref_length(const cref *ref)
{
	if (ref->err)
	{
		return 0;
	}

	return ref->n;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cref_prealloc(cref *ref, size_t slots_number)
{
	if (ref->err)
	{
		return;
	}

	grow(ref, slots_number);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void *
cref_ptr(const cref *ref, size_t index)
{
	if (ref->err || index >= ref->n)
	{
		return ref->default_ptr;
	}
	
	return ref->slots[index].ptr;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cref_pull_index(cref *ref, size_t index)
{
	if (ref->err || index >= ref->n || --ref->slots[index].n_ref > 0)   
	{
		return;
	}

	pull(ref, index);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cref_pull_ptr(cref *ref, void *ptr)
{
	size_t i = 0;

	if (cref_find(ref, ptr, &i) > 0)
	{
		cref_pull_index(ref, i);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cref_purge_index(cref *ref, size_t index)
{
	if (ref->err || index >= ref->n)   
	{
		return;
	}

	pull(ref, index);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cref_purge_ptr(cref *ref, void *ptr)
{
	size_t i = 0;

	if (cref_find(ref, ptr, &i) > 0)
	{
		cref_purge_index(ref, i);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cref_push(cref *ref, void *ptr)
{
	size_t i = 0;

	if (ref->err)
	{
		return;
	}

	/* if found, increment ref counter */

	if (cref_find(ref, ptr, &i) > 0)
	{
		if (ref->slots[i].n_ref == UINT_MAX)
		{
			ref->err = CERR_OVERFLOW;
			return;
		}
		ref->slots[i].n_ref++;
		return;
	}

	/* if not, add new ref */

	if (ref->n >= ref->n_alloc)
	{
		if (!safe_mul(NULL, ref->n_alloc, 2))
		{
			ref->err = CERR_OVERFLOW;
			return;
		}
		if (!grow(ref, ref->n_alloc * 2))
		{
			return;
		}
	}

	ref->slots[ref->n].ptr   = ptr;
	ref->slots[ref->n].n_ref = 1;
	ref->n++;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cref_repair(cref *ref)
{
	if (ref->err != CERR_INVALID)
	{
		ref->err = CERR_NONE;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cref_set_default_ptr(cref *ref, void *ptr)
{
	if (ref->err)
	{
		return;
	}

	ref->default_ptr = ptr;
}

/************************************************************************************************************/
/* STATIC ***************************************************************************************************/
/************************************************************************************************************/

static bool
grow(cref *ref, size_t n)
{
	struct slot *tmp;

	if (n <= ref->n_alloc)
	{
		return true;
	}

	if (!safe_mul(NULL, n, sizeof(struct slot)))
	{
		ref->err = CERR_OVERFLOW;
		return false;
	}

	if (!(tmp = realloc(ref->slots, n * sizeof(struct slot))))
	{
		ref->err = CERR_MEMORY;
		return false;
	}

	ref->n_alloc = n;
	ref->slots   = tmp;

	return true;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
pull(cref *ref, size_t i)
{
	memmove(ref->slots + i, ref->slots + i + 1, (--ref->n - i) * sizeof(struct slot));
}
