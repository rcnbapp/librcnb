/*
cpp-example3.cc - c++ source to a rcnb reference encoder and decoder

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#include <rcnb/encode.h>
#include <iostream>

int main()
{
    rcnb::encoder E;
    setlocale(LC_ALL, "");
    E.encode(std::cin, std::wcout);
    return 0;
}
