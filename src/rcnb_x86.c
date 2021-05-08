/*
cencode_x86.c - x86 intrinsic source to an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#if defined(ENABLE_AVX2) || defined(ENABLE_SSSE3)

#include <immintrin.h>
#include <rcnb/cencode.h>
#include <rcnb/cdecode.h>

typedef struct concat_tbl {
    unsigned char first[16];
    unsigned char second[16];
} concat_tbl;

static const unsigned char swizzle[16] = {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14};

static const concat_tbl rc_lo = {
        {114, 82, 84, 85, 86, 87, 88, 89, 166, 16, 17, 18, 19, 76, 77},
        {99, 67, 6, 7, 8, 9, 10, 11, 12, 13, 135, 136, 199, 59, 60}
};
static const concat_tbl rc_hi = {
        {0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2},
        {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 2}
};
static const concat_tbl nb_lo = {
        {110, 78, 67, 68, 69, 70, 71, 72, 157, 158, 209, 248, 249, 32, 53},
        {98, 66, 128, 129, 131, 132, 133, 223, 222, 254}
};
static const concat_tbl nb_hi = {
        {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2, 2},
        {0, 0, 1, 1, 1, 1, 1, 0, 0, 0}
};

static const concat_tbl rc_tbl = {
        {14, 8, 0, 255, 2, 3, 4, 5, 6, 7, 9, 10, 11, 1, 12, 13},
        {13, 3, 9, 14, 4, 0, 5, 255, 10, 6, 11, 1, 7, 12, 2, 8}
};

static const concat_tbl nb_tbl = {
        {10, 3, 255, 4, 8, 0, 5, 9, 6, 1, 7, 13, 11, 14, 2, 12},
        {2, 3, 255, 255, 4, 255, 5, 6, 8, 7, 255, 1, 9, 255, 255, 0}
};

static const unsigned char s_tbl[16] = {0, 0, 255, 255, 255, 0, 255, 0};
static const concat_tbl mul_c = {
        {225, 1, 225, 1, 225, 1, 225, 1, 225, 1, 225, 1, 225, 1, 225, 1},
        {150, 1, 150, 1, 150, 1, 150, 1, 150, 1, 150, 1, 150, 1, 150, 1}
};

#ifdef __clang__
// Clang really don't like vpermd and attempts to replace it with 5+ ops.
// Mark this as potentially non-const to force Clang using vpermd.
static unsigned int permuted[8] = {0, 4, 1, 5, 2, 6, 3, 7};
static unsigned char shuffler[16] = {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};
void unused_force_clang_use_vpermd() { permuted[0] = 0; }
void unused_force_clang_use_vpshufb() { shuffler[0] = 0; }
#else
static const unsigned int permuted[8] = {0, 4, 1, 5, 2, 6, 3, 7};
static const unsigned char shuffler[16] = {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};
#endif
#endif

#ifdef ENABLE_SSSE3

#define mm_blendv_epi8(a, b, mask) _mm_or_si128(_mm_and_si128(mask, b), _mm_andnot_si128(mask, a))

void rcnb_encode_32n_asm(const char *value_in, char *value_out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        __m128i input1 = _mm_loadu_si128((__m128i *) value_in);
        input1 = _mm_shuffle_epi8(input1, *(__m128i *) &swizzle);
        // 0xffff for neg, 0x0000 for pos
        __m128i sign1 = _mm_srai_epi16(input1, 15);
        input1 = _mm_and_si128(input1, _mm_set1_epi16(0x7fff));

        __m128i input2 = _mm_loadu_si128((__m128i *) (value_in + 16));
        input2 = _mm_shuffle_epi8(input2, *(__m128i *) &swizzle);
        __m128i sign2 = _mm_srai_epi16(input2, 15);
        input2 = _mm_and_si128(input2, _mm_set1_epi16(0x7fff));

        value_in += 32;

        __m128i idx_r1, idx_c1, idx_n1, idx_b1;
        __m128i idx_r2, idx_c2, idx_n2, idx_b2;
        {
            // i / 2250 = (i * 59653) >> (16 + 11)
            idx_r1 = _mm_srli_epi16(_mm_mulhi_epu16(input1, _mm_set1_epi16(-5883)), 11);

            __m128i r_mul_2250 = _mm_mullo_epi16(idx_r1, _mm_set1_epi16(2250));
            // i % 2250
            __m128i i_mod_2250 = _mm_sub_epi16(input1, r_mul_2250);
            // i / 150 = (i * 55925) >> (16 + 7)
            idx_c1 = _mm_srli_epi16(_mm_mulhi_epu16(i_mod_2250, _mm_set1_epi16(-9611)), 7);

            __m128i c_mul_150 = _mm_add_epi16(r_mul_2250, _mm_mullo_epi16(idx_c1, _mm_set1_epi16(150)));
            // i % 150
            __m128i i_mod_150 = _mm_sub_epi16(input1, c_mul_150);
            // i / 10 = (i * 52429) >> (16 + 3);
            idx_n1 = _mm_srli_epi16(_mm_mulhi_epu16(i_mod_150, _mm_set1_epi16(-13107)), 3);

            __m128i n_mul_10 = _mm_add_epi16(c_mul_150, _mm_mullo_epi16(idx_n1, _mm_set1_epi16(10)));
            // i % 10
            idx_b1 = _mm_sub_epi16(input1, n_mul_10);
        }

        {
            idx_r2 = _mm_srli_epi16(_mm_mulhi_epu16(input2, _mm_set1_epi16(-5883)), 11);
            __m128i r_mul_2250 = _mm_mullo_epi16(idx_r2, _mm_set1_epi16(2250));
            __m128i i_mod_2250 = _mm_sub_epi16(input2, r_mul_2250);
            idx_c2 = _mm_srli_epi16(_mm_mulhi_epu16(i_mod_2250, _mm_set1_epi16(-9611)), 7);
            __m128i c_mul_150 = _mm_add_epi16(r_mul_2250, _mm_mullo_epi16(idx_c2, _mm_set1_epi16(150)));
            __m128i i_mod_150 = _mm_sub_epi16(input2, c_mul_150);
            idx_n2 = _mm_srli_epi16(_mm_mulhi_epu16(i_mod_150, _mm_set1_epi16(-13107)), 3);
            __m128i n_mul_10 = _mm_add_epi16(c_mul_150, _mm_mullo_epi16(idx_n2, _mm_set1_epi16(10)));
            idx_b2 = _mm_sub_epi16(input2, n_mul_10);
        }

        __m128i idx_r = _mm_packus_epi16(idx_r1, idx_r2);
        __m128i idx_c = _mm_packus_epi16(idx_c1, idx_c2);
        __m128i idx_n = _mm_packus_epi16(idx_n1, idx_n2);
        __m128i idx_b = _mm_packus_epi16(idx_b1, idx_b2);

        __m128i r_l = _mm_shuffle_epi8(*(__m128i *) &rc_lo.first, idx_r);
        __m128i c_l = _mm_shuffle_epi8(*(__m128i *) &rc_lo.second, idx_c);
        __m128i n_l = _mm_shuffle_epi8(*(__m128i *) &nb_lo.first, idx_n);
        __m128i b_l = _mm_shuffle_epi8(*(__m128i *) &nb_lo.second, idx_b);

        __m128i r_h = _mm_shuffle_epi8(*(__m128i *) &rc_hi.first, idx_r);
        __m128i c_h = _mm_shuffle_epi8(*(__m128i *) &rc_hi.second, idx_c);
        __m128i n_h = _mm_shuffle_epi8(*(__m128i *) &nb_hi.first, idx_n);
        __m128i b_h = _mm_shuffle_epi8(*(__m128i *) &nb_hi.second, idx_b);

        __m128i r1 = _mm_unpacklo_epi8(r_l, r_h);
        __m128i r2 = _mm_unpackhi_epi8(r_l, r_h);
        __m128i c1 = _mm_unpacklo_epi8(c_l, c_h);
        __m128i c2 = _mm_unpackhi_epi8(c_l, c_h);
        __m128i n1 = _mm_unpacklo_epi8(n_l, n_h);
        __m128i n2 = _mm_unpackhi_epi8(n_l, n_h);
        __m128i b1 = _mm_unpacklo_epi8(b_l, b_h);
        __m128i b2 = _mm_unpackhi_epi8(b_l, b_h);

        __m128i rc1_t = _mm_unpacklo_epi16(r1, c1);
        __m128i rc2_t = _mm_unpackhi_epi16(r1, c1);
        __m128i rc3_t = _mm_unpacklo_epi16(r2, c2);
        __m128i rc4_t = _mm_unpackhi_epi16(r2, c2);
        __m128i nb1_t = _mm_unpacklo_epi16(n1, b1);
        __m128i nb2_t = _mm_unpackhi_epi16(n1, b1);
        __m128i nb3_t = _mm_unpacklo_epi16(n2, b2);
        __m128i nb4_t = _mm_unpackhi_epi16(n2, b2);

        __m128i mask1 = _mm_unpacklo_epi16(sign1, sign1);
        __m128i mask2 = _mm_unpackhi_epi16(sign1, sign1);
        __m128i mask3 = _mm_unpacklo_epi16(sign2, sign2);
        __m128i mask4 = _mm_unpackhi_epi16(sign2, sign2);

        __m128i rc1 = mm_blendv_epi8(rc1_t, nb1_t, mask1);
        __m128i rc2 = mm_blendv_epi8(rc2_t, nb2_t, mask2);
        __m128i rc3 = mm_blendv_epi8(rc3_t, nb3_t, mask3);
        __m128i rc4 = mm_blendv_epi8(rc4_t, nb4_t, mask4);
        __m128i nb1 = mm_blendv_epi8(nb1_t, rc1_t, mask1);
        __m128i nb2 = mm_blendv_epi8(nb2_t, rc2_t, mask2);
        __m128i nb3 = mm_blendv_epi8(nb3_t, rc3_t, mask3);
        __m128i nb4 = mm_blendv_epi8(nb4_t, rc4_t, mask4);

        __m128i rcnb1 = _mm_unpacklo_epi32(rc1, nb1);
        __m128i rcnb2 = _mm_unpackhi_epi32(rc1, nb1);
        __m128i rcnb3 = _mm_unpacklo_epi32(rc2, nb2);
        __m128i rcnb4 = _mm_unpackhi_epi32(rc2, nb2);
        __m128i rcnb5 = _mm_unpacklo_epi32(rc3, nb3);
        __m128i rcnb6 = _mm_unpackhi_epi32(rc3, nb3);
        __m128i rcnb7 = _mm_unpacklo_epi32(rc4, nb4);
        __m128i rcnb8 = _mm_unpackhi_epi32(rc4, nb4);

        if (sizeof(wchar_t) == 2) {
            _mm_storeu_si128((__m128i *) (value_out), rcnb1);
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), rcnb2);
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), rcnb3);
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), rcnb4);
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), rcnb5);
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), rcnb6);
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), rcnb7);
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), rcnb8);
            value_out += 16;
        } else if (sizeof(wchar_t) == 4) {
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpacklo_epi16(rcnb1, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpackhi_epi16(rcnb1, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpacklo_epi16(rcnb2, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpackhi_epi16(rcnb2, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpacklo_epi16(rcnb3, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpackhi_epi16(rcnb3, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpacklo_epi16(rcnb4, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpackhi_epi16(rcnb4, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpacklo_epi16(rcnb5, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpackhi_epi16(rcnb5, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpacklo_epi16(rcnb6, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpackhi_epi16(rcnb6, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpacklo_epi16(rcnb7, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpackhi_epi16(rcnb7, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpacklo_epi16(rcnb8, _mm_setzero_si128()));
            value_out += 16;
            _mm_storeu_si128((__m128i *) (value_out), _mm_unpackhi_epi16(rcnb8, _mm_setzero_si128()));
            value_out += 16;
        }
    }
}

int rcnb_decode_32n_asm(const char *value_in, char *value_out, size_t n) {
    __m128i rcnb1, rcnb2, rcnb3, rcnb4, rcnb5, rcnb6, rcnb7, rcnb8;

    __m128i s_t = *(__m128i *) &s_tbl;

    __m128i r_t = *(__m128i *) rc_tbl.first;
    __m128i c_t = *(__m128i *) rc_tbl.second;
    __m128i n_t = *(__m128i *) nb_tbl.first;
    __m128i b_t = *(__m128i *) nb_tbl.second;

    __m128i mul_rc = *(__m128i *) mul_c.first;
    __m128i mul_nb = *(__m128i *) mul_c.second;

    __m128i r_swizzle = *(__m128i *) &swizzle;

    for (size_t i = 0; i < n; ++i) {
        if (sizeof(wchar_t) == 2) {
            rcnb1 = _mm_loadu_si128((__m128i *) value_in);
            rcnb2 = _mm_loadu_si128((__m128i *) (value_in + 16));
            rcnb3 = _mm_loadu_si128((__m128i *) (value_in + 32));
            rcnb4 = _mm_loadu_si128((__m128i *) (value_in + 48));
            rcnb5 = _mm_loadu_si128((__m128i *) (value_in + 64));
            rcnb6 = _mm_loadu_si128((__m128i *) (value_in + 80));
            rcnb7 = _mm_loadu_si128((__m128i *) (value_in + 96));
            rcnb8 = _mm_loadu_si128((__m128i *) (value_in + 112));
            value_in += 128;
        } else if (sizeof(wchar_t) == 4) {
            rcnb1 = _mm_packs_epi32(_mm_loadu_si128((__m128i *) value_in),
                                    _mm_loadu_si128((__m128i *) (value_in + 16)));
            value_in += 32;
            rcnb2 = _mm_packs_epi32(_mm_loadu_si128((__m128i *) value_in),
                                    _mm_loadu_si128((__m128i *) (value_in + 16)));
            value_in += 32;
            rcnb3 = _mm_packs_epi32(_mm_loadu_si128((__m128i *) value_in),
                                    _mm_loadu_si128((__m128i *) (value_in + 16)));
            value_in += 32;
            rcnb4 = _mm_packs_epi32(_mm_loadu_si128((__m128i *) value_in),
                                    _mm_loadu_si128((__m128i *) (value_in + 16)));
            value_in += 32;
            rcnb5 = _mm_packs_epi32(_mm_loadu_si128((__m128i *) value_in),
                                    _mm_loadu_si128((__m128i *) (value_in + 16)));
            value_in += 32;
            rcnb6 = _mm_packs_epi32(_mm_loadu_si128((__m128i *) value_in),
                                    _mm_loadu_si128((__m128i *) (value_in + 16)));
            value_in += 32;
            rcnb7 = _mm_packs_epi32(_mm_loadu_si128((__m128i *) value_in),
                                    _mm_loadu_si128((__m128i *) (value_in + 16)));
            value_in += 32;
            rcnb8 = _mm_packs_epi32(_mm_loadu_si128((__m128i *) value_in),
                                    _mm_loadu_si128((__m128i *) (value_in + 16)));
            value_in += 32;
        }

        __m128i r_c1t, r_c2t, c_c1t, c_c2t, n_c1t, n_c2t, b_c1t, b_c2t;

        {
            __m128i rcnb_04 = _mm_unpacklo_epi16(rcnb1, rcnb3);
            __m128i rcnb_15 = _mm_unpackhi_epi16(rcnb1, rcnb3);
            __m128i rcnb_26 = _mm_unpacklo_epi16(rcnb2, rcnb4);
            __m128i rcnb_37 = _mm_unpackhi_epi16(rcnb2, rcnb4);

            __m128i rcnb_0246_1 = _mm_unpacklo_epi16(rcnb_04, rcnb_26);
            __m128i rcnb_0246_2 = _mm_unpackhi_epi16(rcnb_04, rcnb_26);
            __m128i rcnb_1357_1 = _mm_unpacklo_epi16(rcnb_15, rcnb_37);
            __m128i rcnb_1357_2 = _mm_unpackhi_epi16(rcnb_15, rcnb_37);

            r_c1t = _mm_unpacklo_epi16(rcnb_0246_1, rcnb_1357_1);
            c_c1t = _mm_unpackhi_epi16(rcnb_0246_1, rcnb_1357_1);
            n_c1t = _mm_unpacklo_epi16(rcnb_0246_2, rcnb_1357_2);
            b_c1t = _mm_unpackhi_epi16(rcnb_0246_2, rcnb_1357_2);
        }

        {
            __m128i rcnb_04 = _mm_unpacklo_epi16(rcnb5, rcnb7);
            __m128i rcnb_15 = _mm_unpackhi_epi16(rcnb5, rcnb7);
            __m128i rcnb_26 = _mm_unpacklo_epi16(rcnb6, rcnb8);
            __m128i rcnb_37 = _mm_unpackhi_epi16(rcnb6, rcnb8);

            __m128i rcnb_0246_1 = _mm_unpacklo_epi16(rcnb_04, rcnb_26);
            __m128i rcnb_0246_2 = _mm_unpackhi_epi16(rcnb_04, rcnb_26);
            __m128i rcnb_1357_1 = _mm_unpacklo_epi16(rcnb_15, rcnb_37);
            __m128i rcnb_1357_2 = _mm_unpackhi_epi16(rcnb_15, rcnb_37);

            r_c2t = _mm_unpacklo_epi16(rcnb_0246_1, rcnb_1357_1);
            c_c2t = _mm_unpackhi_epi16(rcnb_0246_1, rcnb_1357_1);
            n_c2t = _mm_unpacklo_epi16(rcnb_0246_2, rcnb_1357_2);
            b_c2t = _mm_unpackhi_epi16(rcnb_0246_2, rcnb_1357_2);
        }

        __m128i sign_idx1 = _mm_srli_epi16(_mm_mullo_epi16(r_c1t, _mm_set1_epi16(2117)), 13);
        __m128i sign_idx2 = _mm_srli_epi16(_mm_mullo_epi16(r_c2t, _mm_set1_epi16(2117)), 13);
        __m128i sign1 = _mm_shuffle_epi8(s_t, sign_idx1);
        __m128i sign2 = _mm_shuffle_epi8(s_t, sign_idx2);
        sign1 = _mm_or_si128(sign1, _mm_slli_epi16(sign1, 8));
        sign2 = _mm_or_si128(sign2, _mm_slli_epi16(sign2, 8));

        __m128i r_c1 = mm_blendv_epi8(r_c1t, n_c1t, sign1);
        __m128i c_c1 = mm_blendv_epi8(c_c1t, b_c1t, sign1);
        __m128i n_c1 = mm_blendv_epi8(n_c1t, r_c1t, sign1);
        __m128i b_c1 = mm_blendv_epi8(b_c1t, c_c1t, sign1);
        __m128i r_c2 = mm_blendv_epi8(r_c2t, n_c2t, sign2);
        __m128i c_c2 = mm_blendv_epi8(c_c2t, b_c2t, sign2);
        __m128i n_c2 = mm_blendv_epi8(n_c2t, r_c2t, sign2);
        __m128i b_c2 = mm_blendv_epi8(b_c2t, c_c2t, sign2);

        sign1 = _mm_slli_epi16(sign1, 15);
        sign2 = _mm_slli_epi16(sign2, 15);

        __m128i r_i116 = _mm_srli_epi16(_mm_mullo_epi16(r_c1, _mm_set1_epi16(4675)), 12);
        __m128i c_i116 = _mm_srli_epi16(_mm_mullo_epi16(c_c1, _mm_set1_epi16(11482)), 12);
        __m128i n_i116 = _mm_srli_epi16(_mm_mullo_epi16(n_c1, _mm_set1_epi16(9726)), 12);
        __m128i b_i116 = _mm_and_si128(_mm_add_epi16(b_c1,
                                                     _mm_add_epi16(
                                                             _mm_srli_epi16(b_c1, 1),
                                                             _mm_srli_epi16(b_c1, 3))),
                                       _mm_set1_epi16(15));

        __m128i r_i216 = _mm_srli_epi16(_mm_mullo_epi16(r_c2, _mm_set1_epi16(4675)), 12);
        __m128i c_i216 = _mm_srli_epi16(_mm_mullo_epi16(c_c2, _mm_set1_epi16(11482)), 12);
        __m128i n_i216 = _mm_srli_epi16(_mm_mullo_epi16(n_c2, _mm_set1_epi16(9726)), 12);
        __m128i b_i216 = _mm_and_si128(_mm_add_epi16(b_c2,
                                                     _mm_add_epi16(
                                                             _mm_srli_epi16(b_c2, 1),
                                                             _mm_srli_epi16(b_c2, 3))),
                                       _mm_set1_epi16(15));

        __m128i r_i = _mm_packus_epi16(r_i116, r_i216);
        __m128i c_i = _mm_packus_epi16(c_i116, c_i216);
        __m128i n_i = _mm_packus_epi16(n_i116, n_i216);
        __m128i b_i = _mm_packus_epi16(b_i116, b_i216);

        __m128i r_v = _mm_shuffle_epi8(r_t, r_i);
        __m128i c_v = _mm_shuffle_epi8(c_t, c_i);
        __m128i n_v = _mm_shuffle_epi8(n_t, n_i);
        __m128i b_v = _mm_shuffle_epi8(b_t, b_i);

        __m128i bad_v = _mm_or_si128(
                _mm_or_si128(
                        _mm_cmpeq_epi8(r_v, _mm_set1_epi8(-1)),
                        _mm_cmpeq_epi8(c_v, _mm_set1_epi8(-1))
                ),
                _mm_or_si128(
                        _mm_cmpeq_epi8(n_v, _mm_set1_epi8(-1)),
                        _mm_cmpeq_epi8(b_v, _mm_set1_epi8(-1))
                )
        );

        __m128i rn_1 = _mm_unpacklo_epi8(r_v, n_v);
        __m128i rn_2 = _mm_unpackhi_epi8(r_v, n_v);
        __m128i cb_1 = _mm_unpacklo_epi8(c_v, b_v);
        __m128i cb_2 = _mm_unpackhi_epi8(c_v, b_v);
        rn_1 = _mm_maddubs_epi16(mul_rc, rn_1);
        rn_2 = _mm_maddubs_epi16(mul_rc, rn_2);
        cb_1 = _mm_maddubs_epi16(mul_nb, cb_1);
        cb_2 = _mm_maddubs_epi16(mul_nb, cb_2);

        if (_mm_movemask_epi8(bad_v)) {
            return 0;
        }

        __m128i result1 = _mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(rn_1, 3), _mm_slli_epi16(rn_1, 1)), cb_1);
        __m128i result2 = _mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(rn_2, 3), _mm_slli_epi16(rn_2, 1)), cb_2);

        result1 = _mm_or_si128(result1, sign1);
        result1 = _mm_shuffle_epi8(result1, r_swizzle);
        result2 = _mm_or_si128(result2, sign2);
        result2 = _mm_shuffle_epi8(result2, r_swizzle);

        _mm_storeu_si128((__m128i *) value_out, result1);
        _mm_storeu_si128((__m128i *) (value_out + 16), result2);
        value_out += 32;
    }
    return 1;
}

#endif

#ifdef ENABLE_AVX2

void rcnb_encode_32n_asm(const char *value_in, char *value_out, size_t n) {
    __m256i r_swizzle = _mm256_broadcastsi128_si256(*(__m128i *) &swizzle);
    __m256i r_permute = *(__m256i *) &permuted;
    __m256i r_shuffler = _mm256_broadcastsi128_si256(*(__m128i *) &shuffler);
    for (size_t i = 0; i < n; ++i) {
        __m256i input = _mm256_loadu_si256((__m256i *) value_in);
        value_in += 32;
        input = _mm256_shuffle_epi8(input, r_swizzle);
        // 0xffff for neg, 0x0000 for pos
        __m256i sign = _mm256_srai_epi16(input, 15);
        input = _mm256_and_si256(input, _mm256_set1_epi16(0x7fff));

        __m256i idx_r = _mm256_srli_epi16(_mm256_mulhi_epu16(input, _mm256_set1_epi16(-5883)), 11);
        __m256i r_mul_2250 = _mm256_mullo_epi16(idx_r, _mm256_set1_epi16(2250));
        __m256i i_mod_2250 = _mm256_sub_epi16(input, r_mul_2250);
        __m256i idx_c = _mm256_srli_epi16(_mm256_mulhi_epu16(i_mod_2250, _mm256_set1_epi16(-9611)), 7);
        __m256i c_mul_150 = _mm256_add_epi16(r_mul_2250, _mm256_mullo_epi16(idx_c, _mm256_set1_epi16(150)));
        __m256i i_mod_150 = _mm256_sub_epi16(input, c_mul_150);
        __m256i idx_n = _mm256_srli_epi16(_mm256_mulhi_epu16(i_mod_150, _mm256_set1_epi16(-13107)), 3);
        __m256i n_mul_10 = _mm256_add_epi16(c_mul_150, _mm256_mullo_epi16(idx_n, _mm256_set1_epi16(10)));
        __m256i idx_b = _mm256_sub_epi16(input, n_mul_10);

        __m256i idx_rc = _mm256_packus_epi16(idx_r, idx_c);
        __m256i idx_nb = _mm256_packus_epi16(idx_n, idx_b);
        idx_rc = _mm256_permute4x64_epi64(idx_rc, 0xd8);
        idx_nb = _mm256_permute4x64_epi64(idx_nb, 0xd8);

        __m256i rc_l = _mm256_shuffle_epi8(*(__m256i *) &rc_lo, idx_rc);
        __m256i rc_h = _mm256_shuffle_epi8(*(__m256i *) &rc_hi, idx_rc);
        __m256i nb_l = _mm256_shuffle_epi8(*(__m256i *) &nb_lo, idx_nb);
        __m256i nb_h = _mm256_shuffle_epi8(*(__m256i *) &nb_hi, idx_nb);

        __m256i r1c1_t = _mm256_unpacklo_epi8(rc_l, rc_h);
        __m256i r2c2_t = _mm256_unpackhi_epi8(rc_l, rc_h);
        __m256i n1b1_t = _mm256_unpacklo_epi8(nb_l, nb_h);
        __m256i n2b2_t = _mm256_unpackhi_epi8(nb_l, nb_h);

        __m256i sign1 = _mm256_permute4x64_epi64(sign, 0b01000100);
        __m256i sign2 = _mm256_permute4x64_epi64(sign, 0b11101110);

        __m256i r1c1 = _mm256_blendv_epi8(r1c1_t, n1b1_t, sign1);
        __m256i r2c2 = _mm256_blendv_epi8(r2c2_t, n2b2_t, sign2);
        __m256i n1b1 = _mm256_blendv_epi8(n1b1_t, r1c1_t, sign1);
        __m256i n2b2 = _mm256_blendv_epi8(n2b2_t, r2c2_t, sign2);

        __m256i rn1cb1 = _mm256_unpacklo_epi16(r1c1, n1b1);
        __m256i rn2cb2 = _mm256_unpackhi_epi16(r1c1, n1b1);
        __m256i rn3cb3 = _mm256_unpacklo_epi16(r2c2, n2b2);
        __m256i rn4cb4 = _mm256_unpackhi_epi16(r2c2, n2b2);

        __m256i rncb1 = _mm256_permutevar8x32_epi32(rn1cb1, r_permute);
        __m256i rncb2 = _mm256_permutevar8x32_epi32(rn2cb2, r_permute);
        __m256i rncb3 = _mm256_permutevar8x32_epi32(rn3cb3, r_permute);
        __m256i rncb4 = _mm256_permutevar8x32_epi32(rn4cb4, r_permute);

        __m256i rcnb1 = _mm256_shuffle_epi8(rncb1, r_shuffler);
        __m256i rcnb2 = _mm256_shuffle_epi8(rncb2, r_shuffler);
        __m256i rcnb3 = _mm256_shuffle_epi8(rncb3, r_shuffler);
        __m256i rcnb4 = _mm256_shuffle_epi8(rncb4, r_shuffler);

        if (sizeof(wchar_t) == 2) {
            _mm256_storeu_si256((__m256i *) (value_out), rcnb1);
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), rcnb2);
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), rcnb3);
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), rcnb4);
            value_out += 32;
        } else if (sizeof(wchar_t) == 4) {
            _mm256_storeu_si256((__m256i *) (value_out), _mm256_cvtepi16_epi32(_mm256_extracti128_si256(rcnb1, 0)));
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), _mm256_cvtepi16_epi32(_mm256_extracti128_si256(rcnb1, 1)));
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), _mm256_cvtepi16_epi32(_mm256_extracti128_si256(rcnb2, 0)));
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), _mm256_cvtepi16_epi32(_mm256_extracti128_si256(rcnb2, 1)));
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), _mm256_cvtepi16_epi32(_mm256_extracti128_si256(rcnb3, 0)));
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), _mm256_cvtepi16_epi32(_mm256_extracti128_si256(rcnb3, 1)));
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), _mm256_cvtepi16_epi32(_mm256_extracti128_si256(rcnb4, 0)));
            value_out += 32;
            _mm256_storeu_si256((__m256i *) (value_out), _mm256_cvtepi16_epi32(_mm256_extracti128_si256(rcnb4, 1)));
            value_out += 32;
        }
    }
}

int rcnb_decode_32n_asm(const char *value_in, char *value_out, size_t n) {
    __m256i rcnb1, rcnb2, rcnb3, rcnb4;

    __m256i rc_t = *(__m256i *) &rc_tbl;
    __m256i nb_t = *(__m256i *) &nb_tbl;

    __m256i s_t = _mm256_broadcastsi128_si256(*(__m128i *) &s_tbl);
    __m256i mul = *(__m256i *) &mul_c;
    __m256i r_swizzle = _mm256_broadcastsi128_si256(*(__m128i *) &swizzle);

    for (size_t i = 0; i < n; ++i) {
        if (sizeof(wchar_t) == 2) {
            rcnb1 = _mm256_loadu_si256((__m256i*) value_in);
            rcnb2 = _mm256_loadu_si256((__m256i*) (value_in + 32));
            rcnb3 = _mm256_loadu_si256((__m256i*) (value_in + 64));
            rcnb4 = _mm256_loadu_si256((__m256i*) (value_in + 96));
            value_in += 128;
        } else if (sizeof(wchar_t) == 4) {
            __m256i tmp1, tmp2;
            tmp1 = _mm256_loadu_si256((__m256i*) value_in);
            tmp2 = _mm256_loadu_si256((__m256i*) (value_in + 32));
            value_in += 64;
            rcnb1 = _mm256_permute4x64_epi64(_mm256_packus_epi32(tmp1, tmp2), 0xd8);

            tmp1 = _mm256_loadu_si256((__m256i*) value_in);
            tmp2 = _mm256_loadu_si256((__m256i*) (value_in + 32));
            value_in += 64;
            rcnb2 = _mm256_permute4x64_epi64(_mm256_packus_epi32(tmp1, tmp2), 0xd8);

            tmp1 = _mm256_loadu_si256((__m256i*) value_in);
            tmp2 = _mm256_loadu_si256((__m256i*) (value_in + 32));
            value_in += 64;
            rcnb3 = _mm256_permute4x64_epi64(_mm256_packus_epi32(tmp1, tmp2), 0xd8);

            tmp1 = _mm256_loadu_si256((__m256i*) value_in);
            tmp2 = _mm256_loadu_si256((__m256i*) (value_in + 32));
            value_in += 64;
            rcnb4 = _mm256_permute4x64_epi64(_mm256_packus_epi32(tmp1, tmp2), 0xd8);
        }

        __m256i rcnb_0_1_8_9 = _mm256_permute2x128_si256(rcnb1, rcnb3, 0x20);
        __m256i rcnb_2_3_a_b = _mm256_permute2x128_si256(rcnb1, rcnb3, 0x31);
        __m256i rcnb_4_5_c_d = _mm256_permute2x128_si256(rcnb2, rcnb4, 0x20);
        __m256i rcnb_6_7_e_f = _mm256_permute2x128_si256(rcnb2, rcnb4, 0x31);

        __m256i rcnb_02_8a = _mm256_unpacklo_epi16(rcnb_0_1_8_9, rcnb_2_3_a_b);
        __m256i rcnb_13_9b = _mm256_unpackhi_epi16(rcnb_0_1_8_9, rcnb_2_3_a_b);
        __m256i rcnb_46_ce = _mm256_unpacklo_epi16(rcnb_4_5_c_d, rcnb_6_7_e_f);
        __m256i rcnb_57_df = _mm256_unpackhi_epi16(rcnb_4_5_c_d, rcnb_6_7_e_f);

        __m256i rcnb_0123_89ab_rc = _mm256_unpacklo_epi16(rcnb_02_8a, rcnb_13_9b);
        __m256i rcnb_0123_89ab_nb = _mm256_unpackhi_epi16(rcnb_02_8a, rcnb_13_9b);
        __m256i rcnb_4567_cdef_rc = _mm256_unpacklo_epi16(rcnb_46_ce, rcnb_57_df);
        __m256i rcnb_4567_cdef_nb = _mm256_unpackhi_epi16(rcnb_46_ce, rcnb_57_df);

        __m256i r_ct = _mm256_unpacklo_epi64(rcnb_0123_89ab_rc, rcnb_4567_cdef_rc);
        __m256i c_ct = _mm256_unpackhi_epi64(rcnb_0123_89ab_rc, rcnb_4567_cdef_rc);
        __m256i n_ct = _mm256_unpacklo_epi64(rcnb_0123_89ab_nb, rcnb_4567_cdef_nb);
        __m256i b_ct = _mm256_unpackhi_epi64(rcnb_0123_89ab_nb, rcnb_4567_cdef_nb);

        __m256i sign_idx = _mm256_srli_epi16(_mm256_mullo_epi16(r_ct, _mm256_set1_epi16(2117)), 13);
        __m256i sign = _mm256_shuffle_epi8(s_t, sign_idx);
        sign = _mm256_or_si256(sign, _mm256_slli_epi16(sign, 8));

        __m256i r_c = _mm256_blendv_epi8(r_ct, n_ct, sign);
        __m256i c_c = _mm256_blendv_epi8(c_ct, b_ct, sign);
        __m256i n_c = _mm256_blendv_epi8(n_ct, r_ct, sign);
        __m256i b_c = _mm256_blendv_epi8(b_ct, c_ct, sign);

        sign = _mm256_slli_epi16(sign, 15);

        __m256i r_i16 = _mm256_srli_epi16(_mm256_mullo_epi16(r_c, _mm256_set1_epi16(4675)), 12);
        __m256i c_i16 = _mm256_srli_epi16(_mm256_mullo_epi16(c_c, _mm256_set1_epi16(11482)), 12);
        __m256i n_i16 = _mm256_srli_epi16(_mm256_mullo_epi16(n_c, _mm256_set1_epi16(9726)), 12);
        __m256i b_i16 = _mm256_and_si256(_mm256_add_epi16(b_c,
                                                          _mm256_add_epi16(
                                                     _mm256_srli_epi16(b_c, 1),
                                                     _mm256_srli_epi16(b_c, 3))),
                                         _mm256_set1_epi16(15));

        __m256i rc_i = _mm256_permute4x64_epi64(_mm256_packus_epi16(r_i16, c_i16), 0xd8);
        __m256i nb_i = _mm256_permute4x64_epi64(_mm256_packus_epi16(n_i16, b_i16), 0xd8);

        __m256i rc_v = _mm256_shuffle_epi8(rc_t, rc_i);
        __m256i nb_v = _mm256_shuffle_epi8(nb_t, nb_i);

        __m256i bad_v = _mm256_or_si256(
                _mm256_cmpeq_epi8(rc_v, _mm256_set1_epi8(-1)),
                _mm256_cmpeq_epi8(nb_v, _mm256_set1_epi8(-1))
                );

        __m256i rn_cb_1 = _mm256_unpacklo_epi8(rc_v, nb_v);
        __m256i rn_cb_2 = _mm256_unpackhi_epi8(rc_v, nb_v);
        rn_cb_1 = _mm256_maddubs_epi16(mul, rn_cb_1);
        rn_cb_2 = _mm256_maddubs_epi16(mul, rn_cb_2);

        if (_mm256_movemask_epi8(bad_v)) {
            return 0;
        }

        __m256i rn = _mm256_permute2x128_si256(rn_cb_1, rn_cb_2, 0x20);
        __m256i cb = _mm256_permute2x128_si256(rn_cb_1, rn_cb_2, 0x31);
        __m256i result = _mm256_add_epi16(_mm256_add_epi16(_mm256_slli_epi16(rn, 3), _mm256_slli_epi16(rn, 1)), cb);
        result = _mm256_or_si256(result, sign);
        result = _mm256_shuffle_epi8(result, r_swizzle);

        _mm256_storeu_si256((__m256i*)value_out, result);
        value_out += 32;
    }
    return 1;
}
#endif
