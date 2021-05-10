/*
cencode_arm64.c - arm64 intrinsic source to an rcnb encoding algorithm

This is part of the librcnb project, and has been placed in the public domain.
For details, see https://github.com/rikakomoe/librcnb
*/

#if defined(ENABLE_NEON)

#include <arm_neon.h>
#include <rcnb/cencode.h>
#include <rcnb/cdecode.h>

static const unsigned char r_lo[16] = {114, 82, 84, 85, 86, 87, 88, 89, 166, 16, 17, 18, 19, 76, 77};
static const unsigned char c_lo[16] = {99, 67, 6, 7, 8, 9, 10, 11, 12, 13, 135, 136, 199, 59, 60};
static const unsigned char n_lo[16] = {110, 78, 67, 68, 69, 70, 71, 72, 157, 158, 209, 248, 249, 32, 53};
static const unsigned char b_lo[16] = {98, 66, 128, 129, 131, 132, 133, 223, 222, 254};

static const unsigned char r_hi[16] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
static const unsigned char c_hi[16] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 2};
static const unsigned char n_hi[16] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2, 2};
static const unsigned char b_hi[16] = {0, 0, 1, 1, 1, 1, 1, 0, 0, 0};

static const unsigned char s_tbl[16] = {0, 0, 255, 255, 255, 0, 255, 0};

static const unsigned char r_tbl[16] = {14, 8, 0, 255, 2, 3, 4, 5, 6, 7, 9, 10, 11, 1, 12, 13};
static const unsigned char c_tbl[16] = {13, 3, 9, 14, 4, 0, 5, 255, 10, 6, 11, 1, 7, 12, 2, 8};
static const unsigned char n_tbl[16] = {10, 3, 255, 4, 8, 0, 5, 9, 6, 1, 7, 13, 11, 14, 2, 12};
static const unsigned char b_tbl[16] = {2, 3, 255, 255, 4, 255, 5, 6, 8, 7, 255, 1, 9, 255, 255, 0};

