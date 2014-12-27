/*

 Copyright (c) 2013, Scott Vokes <scott.vokes@atomicobject.com>
 All rights reserved.
 
 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 
 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#ifndef __HEATSHRINK_H__
#define __HEATSHRINK_H__

#define HEATSHRINK_AUTHOR "Scott Vokes <scott.vokes@atomicobject.com>"
#define HEATSHRINK_URL "https://github.com/atomicobject/heatshrink"

/* Version 0.3.1 */
#define HEATSHRINK_VERSION_MAJOR 0
#define HEATSHRINK_VERSION_MINOR 3
#define HEATSHRINK_VERSION_PATCH 1

#define HEATSHRINK_MIN_WINDOW_BITS 4
#define HEATSHRINK_MAX_WINDOW_BITS 15

#define HEATSHRINK_MIN_LOOKAHEAD_BITS 2

#define HEATSHRINK_LITERAL_MARKER 0x01
#define HEATSHRINK_BACKREF_MARKER 0x00

#endif /* __HEATSHRINK_H__ */
