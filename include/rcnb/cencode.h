/*
cencode.h - c header for an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#ifndef RCNB_CENCODE_H
#define RCNB_CENCODE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct
{
    bool cached;
    char trailing_byte;
} rcnb_encodestate;

void rcnb_init_encodestate(rcnb_encodestate* state_in);
size_t rcnb_encode_block(const char* plaintext_in, size_t length_in, wchar_t* code_out, rcnb_encodestate* state_in);
size_t rcnb_encode_blockend(wchar_t* code_out, rcnb_encodestate* state_in);
size_t rcnb_encode(const char* plaintext_in, size_t length_in, wchar_t* code_out);

void rcnb_encode_32n_asm(const char *value_in, char *value_out, size_t n);

#endif /* RCNB_CENCODE_H */
