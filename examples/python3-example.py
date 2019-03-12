"""
python3-example.py - librcnb example code

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb

This is a short example of how to use librcnb's shared library to encode
and decode a string directly.
"""

from ctypes import *

rcnb = CDLL('librcnb.so')

origin = 'The Quick Brown RC Jumps Over the NB Dog. 😄'.encode('utf-8')
encoded = create_unicode_buffer(256)
rcnb.rcnb_encode(c_char_p(origin), len(origin), encoded)
print(encoded.value)

decoded = create_string_buffer(256)
rcnb.rcnb_decode(encoded, len(encoded.value), decoded)
print(decoded.value.decode('utf-8'))
