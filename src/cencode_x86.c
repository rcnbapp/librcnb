/*
cencode_x86.c - x86 intrinsic source to an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#if defined(ENABLE_AVX2) || defined(ENABLE_SSSE3)

#include <immintrin.h>
#include <rcnb/cencode.h>

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

        __m128i rc1 = _mm_or_si128(_mm_and_si128(mask1, nb1_t), _mm_andnot_si128(mask1, rc1_t));
        __m128i rc2 = _mm_or_si128(_mm_and_si128(mask2, nb2_t), _mm_andnot_si128(mask2, rc2_t));
        __m128i rc3 = _mm_or_si128(_mm_and_si128(mask3, nb3_t), _mm_andnot_si128(mask3, rc3_t));
        __m128i rc4 = _mm_or_si128(_mm_and_si128(mask4, nb4_t), _mm_andnot_si128(mask4, rc4_t));
        __m128i nb1 = _mm_or_si128(_mm_and_si128(mask1, rc1_t), _mm_andnot_si128(mask1, nb1_t));
        __m128i nb2 = _mm_or_si128(_mm_and_si128(mask2, rc2_t), _mm_andnot_si128(mask2, nb2_t));
        __m128i nb3 = _mm_or_si128(_mm_and_si128(mask3, rc3_t), _mm_andnot_si128(mask3, nb3_t));
        __m128i nb4 = _mm_or_si128(_mm_and_si128(mask4, rc4_t), _mm_andnot_si128(mask4, nb4_t));

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

#endif
