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

#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "cerr.h"

#if __GNUC__ > 4
	#define CBOOK_NONNULL_RETURN __attribute__((returns_nonnull))
	#define CBOOK_NONNULL(...)   __attribute__((nonnull (__VA_ARGS__)))
	#define CBOOK_PURE           __attribute__((pure))
#else
	#define CBOOK_NONNULL_RETURN
	#define CBOOK_NONNULL(...)
	#define CBOOK_PURE
#endif

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************************************/
/* TYPES ****************************************************************************************************/
/************************************************************************************************************/

/**
 * Opaque book object. It stores an automatically extensible array of chars. Chars are grouped into NUL
 * terminated words, and words can also be grouped. The book behaves like a stack, words can only be added or
 * erased from the end of the book.
 *
 * Some methods, upon failure, will set an error that can be checked with cbook_error(). If any error is set
 * all string methods will exit early with default return values and no side-effects. It's possible to clear
 * errors with cbook_repair().
 */
typedef struct cbook cbook;

/************************************************************************************************************/
/* GLOBALS **************************************************************************************************/
/************************************************************************************************************/

/**
 * A macro that gives uninitialized books a non-NULL value that is safe to use with the book's related
 * functions. However, any function called with a handle set to this value will return early and without any
 * side effects.
 */
#define CBOOK_PLACEHOLDER (&cbook_placeholder_instance)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/**
 * Global book instance with the error state set to CERR_INVALID. This instance is made available to allow the
 * static initialization of book pointers with the macro CBOOK_PLACEHOLDER.
 */
extern cbook cbook_placeholder_instance;

/************************************************************************************************************/
/* CONSTRUCTORS / DESTRUCTORS *******************************************************************************/
/************************************************************************************************************/

/**
 * Create a book instance and deep copy the contents of another book instance into it.
 *
 * @param book : Book to copy contents from
 *
 * @return     : New book instance
 * @return_err : CBOOK_PLACEHOLDER
 */
cbook *
cbook_clone(const cbook *book)
CBOOK_NONNULL_RETURN
CBOOK_NONNULL(1);

/**
 * Creates an empty book instance.
 *
 * @return     : New book instance
 * @return_err : CBOOK_PLACEHOLDER
 */
cbook *
cbook_create(void)
CBOOK_NONNULL_RETURN;

/**
 * Destroys the given book and frees memory.
 *
 * @param book : Book to interact with
 */
void
cbook_destroy(cbook *book)
CBOOK_NONNULL(1);

/************************************************************************************************************/
/* IMPURE METHODS *******************************************************************************************/
/************************************************************************************************************/

/**
 * Convenience for-loop wrapper. The I parameter is the global, not local, word index. Therefore, cbook_word()
 * needs to be used inside the loop instead of cbook_word_in_group().
 */
#define CBOOK_FOR_EACH(BOOK, GROUP, I) \
	for( \
		size_t I = cbook_word_index(BOOK, GROUP, 0); \
		I < cbook_word_index(BOOK, GROUP, 0) + cbook_group_length(BOOK, GROUP); \
		I++)

/**
 * Convenience inverse for-loop wrapper. The I parameter is the global, not local, word index. Therefore,
 * cbook_word() needs to be used inside the loop instead of cbook_word_in_group().
 */
#define CBOOK_FOR_EACH_REV(BOOK, GROUP, I) \
	for( \
		size_t I = cbook_group_length(BOOK, GROUP) == 0 ? \
			SIZE_MAX : \
			cbook_word_index(BOOK, GROUP, cbook_group_length(BOOK, GROUP) - 1); \
		I - cbook_word_index(BOOK, GROUP, 0) < SIZE_MAX; \
		I--)

/**
 * Clears the contents of a given book. Allocated memory is not freed, use cbook_destroy() for that.
 *
 * @param book : Book to interact with
 */
void
cbook_clear(cbook *book)
CBOOK_NONNULL(1);

/**
 * Deletes the last group of words. Allocated memory is not freed, use cbook_destroy() for that
 * 
 * @param book : Book to interact with
 */
void
cbook_pop_group(cbook *book)
CBOOK_NONNULL(1);

/**
 * Deletes the last word. Allocated memory is not freed, use cbook_destroy() for that.
 * 
 * @param book : Book to interact with
 */
void
cbook_pop_word(cbook *book)
CBOOK_NONNULL(1);

/**
 * Preallocates a set number of characters, words, references, and groups to avoid triggering multiple
 * automatic reallocs when adding data to the book. This function has no effect if the requested numbers
 * are smaller than the previously allocated amounts.
 *
 * @param book          : Book to interact with
 * @param bytes_number  : Total number of bytes across all words
 * @param words_number  : Total number of words across all groups
 * @param groups_number : Total number of groups
 *
 * @error CERR_OVERFLOW : The size of the resulting book will be > SIZE_MAX
 * @error CERR_MEMORY   : Failed memory allocation
 */
