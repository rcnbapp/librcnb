/*
c-example2.c - librcnb example code

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb

This is a short example of how to use librcnb's C interface to encode
and decode a file directly.

The main work is done between the START/STOP ENCODING and DECODING lines.
The main difference between this code and c-example1.c is that we do not
know the size of the input file before hand, and so we use to iterate over
encoding and decoding the data.
*/

#include <rcnb/cencode.h>
#include <rcnb/cdecode.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* arbitrary buffer size */
#define SIZE 100

void encode(FILE* inputFile, FILE* outputFile)
{
    /* set up a destination buffer large enough to hold the encoded data */
    int size = SIZE;
    char* input = (char*)malloc(size);
    wchar_t* encoded = (wchar_t*)malloc(3 * size * sizeof(wchar_t)); /* ~2 x input */
    /* we need an encoder and decoder state */
    rcnb_encodestate es;
    /* store the number of bytes encoded by a single call */
    size_t cnt = 0;

    /*---------- START ENCODING ----------*/
    /* initialise the encoder state */
    rcnb_init_encodestate(&es);
    /* gather data from the input and send it to the output */
    while (true)
    {
        cnt = fread(input, sizeof(char), size, inputFile);
        if (cnt == 0) break;
        rcnb_encode_block(input, cnt, encoded, &es);
        /* output the encoded bytes to the output file */
        fputws(encoded, outputFile);
    }
    /* since we have reached the end of the input file, we know that
       there is no more input data; finalise the encoding */
    rcnb_encode_blockend(encoded, &es);
    /* write the last bytes to the output file */
    fputws(encoded, outputFile);
    /*---------- STOP ENCODING  ----------*/

    free(encoded);
    free(input);
}

void decode(FILE* inputFile, FILE* outputFile)
{
    /* set up a destination buffer large enough to hold the decoded data */
    int size = SIZE;
    wchar_t* encoded = (wchar_t*)malloc(3 * size * sizeof(wchar_t));
    char* decoded = (char*)malloc(size); /* ~1/2 x encoded */
    /* we need an encoder and decoder state */
    rcnb_decodestate ds;
    /* store the number of bytes encoded by a single call */
    size_t cnt = 0;

    /*---------- START DECODING ----------*/
    /* initialise the encoder state */
    rcnb_init_decodestate(&ds);
    /* gather data from the input and send it to the output */
    while (fgetws(encoded, size, inputFile))
    {
        cnt = rcnb_decode_block(encoded, wcslen(encoded), decoded, &ds);
        /* output the encoded bytes to the output file */
        fwrite(decoded, sizeof(char), cnt, outputFile);
    }
    /* since we have reached the end of the input file, we know that
       there is no more input data; finalise the decoding */
    cnt = rcnb_decode_blockend(decoded, &ds);
    /* write the last bytes to the output file */
    fwrite(decoded, sizeof(char), cnt, outputFile);
    /*---------- STOP DECODING  ----------*/

    free(encoded);
    free(decoded);
}

int main(int argc, char** argv)
{
    FILE* inputFile;
    FILE* encodedFile;
    FILE* decodedFile;

    if (argc < 4)
    {
        printf("please supply three filenames: input, encoded & decoded\n");
        exit(-1);
    }

    /* encode the input file */

    setlocale(LC_ALL, "");
    inputFile   = fopen(argv[1], "rb, ccs=UTF-8");
    encodedFile = fopen(argv[2], "wb, ccs=UTF-8");

    encode(inputFile, encodedFile);

    fclose(inputFile);
    fclose(encodedFile);

    /* decode the encoded file */

    encodedFile = fopen(argv[2], "rb, ccs=UTF-8");
    decodedFile = fopen(argv[3], "wb, ccs=UTF-8");

    decode(encodedFile, decodedFile);

    fclose(encodedFile);
    fclose(decodedFile);

    return 0;
}

