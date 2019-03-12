/*
cdecode.h - c header for an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#ifndef RCNB_CDECODE_H
#define RCNB_CDECODE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct
{
    size_t i;
    wchar_t trailing_code[4];
} rcnb_decodestate;

void rcnb_init_decodestate(rcnb_decodestate* state_in);
ptrdiff_t rcnb_decode_block(const wchar_t* code_in, size_t length_in, char* plaintext_out, rcnb_decodestate* state_in);
ptrdiff_t rcnb_decode_blockend(char* plaintext_out, rcnb_decodestate* state_in);
ptrdiff_t rcnb_decode(const wchar_t* code_in, size_t length_in, char* plaintext_out);

#endif //RCNB_CDECODE_H
