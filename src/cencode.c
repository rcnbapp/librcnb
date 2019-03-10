/*
cencode.c - c source to an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#include <rcnb/cencode.h>
#include <rcnb/rcnb.h>

void rcnb_init_encodestate(rcnb_encodestate* state_in)
{
    state_in->cached = false;
}

void rcnb_encode_short(unsigned short value_in, wchar_t** value_out)
{
    bool reverse = false;
    if (value_in > 0x7FFF) {
        reverse = true;
        value_in = (unsigned short)(value_in & 0x7FFF);
    }
    if (reverse)
        *value_out += 2;
    *(*value_out)++ = cr[value_in / scnb];
    *(*value_out)++ = cc[value_in % scnb / snb];
    if (reverse)
        *value_out -= 4;
    *(*value_out)++ = cn[value_in % snb / sb];
    *(*value_out)++ = cb[value_in % sb];
    if (reverse)
        *value_out += 2;
}

void rcnb_encode_byte(unsigned char value_in, wchar_t** value_out)
{
    if (value_in > 0x7F) {
        value_in = (unsigned char)(value_in & 0x7F);
        *(*value_out)++ = cn[value_in / sb];
        *(*value_out) = cb[value_in % sb];
        return;
    }
    *(*value_out)++ = cr[value_in / sc];
    *(*value_out)++ = cc[value_in % sc];
}

size_t rcnb_encode_block(const char* plaintext_in, size_t length_in,
        wchar_t* const code_out, rcnb_encodestate* state_in)
{
    if (length_in == 0)
        return 0;
    wchar_t* code_char = code_out;
    if (state_in->cached) {
        rcnb_encode_short(*(unsigned char*)(&state_in->trailing_byte) << 8 | *(unsigned char*)(&plaintext_in[0]),
                &code_char);
        plaintext_in++;
        length_in--;
        state_in->cached = false;
    }
    for (int i = 0; i < (length_in >> 1); ++i)
        rcnb_encode_short(*(unsigned char*)(&plaintext_in[i * 2]) << 8 | *(unsigned char*)(&plaintext_in[i * 2 + 1]),
                &code_char);
    if (length_in & 1) {
        state_in->trailing_byte = plaintext_in[length_in - 1];
        state_in->cached = true;
    }
    *code_char = 0;
    return code_char - code_out;
}

size_t rcnb_encode_blockend(wchar_t* const code_out, rcnb_encodestate* state_in)
{
    wchar_t* code_char = code_out;
    if (state_in->cached) {
        rcnb_encode_byte(*(unsigned char*)(&state_in->trailing_byte), &code_char);
    }
    *code_char = 0;
    state_in->cached = false;
    return code_char - code_out;
}

ptrdiff_t rcnb_encode(const char* plaintext_in, size_t length_in, wchar_t* code_out)
{
    rcnb_encodestate es;
    rcnb_init_encodestate(&es);
    size_t output_size = 0;
    output_size += rcnb_encode_block(plaintext_in, length_in, code_out, &es);
    output_size += rcnb_encode_blockend(code_out + output_size, &es);
    return output_size;
}