void rcnb_encode_32n_asm(const char *value_in, char *value_out, size_t n) {
    const int16x8_t mask = vdupq_n_s16(0x7fff);
    for (size_t i = 0; i < n; ++i) {
        int16x8_t sinput1 = (int16x8_t) vrev16q_s8(vld1q_s8((const signed char *) value_in));
        int16x8_t sinput2 = (int16x8_t) vrev16q_s8(vld1q_s8((const signed char *) (value_in + 16)));
        value_in += 32;
        uint16x8_t sign1 = (uint16x8_t) vshrq_n_s16(sinput1, 15);
        uint16x8_t sign2 = (uint16x8_t) vshrq_n_s16(sinput2, 15);
        uint16x8_t input1 = (uint16x8_t) vandq_s16(sinput1, mask);
        uint16x8_t input2 = (uint16x8_t) vandq_s16(sinput2, mask);

        uint32x4_t t1, t2;
        uint16x8_t idx_r1, idx_c1, idx_n1, idx_b1;
        uint16x8_t idx_r2, idx_c2, idx_n2, idx_b2;
        {
            t1 = vmull_n_u16(vget_low_u16(input1), 59653);
            t2 = vmull_high_n_u16(input1, 59653);
            idx_r1 = vuzp2q_u16((uint16x8_t) t1, (uint16x8_t) t2);
            idx_r1 = vshrq_n_u16(idx_r1, 11);

            uint16x8_t r_mul_2250 = vmulq_n_u16(idx_r1, 2250);
            uint16x8_t i_mod_2250 = vsubq_u16(input1, r_mul_2250);
            t1 = vmull_n_u16(vget_low_u16(i_mod_2250), 55925);
            t2 = vmull_high_n_u16(i_mod_2250, 55925);
            idx_c1 = vuzp2q_u16((uint16x8_t) t1, (uint16x8_t) t2);
            idx_c1 = vshrq_n_u16(idx_c1, 7);

            uint16x8_t c_mul_150 = vmlaq_n_u16(r_mul_2250, idx_c1, 150);
            uint16x8_t i_mod_150 = vsubq_u16(input1, c_mul_150);
            t1 = vmull_n_u16(vget_low_u16(i_mod_150), 52429);
            t2 = vmull_high_n_u16(i_mod_150, 52429);
            idx_n1 = vuzp2q_u16((uint16x8_t) t1, (uint16x8_t) t2);
            idx_n1 = vshrq_n_u16(idx_n1, 3);

            idx_b1 = vsubq_u16(input1, vmlaq_n_u16(c_mul_150, idx_n1, 10));
        }

        {
            t1 = vmull_n_u16(vget_low_u16(input2), 59653);
            t2 = vmull_high_n_u16(input2, 59653);
            idx_r2 = vuzp2q_u16((uint16x8_t) t1, (uint16x8_t) t2);
            idx_r2 = vshrq_n_u16(idx_r2, 11);

            uint16x8_t r_mul_2250 = vmulq_n_u16(idx_r2, 2250);
            uint16x8_t i_mod_2250 = vsubq_u16(input2, r_mul_2250);
            t1 = vmull_n_u16(vget_low_u16(i_mod_2250), 55925);
            t2 = vmull_high_n_u16(i_mod_2250, 55925);
            idx_c2 = vuzp2q_u16((uint16x8_t) t1, (uint16x8_t) t2);
            idx_c2 = vshrq_n_u16(idx_c2, 7);

            uint16x8_t c_mul_150 = vmlaq_n_u16(r_mul_2250, idx_c2, 150);
            uint16x8_t i_mod_150 = vsubq_u16(input2, c_mul_150);
            t1 = vmull_n_u16(vget_low_u16(i_mod_150), 52429);
            t2 = vmull_high_n_u16(i_mod_150, 52429);
            idx_n2 = vuzp2q_u16((uint16x8_t) t1, (uint16x8_t) t2);
            idx_n2 = vshrq_n_u16(idx_n2, 3);

            idx_b2 = vsubq_u16(input2, vmlaq_n_u16(c_mul_150, idx_n2, 10));
        }

        uint8x8_t idx_rt = vmovn_u16(idx_r1);
        uint8x16_t idx_r = vmovn_high_u16(idx_rt, idx_r2);
        uint8x8_t idx_ct = vmovn_u16(idx_c1);
        uint8x16_t idx_c = vmovn_high_u16(idx_ct, idx_c2);
        uint8x8_t idx_nt = vmovn_u16(idx_n1);
        uint8x16_t idx_n = vmovn_high_u16(idx_nt, idx_n2);
        uint8x8_t idx_bt = vmovn_u16(idx_b1);
        uint8x16_t idx_b = vmovn_high_u16(idx_bt, idx_b2);

        uint8x16_t r_l = vqtbl1q_u8(vld1q_u8(r_lo), idx_r);
        uint8x16_t r_h = vqtbl1q_u8(vld1q_u8(r_hi), idx_r);
        uint8x16_t c_l = vqtbl1q_u8(vld1q_u8(c_lo), idx_c);
        uint8x16_t c_h = vqtbl1q_u8(vld1q_u8(c_hi), idx_c);
        uint8x16_t n_l = vqtbl1q_u8(vld1q_u8(n_lo), idx_n);
        uint8x16_t n_h = vqtbl1q_u8(vld1q_u8(n_hi), idx_n);
        uint8x16_t b_l = vqtbl1q_u8(vld1q_u8(b_lo), idx_b);
        uint8x16_t b_h = vqtbl1q_u8(vld1q_u8(b_hi), idx_b);

        uint16x8_t r1t = (uint16x8_t) vzip1q_u8(r_l, r_h);
        uint16x8_t r2t = (uint16x8_t) vzip2q_u8(r_l, r_h);
        uint16x8_t c1t = (uint16x8_t) vzip1q_u8(c_l, c_h);
        uint16x8_t c2t = (uint16x8_t) vzip2q_u8(c_l, c_h);
        uint16x8_t n1t = (uint16x8_t) vzip1q_u8(n_l, n_h);
        uint16x8_t n2t = (uint16x8_t) vzip2q_u8(n_l, n_h);
        uint16x8_t b1t = (uint16x8_t) vzip1q_u8(b_l, b_h);
        uint16x8_t b2t = (uint16x8_t) vzip2q_u8(b_l, b_h);

        if (sizeof(wchar_t) == 2) {
            uint16x8x4_t rcnb1, rcnb2;

            rcnb1.val[0] = vbslq_u16(sign1, n1t, r1t);
            rcnb1.val[1] = vbslq_u16(sign1, b1t, c1t);
            rcnb1.val[2] = vbslq_u16(sign1, r1t, n1t);
            rcnb1.val[3] = vbslq_u16(sign1, c1t, b1t);
            rcnb2.val[0] = vbslq_u16(sign2, n2t, r2t);
            rcnb2.val[1] = vbslq_u16(sign2, b2t, c2t);
            rcnb2.val[2] = vbslq_u16(sign2, r2t, n2t);
            rcnb2.val[3] = vbslq_u16(sign2, c2t, b2t);

            vst4q_u16((unsigned short *) value_out, rcnb1);
            value_out += 64;
            vst4q_u16((unsigned short *) value_out, rcnb2);
            value_out += 64;
        } else if (sizeof(wchar_t) == 4) {
            uint16x8_t r1 = vbslq_u16(sign1, n1t, r1t);
            uint16x8_t c1 = vbslq_u16(sign1, b1t, c1t);
            uint16x8_t n1 = vbslq_u16(sign1, r1t, n1t);
            uint16x8_t b1 = vbslq_u16(sign1, c1t, b1t);
            uint16x8_t r2 = vbslq_u16(sign2, n2t, r2t);
            uint16x8_t c2 = vbslq_u16(sign2, b2t, c2t);
            uint16x8_t n2 = vbslq_u16(sign2, r2t, n2t);
            uint16x8_t b2 = vbslq_u16(sign2, c2t, b2t);

            uint32x4x4_t rcnb;

            rcnb.val[0] = vmovl_u16(vget_low_u16(r1));
            rcnb.val[1] = vmovl_u16(vget_low_u16(c1));
            rcnb.val[2] = vmovl_u16(vget_low_u16(n1));
            rcnb.val[3] = vmovl_u16(vget_low_u16(b1));
            vst4q_u32((unsigned int *) value_out, rcnb);
            value_out += 64;

            rcnb.val[0] = vmovl_u16(vget_high_u16(r1));
            rcnb.val[1] = vmovl_u16(vget_high_u16(c1));
            rcnb.val[2] = vmovl_u16(vget_high_u16(n1));
            rcnb.val[3] = vmovl_u16(vget_high_u16(b1));
            vst4q_u32((unsigned int *) value_out, rcnb);
            value_out += 64;

            rcnb.val[0] = vmovl_u16(vget_low_u16(r2));
            rcnb.val[1] = vmovl_u16(vget_low_u16(c2));
            rcnb.val[2] = vmovl_u16(vget_low_u16(n2));
            rcnb.val[3] = vmovl_u16(vget_low_u16(b2));
            vst4q_u32((unsigned int *) value_out, rcnb);
            value_out += 64;

            rcnb.val[0] = vmovl_u16(vget_high_u16(r2));
            rcnb.val[1] = vmovl_u16(vget_high_u16(c2));
            rcnb.val[2] = vmovl_u16(vget_high_u16(n2));
            rcnb.val[3] = vmovl_u16(vget_high_u16(b2));
            vst4q_u32((unsigned int *) value_out, rcnb);
            value_out += 64;
        }
    }
}