void
cbook_prealloc(cbook *book, size_t bytes_number, size_t words_number, size_t groups_number)
CBOOK_NONNULL(1);

/**
 * After this function is called, the next word that is added with cbook_write() will be part of a new group.
 *
 * @param book : Book to interact with.
 */
void
cbook_prepare_new_group(cbook *book)
CBOOK_NONNULL(1);

/**
 * Clears errors and puts the book back into an usable state. The only unrecoverable error is CBOOK_INVALID.
 *
 * @param book : Book to interact with
 */
void
cbook_repair(cbook *book)
CBOOK_NONNULL(1);

/**
 * Reverts the effects of cbook_prepare_new_group().
 *
 * @param book : Book to interact with
 */
void
cbook_undo_new_group(cbook *book)
CBOOK_NONNULL(1);

/**
 * Appends a new word to the book and increments the book word count (and possibly group count) by 1 as well
 * as the character count by the string's length (NUL terminator included). The book will automatically extend
 * its allocated memory to accommodate the new word.
 * 
 * @param book : Book to interact with
 * @param str  : C string
 *
 * @error CERR_OVERFLOW : The size of the resulting book will be > SIZE_MAX
 * @error CERR_MEMORY   : Failed memory allocation
 */
void
cbook_write(cbook *book, const char *str)
CBOOK_NONNULL(1, 2);

/**
 * Similar to cbook_clear() but all of the allocated memory is also zeroed.
 * 
 * @param book : Book to interact with
 */
void
cbook_zero(cbook *book)
CBOOK_NONNULL(1);

/************************************************************************************************************/
/* PURE METHODS *********************************************************************************************/
/************************************************************************************************************/

/**
 * Gets the errror state.
 *
 * @param book : Book to interact with
 *
 * @return : Error value
 */
enum cerr
cbook_error(const cbook *book)
CBOOK_NONNULL(1)
CBOOK_PURE;

/**
 * Gets a group's word count. If group_index is out of bounds, the default return_err value is returned.
 * 
 * @param book        : Book to interact with
 * @param group_index : Group index within book
 *
 * @return     : Number of words
 * @return_err : 0
 */
size_t
cbook_group_length(const cbook *book, size_t group_index)
CBOOK_NONNULL(1)
CBOOK_PURE;

/**
 * Gets the total number of groups.
 * 
 * @param book : Book to interact with
 *
 * @return     : Number of groups
 * @return_err : 0
 */
size_t
cbook_groups_number(const cbook *book)
CBOOK_NONNULL(1)
CBOOK_PURE;

/**
 * Gets the total length of the book (all NUL terminators included).
 *
 * @param book : Book to interact with
 *
 * @return     : Number of bytes
 * @return_err : 0
 */
size_t
cbook_length(const cbook *book)
CBOOK_NONNULL(1)
CBOOK_PURE;

/**
 * Gets a word. If word_index is out of bounds, the default return_err value is returned.
 * 
 * @param book       : Book to interact with
 * @param word_index : Word index in book across all groups
 *
 * @return     : C string
 * @return_err : "\0"
 */
const char *
cbook_word(const cbook *book, size_t word_index)
CBOOK_NONNULL_RETURN
CBOOK_NONNULL(1)
CBOOK_PURE;

/**
 * Gets a word from a specific group. If group_index or word_index are out of bounds, the default return_err
 * value is returned.
 * 
 * @param book             : Book to interact with
 * @param group_index      : Group index within book
 * @param word_local_index : Word index within group
 *
 * @return     : C string
 * @return_err : "\0"
 */
const char *
cbook_word_in_group(const cbook *book, size_t group_index, size_t word_local_index)
CBOOK_NONNULL_RETURN
CBOOK_NONNULL(1)
CBOOK_PURE;

/**
 * Converts a group + local word indexes to a book-wide word index. If group_index or word_index are out of
 * bounds, the default return_err value is returned.
 *
 * @param book             : Book to interact with
 * @param group_index      : Group index within book
 * @param word_local_index : Word index within group
 * 
 * @return     : Word index
 * @return_err : 0
 */
size_t
cbook_word_index(const cbook *book, size_t group_index, size_t word_local_index)
CBOOK_NONNULL(1)
CBOOK_PURE;

/**
 * Gets the total number of words.
 * 
 * @param book : Book to interact with
 *
 * @return     : Total number of words across all groups
 * @return_err : 0
 */
size_t
cbook_words_number(const cbook *book)
CBOOK_NONNULL(1)
CBOOK_PURE;

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

#ifdef __cplusplus
}
#endif
