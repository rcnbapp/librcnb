/*
cencode.c - c source to an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#include <rcnb/cencode.h>

#include <stddef.h>
#include <wchar.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

const wchar_t cr[] = {'r','R',L'Ŕ',L'ŕ',L'Ŗ',L'ŗ',L'Ř',L'ř',L'Ʀ',L'Ȑ',L'ȑ',L'Ȓ',L'ȓ',L'Ɍ',L'ɍ'};
const wchar_t cc[] = {'c','C',L'Ć',L'ć',L'Ĉ',L'ĉ',L'Ċ',L'ċ',L'Č',L'č',L'Ƈ',L'ƈ',L'Ç',L'Ȼ',L'ȼ'};
const wchar_t cn[] = {'n','N',L'Ń',L'ń',L'Ņ',L'ņ',L'Ň',L'ň',L'Ɲ',L'ƞ',L'Ñ',L'Ǹ',L'ǹ',L'Ƞ',L'ȵ'};
const wchar_t cb[] = {'b','B',L'ƀ',L'Ɓ',L'ƃ',L'Ƅ',L'ƅ',L'ß',L'Þ',L'þ'};

// const unsigned short sr = sizeof(cr) / sizeof(wchar_t);
const unsigned short sc = sizeof(cc) / sizeof(wchar_t);
const unsigned short sn = sizeof(cn) / sizeof(wchar_t);
const unsigned short sb = sizeof(cb) / sizeof(wchar_t);

// const unsigned short src = sr * sc;
const unsigned short snb = sn * sb;
const unsigned short scnb = sc * snb;

void rcnb_init_encodestate(rcnb_encodestate* state_in)
{
    state_in->cached = false;
}

void rcnb_encode_short(short value_in, wchar_t** value_out)
{
    bool reverse = false;
    if (value_in > 0x7FFF) {
        reverse = true;
        value_in = (short)(value_in & 0x7FFF);
    }
    if (reverse)
        *value_out += 2;
    *(*value_out)++ = cr[value_in / scnb];
    *(*value_out)++ = cc[value_in % scnb / snb];
    if (reverse)
        *value_out -= 2;
    *(*value_out)++ = cn[value_in % snb / sb];
    *(*value_out)++ = cb[value_in % sb];
    if (reverse)
        *value_out += 2;
}

void rcnb_encode_byte(char value_in, wchar_t** value_out)
{
    if (value_in > 0x7F) {
        value_in = (char)(value_in & 0x7F);
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
        rcnb_encode_short(state_in->trailing_byte << 8 | plaintext_in[0], &code_char);
        plaintext_in++;
        length_in--;
        state_in->cached = false;
    }
    for (int i = 0; i < (length_in >> 1); ++i)
        rcnb_encode_short(plaintext_in[i * 2] << 8 | plaintext_in[i * 2 + 1], &code_char);
    if (length_in & 1) {
        state_in->trailing_byte = plaintext_in[length_in - 1];
        state_in->cached = true;
    }
    return code_char - code_out;
}

size_t rcnb_encode_blockend(wchar_t* const code_out, rcnb_encodestate* state_in)
{
    wchar_t* code_char = code_out;
    if (state_in->cached) {
        rcnb_encode_byte(state_in->trailing_byte, &code_char);
    }
    *code_char++ = '\0';
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

int main() {
    static char* input = "The Quick Brown RC Jumps Over the NB Dog.";
    wchar_t* buffer = malloc(256*sizeof(wchar_t));
    rcnb_encode(input, strlen(input), buffer);
    static wchar_t* contrast = L"ȐčnÞȒċƝÞȐĈnƁȒȼǹþȓĆǹƃřČŇbȓƇńƄȓċȵƀȐĉņþŕƇNƅɌĉŇBȓƈȠßŕƇŃBɌċnþȓȼǸƅɌćÑbȒċƝÞƦȻƝƃŕƇNbȓƇNþŕC";
    setlocale(LC_ALL, "en_US.UTF-8");
    wprintf(L"%ls\n%ls", contrast, buffer);
    free(buffer);
    return 0;
}