int rcnb_decode_32n_asm(const char *value_in, char *value_out, size_t n) {
    uint16x8x4_t rcnb1, rcnb2;
    for (size_t i = 0; i < n; ++i) {
        if (sizeof(wchar_t) == 2) {
            rcnb1 = vld4q_u16((const unsigned short *) value_in);
            rcnb2 = vld4q_u16((const unsigned short *) (value_in + 64));
            value_in += 128;
        } else if (sizeof(wchar_t) == 4) {
            uint32x4x4_t tmp1, tmp2;
            tmp1 = vld4q_u32((const unsigned int *) value_in);
            tmp2 = vld4q_u32((const unsigned int *) (value_in + 64));
            rcnb1.val[0] = vcombine_u16(vmovn_u32(tmp1.val[0]), vmovn_u32(tmp2.val[0]));
            rcnb1.val[1] = vcombine_u16(vmovn_u32(tmp1.val[1]), vmovn_u32(tmp2.val[1]));
            rcnb1.val[2] = vcombine_u16(vmovn_u32(tmp1.val[2]), vmovn_u32(tmp2.val[2]));
            rcnb1.val[3] = vcombine_u16(vmovn_u32(tmp1.val[3]), vmovn_u32(tmp2.val[3]));
            value_in += 128;

            tmp1 = vld4q_u32((const unsigned int *) value_in);
            tmp2 = vld4q_u32((const unsigned int *) (value_in + 64));
            rcnb2.val[0] = vcombine_u16(vmovn_u32(tmp1.val[0]), vmovn_u32(tmp2.val[0]));
            rcnb2.val[1] = vcombine_u16(vmovn_u32(tmp1.val[1]), vmovn_u32(tmp2.val[1]));
            rcnb2.val[2] = vcombine_u16(vmovn_u32(tmp1.val[2]), vmovn_u32(tmp2.val[2]));
            rcnb2.val[3] = vcombine_u16(vmovn_u32(tmp1.val[3]), vmovn_u32(tmp2.val[3]));
            value_in += 128;
        }

        uint16x8_t sign_idx1 = vshrq_n_u16(vmulq_n_u16(rcnb1.val[0], 2117), 13);
        uint16x8_t sign_idx2 = vshrq_n_u16(vmulq_n_u16(rcnb2.val[0], 2117), 13);
        uint16x8_t sign1 = (uint16x8_t)vqtbl1q_u8(vld1q_u8(s_tbl), (uint8x16_t)sign_idx1);
        uint16x8_t sign2 = (uint16x8_t)vqtbl1q_u8(vld1q_u8(s_tbl), (uint8x16_t)sign_idx2);
        sign1 = vsliq_n_u16(sign1, sign1, 8);
        sign2 = vsliq_n_u16(sign2, sign2, 8);
        
        uint16x8_t r_c1 = vbslq_u16(sign1, rcnb1.val[2], rcnb1.val[0]);
        uint16x8_t c_c1 = vbslq_u16(sign1, rcnb1.val[3], rcnb1.val[1]);
        uint16x8_t n_c1 = vbslq_u16(sign1, rcnb1.val[0], rcnb1.val[2]);
        uint16x8_t b_c1 = vbslq_u16(sign1, rcnb1.val[1], rcnb1.val[3]);
        uint16x8_t r_c2 = vbslq_u16(sign2, rcnb2.val[2], rcnb2.val[0]);
        uint16x8_t c_c2 = vbslq_u16(sign2, rcnb2.val[3], rcnb2.val[1]);
        uint16x8_t n_c2 = vbslq_u16(sign2, rcnb2.val[0], rcnb2.val[2]);
        uint16x8_t b_c2 = vbslq_u16(sign2, rcnb2.val[1], rcnb2.val[3]);

        sign1 = vshlq_n_u16(sign1, 15);
        sign2 = vshlq_n_u16(sign2, 15);

        r_c1 = vshrq_n_u16(vmulq_n_u16(r_c1, 4675), 12);
        c_c1 = vshrq_n_u16(vmulq_n_u16(c_c1, 11482), 12);
        n_c1 = vshrq_n_u16(vmulq_n_u16(n_c1, 9726), 12);
        b_c1 = vsraq_n_u16(vsraq_n_u16(b_c1, b_c1, 1), b_c1, 3);

        r_c2 = vshrq_n_u16(vmulq_n_u16(r_c2, 4675), 12);
        c_c2 = vshrq_n_u16(vmulq_n_u16(c_c2, 11482), 12);
        n_c2 = vshrq_n_u16(vmulq_n_u16(n_c2, 9726), 12);
        b_c2 = vsraq_n_u16(vsraq_n_u16(b_c2, b_c2, 1), b_c2, 3);

        uint8x16_t r_v = vqtbl1q_u8(vld1q_u8(r_tbl), vcombine_u8(vmovn_u16(r_c1), vmovn_u16(r_c2)));
        uint8x16_t c_v = vqtbl1q_u8(vld1q_u8(c_tbl), vcombine_u8(vmovn_u16(c_c1), vmovn_u16(c_c2)));
        uint8x16_t n_v = vqtbl1q_u8(vld1q_u8(n_tbl), vcombine_u8(vmovn_u16(n_c1), vmovn_u16(n_c2)));
        uint8x16_t b_v = vqtbl1q_u8(vld1q_u8(b_tbl), vbicq_u8(vcombine_u8(vmovn_u16(b_c1), vmovn_u16(b_c2)), vdupq_n_u8(0xf0)));

        uint8x16_t bad_cv = vdupq_n_u8(0xff);
        uint8x16_t bad_v = vorrq_u8(vorrq_u8(vceqq_u8(r_v, bad_cv), vceqq_u8(c_v, bad_cv)),
                                    vorrq_u8(vceqq_u8(n_v, bad_cv), vceqq_u8(b_v, bad_cv)));

        uint16x8_t rn1 = vmovl_u8(vget_low_u8(n_v));
        uint16x8_t rn2 = vmovl_u8(vget_high_u8(n_v));
        rn1 = vmlal_u8(rn1, vget_low_u8(r_v), vdup_n_u8(225));
        rn2 = vmlal_u8(rn2, vget_high_u8(r_v), vdup_n_u8(225));

        uint16x8_t cb1 = vmovl_u8(vget_low_u8(b_v));
        uint16x8_t cb2 = vmovl_u8(vget_high_u8(b_v));
        cb1 = vmlal_u8(cb1, vget_low_u8(c_v), vdup_n_u8(150));
        cb2 = vmlal_u8(cb2, vget_high_u8(c_v), vdup_n_u8(150));

        if (vmaxvq_u8(bad_v)) {
            return 0;
        }

        uint16x8x2_t result;
        result.val[0] = vmlaq_n_u16(cb1, rn1, 10);
        result.val[1] = vmlaq_n_u16(cb2, rn2, 10);
        result.val[0] = vorrq_u16(result.val[0], sign1);
        result.val[1] = vorrq_u16(result.val[1], sign2);
        result.val[0] = (uint16x8_t)vrev16q_u8((uint8x16_t)result.val[0]);
        result.val[1] = (uint16x8_t)vrev16q_u8((uint8x16_t)result.val[1]);

        vst1q_u16_x2((unsigned short *) value_out, result);
        value_out += 32;
    }

    return 1;
}

#endif
