/* vim: set tabstop=4 shiftwidth=4: */

/*
 * Word breaking in a Unicode sequence.  Designed to be used in a
 * generic text renderer.
 *
 * Copyright (C) 2011-2011 Tom Hacohen <tom@stosb.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the author be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software.  If you use this
 *    software in a product, an acknowledgement in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must
 *    not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 * The main reference is Unicode Standard Annex 29 (UAX #29):
 *		<URL:http://unicode.org/reports/tr29>
 *
 * When this library was designed, this annex was at Revision 17, for
 * Unicode 6.0.0:
 *		<URL:http://www.unicode.org/reports/tr29/tr29-17.html>
 *
 * The Unicode Terms of Use are available at
 *		<URL:http://www.unicode.org/copyright.html>
 */

/**
 * @file	wordbreak.h
 *
 * Header file for the word breaking (segmentation) algorithm.
 *
 * @version	2.0, 2011/12/12
 * @author	Tom Hacohen
 */

#ifndef WORDBREAK_H
#define WORDBREAK_H

#include <stddef.h>
#include <linebreak.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WORDBREAK_BREAK 0 /* Break found */
#define WORDBREAK_NOBREAK 1 /**< Break not found */
#define WORDBREAK_INSIDECHAR 2 /**< A UTF-8/16 sequence is unfinished */

void init_wordbreak(void);
void set_wordbreaks_utf8(
		const utf8_t *s, size_t len, const char* lang, char *brks);
void set_wordbreaks_utf16(
		const utf16_t *s, size_t len, const char* lang, char *brks);
void set_wordbreaks_utf32(
		const utf32_t *s, size_t len, const char* lang, char *brks);

#ifdef __cplusplus
}
#endif

#endif