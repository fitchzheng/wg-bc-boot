#ifndef __MY_MATH_H
#define __MY_MATH_H

#ifdef IS_PLECS
#include "math.h"
#else
#include "arm_math.h"
#endif

#define CTRL_FREQ 25.0e3f
#define CTRL_TS (1.0f / CTRL_FREQ)

#define PWM_FREQ 100.0e3f
#define PWM_TS (1.0f / CTRL_PWM)

#define UP_LMT(in, lmt) (in = ((in > (lmt)) ? (lmt) : in))
#define DN_LMT(in, lmt) (in = ((in < (lmt)) ? (lmt) : in))
#define UP_DN_LMT(in, up_lmt, dn_lmt) (in = ((in > (up_lmt)) ? (up_lmt) : ((in < (dn_lmt)) ? (dn_lmt) : in)))

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define RAMP(act, tag, step) (act > tag + step)         \
                                 ? (act -= step)        \
                                 : ((act < tag - step)  \
                                        ? (act += step) \
                                        : (act = tag))

#define FLT(old, new, coef) old = old * (1 - coef) + (new) * coef

#define DEC_TO_BCD(x) (((((x) % 100) / 10) << 4) | (((x)) % 10))
#define BCD_TO_DEC(x) (((((x)) >> 4) * 10) + (((x)) & 0x0F))
#define DEC_TO_BCD_16(x) (DEC_TO_BCD((x) / 100) << 8) | (DEC_TO_BCD((x) % 100))
#define BCD_TO_DEC_16(x) (BCD_TO_DEC(x >> 8) * 100 + BCD_TO_DEC((x & 0x00FF)))

#define TIME_CNT_1MS_IN_1MS (1)
#define TIME_CNT_5MS_IN_1MS (5 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_10MS_IN_1MS (10 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_20MS_IN_1MS (20 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_200MS_IN_1MS (200 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_500MS_IN_1MS (500 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_1S_IN_1MS (1000 * TIME_CNT_1MS_IN_1MS)
#define TIME_CNT_2S_IN_1MS (2 * TIME_CNT_1S_IN_1MS)
#define TIME_CNT_3S_IN_1MS (3 * TIME_CNT_1S_IN_1MS)

#define TIME_CNT_40US_IN_CTRL_TS ((uint32_t)(CTRL_FREQ * 0.00004f))
#define TIME_CNT_120US_IN_CTRL_TS ((uint32_t)(CTRL_FREQ * 0.00012f))
#define TIME_CNT_1MS_IN_CTRL_TS ((uint32_t)(CTRL_FREQ * 0.001f))

#endif
