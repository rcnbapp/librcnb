/*
c-example1.c - librcnb example code

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb

This is a short example of how to use librcnb's C interface to encode
and decode a string directly.
*/

#include <rcnb/cencode.h>
#include <rcnb/cdecode.h>

#include <assert.h>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* arbitrary buffer size */
#define SIZE 256

int main()
{
    const char* input = "The Quick Brown RC Jumps Over the NB Dog.";
    const wchar_t* rcnb_contrast = L"ȐčnÞȒċƝÞȐĈnƁȒȼǹþȓĆǹƃřČŇbȓƇńƄȓċȵƀȐĉņþŕƇNƅɌĉŇBȓƈȠßŕƇŃBɌċnþȓȼǸƅɌćÑbȒċƝÞƦȻƝƃŕƇNbȓƇNþŕC";
    wchar_t* encoded = malloc(SIZE * sizeof(wchar_t));
    char* decoded = malloc(SIZE);

    setlocale(LC_ALL, "");

    /* encode the data */
    rcnb_encode(input, strlen(input), encoded);
    wprintf(L"encoded: %ls\n", encoded);

    /* decode the data */
    ptrdiff_t res = rcnb_decode(encoded, wcslen(encoded), decoded);
    if (res < 0)
        wprintf(L"decode failed\n");
    wprintf(L"decoded: %s\n", decoded);

    /* compare the original and decoded data */
    assert(strcmp(input, decoded) == 0);
    assert(wcscmp(encoded, rcnb_contrast) == 0);

    free(encoded);
    free(decoded);
    return 0;
}