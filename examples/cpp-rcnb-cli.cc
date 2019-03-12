/*
rcnb.cc - c++ source to a rcnb reference encoder and decoder

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#include <rcnb/decode.h>
#include <rcnb/encode.h>

#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

void usage()
{
    std::cerr << \
		"rcnb: Encodes and Decodes files using rcnb\n" \
		"Usage: rcnb [-e|-d] [input] [output]\n" \
		"   Where [-e] will encode the input file into the output file,\n" \
		"         [-d] will decode the input file into the output file, and\n" \
		"         [input] and [output] are the input and output files, respectively.\n";
}

void usage(const std::string& message)
{
    usage();
    std::cerr << "Incorrect invocation of rcnb:\n";
    std::cerr << message << std::endl;
}

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        usage();
        exit(-1);
    }
    if (argc != 4)
    {
        usage("Wrong number of arguments!");
        exit(-1);
    }

    std::string input = argv[2];
    std::string output = argv[3];

    // determine whether we need to encode or decode:
    std::string choice = argv[1];
    if (choice == "-d")
    {
        std::wifstream instream(input.c_str(), std::ios_base::in | std::ios_base::binary);
        if (!instream.is_open())
        {
            usage("Could not open input file!");
            exit(-1);
        }
        std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>);
        instream.imbue(utf8_locale);

        std::ofstream outstream(output.c_str(), std::ios_base::out | std::ios_base::binary);
        if (!outstream.is_open())
        {
            usage("Could not open output file!");
            exit(-1);
        }
        rcnb::decoder D;
        D.decode(instream, outstream);
    }
    else if (choice == "-e")
    {
        std::ifstream instream(input.c_str(), std::ios_base::in | std::ios_base::binary);
        if (!instream.is_open())
        {
            usage("Could not open input file!");
            exit(-1);
        }

        std::wofstream outstream(output.c_str(), std::ios_base::out | std::ios_base::binary);
        if (!outstream.is_open())
        {
            usage("Could not open output file!");
            exit(-1);
        }
        std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>);
        outstream.imbue(utf8_locale);
        rcnb::encoder E;
        E.encode(instream, outstream);
    }
    else
    {
        std::cout<<"["<<choice<<"]"<<std::endl;
        usage("Please specify -d or -e as first argument!");
    }

    return 0;
}
