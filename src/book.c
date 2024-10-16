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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "safe.h"

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

struct cbook
{
	char *chars;
	size_t *words;
	size_t *groups;
	size_t n_chars;
	size_t n_words;
	size_t n_groups;
	size_t n_alloc_chars;
	size_t n_alloc_words;
	size_t n_alloc_groups;
	bool new_group;
	enum cerr err;
};

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

static size_t group_size (const cbook *, size_t) CBOOK_PURE CBOOK_NONNULL(1);
static bool   grow       (cbook *, size_t, size_t, size_t)  CBOOK_NONNULL(1);

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

cbook cbook_placeholder_instance =
{
	.chars          = NULL,
	.words          = NULL,
	.groups         = NULL,
	.n_chars        = 0,
	.n_words        = 0,
	.n_groups       = 0,
	.n_alloc_chars  = 0,
	.n_alloc_words  = 0,
	.n_alloc_groups = 0,
	.new_group      = false,
	.err            = CERR_INVALID,
};

/************************************************************************************************************/
/* PUBLIC ***************************************************************************************************/
/************************************************************************************************************/

void
cbook_clear(cbook *book)
{
	if (book->err)
	{
		return;
	}

	book->n_groups  = 0;
	book->n_words   = 0;
	book->n_chars   = 0;
	book->new_group = true;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

cbook *
cbook_clone(const cbook *book)
{
	cbook *book_new;

	if (book->err || !(book_new = calloc(1, sizeof(cbook))))
	{
		return CBOOK_PLACEHOLDER;
	}

	if (!grow(book_new, book->n_alloc_chars, book->n_alloc_words, book->n_alloc_groups))
	{
		free(book_new->chars);
		free(book_new->words);
		free(book_new->groups);
		free(book_new);
		return CBOOK_PLACEHOLDER;
	}

	memcpy(book_new->chars,  book->chars,  book->n_chars);
	memcpy(book_new->words,  book->words,  book->n_words  * sizeof(size_t));
	memcpy(book_new->groups, book->groups, book->n_groups * sizeof(size_t));

	book_new->n_chars   = book->n_chars;
	book_new->n_words   = book->n_words;
	book_new->n_groups  = book->n_groups;
	book_new->new_group = book->new_group;
	book_new->err       = CERR_NONE;

	return book_new;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

cbook *
cbook_create(void)
{
	cbook *book;

	if (!(book = calloc(1, sizeof(cbook))))
	{
		return CBOOK_PLACEHOLDER;
	}

	if (!grow(book, 1, 1, 1))
	{
		free(book->chars);
		free(book->words);
		free(book->groups);
		free(book);
		return CBOOK_PLACEHOLDER;
	}

	book->n_chars   = 0;
	book->n_words   = 0;
	book->n_groups  = 0;
	book->new_group = true;
	book->err       = CERR_NONE;

	return book;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cbook_destroy(cbook *book)
{
	if (book == CBOOK_PLACEHOLDER)
	{
		return;
	}

	free(book->groups);
	free(book->words);
	free(book->chars);
	free(book);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

enum cerr
cbook_error(const cbook *book)
{
	return book->err;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

size_t
cbook_group_length(const cbook *book, size_t group_index)
{
	if (book->err)
	{
		return 0;
	}

	return group_size(book, group_index);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

size_t
cbook_groups_number(const cbook *book)
{
	if (book->err)
	{
		return 0;
	}

	return book->n_groups;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

size_t
cbook_length(const cbook *book)
{
	if (book->err)
	{
		return 0;
	}

	return book->n_chars;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cbook_pop_group(cbook *book)
{
	if (book->err || book->n_groups == 0)
	{
		return;
	}

	book->n_chars = book->words[book->groups[--book->n_groups]];
	book->n_words = book->groups[book->n_groups];

	if (book->n_groups == 0)
	{
		book->new_group = true;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cbook_pop_word(cbook *book)
{
	if (book->err || book->n_words == 0)
	{
		return;
	}

	if (book->groups[book->n_groups - 1] == book->n_words - 1)
	{
		if (--book->n_groups == 0)
		{
			book->new_group = true;
		}
	}

	book->n_chars = book->words[--book->n_words];
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cbook_prealloc(cbook *book, size_t bytes_number, size_t words_number, size_t groups_number)
{
	if (book->err)
	{
		return;
	}

	grow(book, bytes_number, words_number, groups_number);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cbook_prepare_new_group(cbook *book)
{
	if (book->err)
	{
		return;
	}

	book->new_group = true;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cbook_repair(cbook *book)
{
	if (book->err != CERR_INVALID)
	{
		book->err = CERR_NONE;
	}
}	

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cbook_undo_new_group(cbook *book)
{
	if (book->err || book->n_groups == 0)
	{
		return;
	}

	book->new_group = false;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

const char *
cbook_word(const cbook *book, size_t word_index)
{
	if (book->err || word_index >= book->n_words)
	{
		return "";
	}

	return book->chars + book->words[word_index];
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

const char *
cbook_word_in_group(const cbook *book, size_t group_index, size_t word_local_index)
{
	if (book->err
	 || group_size(book, group_index) == 0
	 || group_size(book, group_index) <= word_local_index)
	{
		return "";
	}

	return book->chars + book->words[book->groups[group_index] + word_local_index];
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

size_t
cbook_word_index(const cbook *book, size_t group_index, size_t word_local_index)
{
	if (book->err
	 || group_size(book, group_index) == 0
	 || group_size(book, group_index) <= word_local_index)
	{
		return 0;
	}

	return book->groups[group_index] + word_local_index;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

size_t
cbook_words_number(const cbook *book)
{
	if (book->err)
	{
		return 0;
	}

	return book->n_words;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cbook_write(cbook *book, const char *str)
{
	size_t ns;
	size_t nc;
	size_t nw;
	size_t ng;

	if (book->err)
	{
		return;
	}

	ns = strlen(str) + 1;
	nc = book->n_alloc_chars;
	nw = book->n_alloc_words  * (book->n_words  >= book->n_alloc_words  ? 2 : 1);
	ng = book->n_alloc_groups * (book->n_groups >= book->n_alloc_groups ? 2 : 1);

	while (ns > nc - book->n_chars)
	{
		if (!safe_mul(&nc, nc, 2))
		{
			book->err = CERR_OVERFLOW;
			return;
		}
	}

	if (!grow(book, nc, nw, ng))
	{
		return;
	}

	if (book->new_group)
	{
		book->groups[book->n_groups++] = book->n_words;
		book->new_group = false;
	}

	memmove(book->chars + book->n_chars, str, ns);
	book->words[book->n_words++] = book->n_chars;
	book->n_chars += ns;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
cbook_zero(cbook *book)
{
	if (book->err)
	{
		return;
	}

	memset(book->chars, '\0', book->n_alloc_chars);

	book->n_groups = 0;
	book->n_words  = 0;
	book->n_chars  = 0;
}

/************************************************************************************************************/
/* STATIC ***************************************************************************************************/
/************************************************************************************************************/

static size_t
group_size(const cbook *book, size_t i)
{
	if (i >= book->n_groups)
	{
		return 0;
	}
	else if (i == book->n_groups - 1)
	{
		return book->n_words - book->groups[i];
	}
	else
	{
		return book->groups[i + 1] - book->groups[i];
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static bool
grow(cbook *book, size_t n_chars, size_t n_words, size_t n_groups)
{
	void *tmp;

	if (!safe_mul(NULL, n_words,  sizeof(size_t))
	 || !safe_mul(NULL, n_groups, sizeof(size_t)))
	{
		book->err = CERR_OVERFLOW;
		return false;
	}

	/* char */

	if (n_chars > book->n_alloc_chars)
	{
		if (!(tmp = realloc(book->chars, n_chars)))
		{
			book->err = CERR_MEMORY;
			return false;
		}
		book->n_alloc_chars = n_chars;
		book->chars = tmp;
	}

	/* word indexes */

	if (n_words > book->n_alloc_words)
	{
		if (!(tmp = realloc(book->words, n_words * sizeof(size_t))))
		{
			book->err = CERR_MEMORY;
			return false;
		}
		book->n_alloc_words = n_words;
		book->words = tmp;
	}

	/* group indexes */

	if (n_groups > book->n_alloc_groups)
	{
		if (!(tmp = realloc(book->groups, n_groups * sizeof(size_t))))
		{
			book->err = CERR_MEMORY;
			return false;
		}
		book->n_alloc_groups = n_groups;
		book->groups = tmp;
	}

	/* end */

	return true;
}
