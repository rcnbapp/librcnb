/*
rcnb.h - c header to an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#ifndef RCNB_RCNB_H
#define RCNB_RCNB_H

#include <stddef.h>

extern const wchar_t cr[];
extern const wchar_t cc[];
extern const wchar_t cn[];
extern const wchar_t cb[];

extern const unsigned short sr;
extern const unsigned short sc;
extern const unsigned short sn;
extern const unsigned short sb;

#define src (sr * sc)
#define snb (sn * sb)
#define scnb (sc * snb)

#endif // RCNB_RCNB_H
