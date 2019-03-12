/*
cdecode.c - c source to an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#include <rcnb/cdecode.h>
#include <rcnb/rcnb.h>

int find(const wchar_t* const arr, const unsigned length, const wchar_t target)
{
    for (const wchar_t* iter = arr; iter != arr + length; ++iter) {
        if (*iter == target)
            return (int)(iter - arr);
    }
    return -1;
}

void rcnb_init_decodestate(rcnb_decodestate* state_in)
{
    state_in->i = 0;
}

bool rcnb_decode_short(const wchar_t* value_in, char** value_out)
{
    bool reverse = find(cr, sr, *value_in) < 0;
    int idx[4];
    if (!reverse) {
        idx[0] = find(cr, sr, *value_in);
        idx[1] = find(cc, sc, *(value_in + 1));
        idx[2] = find(cn, sn, *(value_in + 2));
        idx[3] = find(cb, sb, *(value_in + 3));
    } else {
        idx[0] = find(cr, sr, *(value_in + 2));
        idx[1] = find(cc, sc, *(value_in + 3));
        idx[2] = find(cn, sn, *value_in);
        idx[3] = find(cb, sb, *(value_in + 1));
    }
    if (idx[0] < 0 || idx[1] < 0 || idx[2] < 0 || idx[3] < 0)
        return false;
    int result = idx[0] * scnb + idx[1] * snb + idx[2] * sb + idx[3];
    if (result > 0x7FFF)
        return false;
    result = reverse ? result | 0x8000 : result;
    *(*value_out)++ = (char)(result >> 8);
    *(*value_out)++ = (char)(result & 0xFF);
    return true;
}

bool rcnb_decode_byte(const wchar_t* value_in, char** value_out)
{
    bool nb = false;
    int idx[2] = { find(cr, sr, *value_in), find(cc, sc, *(value_in + 1)) };
    if (idx[0] < 0 || idx[1] < 0) {
        idx[0] = find(cn, sn, *value_in);
        idx[1] = find(cb, sb, *(value_in + 1));
        nb = true;
    }
    if (idx[0] < 0 || idx[1] < 0)
        return false;
    int result = nb ? idx[0] * sb + idx[1] : idx[0] * sc + idx[1];
    if (result > 0x7F)
        return false;
    *(*value_out)++ = (char)(nb ? result | 0x80 : result);
    return true;
}

ptrdiff_t rcnb_decode_block(const wchar_t* code_in, size_t length_in,
        char* const plaintext_out, rcnb_decodestate* state_in)
{
    char* plaintext_char = plaintext_out;
    bool res;
    while (state_in->i < 4 && length_in > 0) {
        state_in->trailing_code[state_in->i++] = code_in[0];
        length_in--;
        code_in++;
    }
    if (length_in == 0)
        return 0;
    res = rcnb_decode_short(state_in->trailing_code, &plaintext_char);
    if (!res)
        return -1;
    state_in->i = 0;
    for (int i = 0; i < (length_in >> 2); ++i) {
        res = rcnb_decode_short(code_in + i * 4, &plaintext_char);
        if (!res)
            return -1;
    }
    state_in->i = length_in % 4;
    for (size_t j = 0; j < state_in->i; ++j) {
        state_in->trailing_code[j] = code_in[length_in - state_in->i + j];
    }
    *plaintext_char = 0;
    return plaintext_char - plaintext_out;
}

ptrdiff_t rcnb_decode_blockend(char* const plaintext_out, rcnb_decodestate* state_in)
{
    if (state_in->i != 0 && state_in->i != 2)
        return -1;
    char* plaintext_char = plaintext_out;
    if (state_in->i == 2) {
        if(!rcnb_decode_byte(state_in->trailing_code, &plaintext_char))
            return -1;
    }
    *plaintext_char = 0;
    state_in->i = 0;
    return plaintext_char - plaintext_out;
}

ptrdiff_t rcnb_decode(const wchar_t* code_in, size_t length_in, char* plaintext_out)
{
    if (length_in == 0)
        return 0;
    rcnb_decodestate es;
    rcnb_init_decodestate(&es);
    size_t output_size = 0;
    ptrdiff_t block_size = 0;
    block_size = rcnb_decode_block(code_in, length_in, plaintext_out, &es);
    if (block_size < 0)
        return -1;
    output_size += block_size;
    block_size = rcnb_decode_blockend(plaintext_out + output_size, &es);
    if (block_size < 0)
        return -1;
    return output_size + block_size;
}
