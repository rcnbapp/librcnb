// :mode=c++:

/*
decode.h - c++ wrapper for an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#ifndef RCNB_DECODE_H
#define RCNB_DECODE_H

#define BUFFERSIZE 4096

#include <iostream>

namespace rcnb {

extern "C" {
    #include "cdecode.h"
}

struct decoder {
    rcnb_decodestate _state;
    int _buffersize;

    explicit decoder(int buffersize_in = BUFFERSIZE) : _buffersize(buffersize_in)
    {
    }

    void initialize()
    {
        rcnb_init_decodestate(&_state);
    }

    size_t decode(const wchar_t* code_in, size_t length_in, char* plaintext_out) {
        return rcnb_decode_block(code_in, length_in, plaintext_out, &_state);
    }

    size_t decode_end(char* const plaintext_out) {
        return rcnb_decode_blockend(plaintext_out, &_state);
    }

    void decode(std::wistream& istream_in, std::ostream& ostream_in)
    {
        initialize();

        const int N = _buffersize;
        char* plaintext = new char[N];
        auto code = new wchar_t[3 * N];
        long plainlength;
        long codelength;

        do {
            istream_in.read(code, N);
            codelength = istream_in.gcount();

            plainlength = decode(code, codelength, plaintext);
            ostream_in.write(plaintext, plainlength);
        } while (istream_in.good() && plainlength > 0);

        plainlength = decode_end(plaintext);
        ostream_in.write(plaintext, plainlength);

        delete[] code;
        delete[] plaintext;
    }
};

} // namespace rcnb

#endif // RCNB_DECODE_H