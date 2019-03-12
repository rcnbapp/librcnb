/*
rcnb.c - c source to an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#include <rcnb/rcnb.h>

const wchar_t cr[] = {'r','R',L'Ŕ',L'ŕ',L'Ŗ',L'ŗ',L'Ř',L'ř',L'Ʀ',L'Ȑ',L'ȑ',L'Ȓ',L'ȓ',L'Ɍ',L'ɍ'};
const wchar_t cc[] = {'c','C',L'Ć',L'ć',L'Ĉ',L'ĉ',L'Ċ',L'ċ',L'Č',L'č',L'Ƈ',L'ƈ',L'Ç',L'Ȼ',L'ȼ'};
const wchar_t cn[] = {'n','N',L'Ń',L'ń',L'Ņ',L'ņ',L'Ň',L'ň',L'Ɲ',L'ƞ',L'Ñ',L'Ǹ',L'ǹ',L'Ƞ',L'ȵ'};
const wchar_t cb[] = {'b','B',L'ƀ',L'Ɓ',L'ƃ',L'Ƅ',L'ƅ',L'ß',L'Þ',L'þ'};

const unsigned short sr = sizeof(cr) / sizeof(wchar_t);
const unsigned short sc = sizeof(cc) / sizeof(wchar_t);
const unsigned short sn = sizeof(cn) / sizeof(wchar_t);
const unsigned short sb = sizeof(cb) / sizeof(wchar_t);
