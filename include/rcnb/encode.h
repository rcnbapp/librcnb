// :mode=c++:

/*
encode.h - c++ wrapper for an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#ifndef RCNB_ENCODE_H
#define RCNB_ENCODE_H

#define BUFFERSIZE 4096

#include <iostream>

namespace rcnb {

extern "C" {
    #include "cencode.h"
}

struct encoder {
    rcnb_encodestate _state;
    int _buffersize;

    explicit encoder(int buffersize_in = BUFFERSIZE) : _buffersize(buffersize_in)
    {
    }

    void initialize()
    {
        rcnb_init_encodestate(&_state);
    }

    size_t encode(const char* plaintext_in, size_t length_in, wchar_t* const code_out) {
        return rcnb_encode_block(plaintext_in, length_in, code_out, &_state);
    }

    size_t encode_end(wchar_t* const code_out) {
        return rcnb_encode_blockend(code_out, &_state);
    }

    void encode(std::istream& istream_in, std::wostream& ostream_in)
    {
        initialize();

        const int N = _buffersize;
        char* plaintext = new char[N];
        auto code = new wchar_t[3 * N];
        long plainlength;
        long codelength;

        do {
            istream_in.read(plaintext, N);
            plainlength = istream_in.gcount();

            codelength = encode(plaintext, plainlength, code);
            ostream_in.write(code, codelength);
        } while (istream_in.good() && plainlength > 0);

        codelength = encode_end(code);
        ostream_in.write(code, codelength);

        delete[] code;
        delete[] plaintext;
    }
};

} // namespace rcnb

#endif // RCNB_ENCODE_H