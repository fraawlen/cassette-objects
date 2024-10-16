/**
 * Copyright © 2024 Fraawlen <fraawlen@posteo.net>
 *
 * This file is part of the Cassette Graphics (CGUI) library.
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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "safe.h"

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

struct slot
{
	unsigned int id;
	int16_t x;
	int16_t y;
	void *ptr;
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

struct cinputs
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

static bool resize (cinputs *, size_t) CINPUTS_NONNULL(1);

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

cinputs cinputs_placeholder_instance =
{
	.slots       = NULL,
	.default_ptr = NULL,
	.n           = 0,
	.n_alloc     = 0,
	.err         = CERR_INVALID,
};

/************************************************************************************************************/
/* PUBLIC ***************************************************************************************************/
/************************************************************************************************************/

void
cinputs_clear(cinputs *inputs)
{
	if (inputs->err)
	{
		return;
	}

	inputs->n = 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

cinputs *
cinputs_clone(const cinputs *inputs)
{
	cinputs *inputs_new;

	if (inputs->err || !(inputs_new = calloc(1, sizeof(cinputs))))
	{
		return CINPUTS_PLACEHOLDER;
	}

	if (!resize(inputs_new, inputs->n_alloc))
	{
		free(inputs_new);
		return CINPUTS_PLACEHOLDER;
	}

	memcpy(inputs_new->slots, inputs->slots, inputs->n * sizeof(struct slot));

	inputs_new->default_ptr = inputs->default_ptr;
	inputs_new->n           = inputs->n;
	inputs_new->err         = CERR_NONE;

	return inputs_new;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

cinputs *
cinputs_create(size_t max_inputs)
{
	cinputs *inputs;

	if (!(inputs = calloc(1, sizeof(cinputs))))
	{
		return CINPUTS_PLACEHOLDER;
	}

	if (!resize(inputs, max_inputs))
	{
		free(inputs);
		return CINPUTS_PLACEHOLDER;
	}

	inputs->default_ptr = NULL;
	inputs->n           = 0;
	inputs->err         = CERR_NONE;

	return inputs;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cinputs_destroy(cinputs *inputs)
{
	if (inputs == CINPUTS_PLACEHOLDER)
	{
		return;
	}

	free(inputs->slots);
	free(inputs);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

enum cerr
cinputs_error(const cinputs *inputs)
{
	return inputs->err;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

bool
cinputs_find(const cinputs *inputs, unsigned int id, size_t *index)
{
	if (inputs->err)
	{
		return false;
	}

	for (size_t i = 0; i < inputs->n; i++)
	{
		if (inputs->slots[i].id == id)
		{
			if (index)
			{
				*index = i;
			}
			return true;
		}
	}

	return false;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

unsigned int
cinputs_id(const cinputs *inputs, size_t index)
{
	if (inputs->err || index >= inputs->n)
	{
		return 0;
	}

	return inputs->slots[index].id;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

size_t
cinputs_load(const cinputs *inputs)
{
	if (inputs->err)
	{
		return 0;
	}

	return inputs->n;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void *
cinputs_ptr(const cinputs *inputs, size_t index)
{
	if (inputs->err || index >= inputs->n)
	{
		return inputs->default_ptr;
	}

	return inputs->slots[index].ptr;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cinputs_pull_id(cinputs *inputs, unsigned int id)
{
	size_t index;

	if (cinputs_find(inputs, id, &index))
	{
		cinputs_pull_index(inputs, index);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cinputs_pull_index(cinputs *inputs, size_t index)
{
	if (inputs->err || index >= inputs->n)
	{
		return;
	}

	memmove(
		inputs->slots + index,
		inputs->slots + index + 1,
		(--inputs->n - index) * sizeof(struct slot));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cinputs_push(cinputs *inputs, unsigned int id, int x, int y, void *ptr)
{
	if (inputs->err)
	{
		return;
	}

	cinputs_pull_id(inputs, id);
	if (inputs->n >= inputs->n_alloc)
	{
		return;
	}

	inputs->slots[inputs->n].id  = id;
	inputs->slots[inputs->n].x   = x;
	inputs->slots[inputs->n].y   = y;
	inputs->slots[inputs->n].ptr = ptr;
	inputs->n++;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cinputs_repair(cinputs *inputs)
{
	if (inputs->err != CERR_INVALID)
	{
		inputs->err = CERR_NONE;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cinputs_resize(cinputs *inputs, size_t max_inputs)
{
	if (inputs->err)
	{
		return;
	}

	resize(inputs, max_inputs);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cinputs_set_default_ptr(cinputs *inputs, void *ptr)
{
	if (inputs->err)
	{
		return;
	}

	inputs->default_ptr = ptr;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int16_t
cinputs_x(const cinputs *inputs, size_t index)
{
	if (inputs->err || index >= inputs->n)
	{
		return 0;
	}

	return inputs->slots[index].x;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int16_t
cinputs_y(const cinputs *inputs, size_t index)
{
	if (inputs->err || index >= inputs->n)
	{
		return 0;
	}

	return inputs->slots[index].y;
}


/************************************************************************************************************/
/* STATIC ***************************************************************************************************/
/************************************************************************************************************/

static bool
resize(cinputs *inputs, size_t n)
{
	struct slot *tmp;

	if (n == 0)
	{
		inputs->err = CERR_PARAM;
		return false;
	}

	if (!safe_mul(NULL, n, sizeof(struct slot)))
	{
		inputs->err = CERR_OVERFLOW;
		return false;
	}

	if (!(tmp = realloc(inputs->slots, n * sizeof(struct slot))))
	{
		inputs->err = CERR_MEMORY;
		return false;
	}

	inputs->n       = n < inputs->n ? n : inputs->n;
	inputs->n_alloc = n;
	inputs->slots   = tmp;
	
	return true;
}
