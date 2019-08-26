/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#ifndef DEC_UTIL_H
#define DEC_UTIL_H

#include "typedef.h"
#include "dec_main.h"

#define MAX_16       (short)0x7FFF
#define MIN_16       (short)0x8000



static int clip_to_sint16(int v);
Word16t D_UTIL_saturate(Word32 v);
void D_UTIL_l_extract(Word32 L_32, Word16t *hi, Word16t *lo);

Word32 D_UTIL_mpy_32(Word16t hi1, Word16t lo1, Word16t hi2, Word16t lo2);

Word32 D_UTIL_mpy_32_16(Word16t hi, Word16t lo, Word16t n);

Word16t D_UTIL_random(Word16t *seed);

Word16t D_UTIL_norm_l (Word32 L_var1);
Word16t D_UTIL_norm_s (Word16t var1);


Word32 D_UTIL_dot_product12(Word16t x[], Word16t y[], Word16t lg, Word16t *exp);
Word32 D_UTIL_pow2(Word16t exponant, Word16t fraction);
void   D_UTIL_log2(Word32 L_x, Word16t *exponent, Word16t *fraction);
void   D_UTIL_normalised_inverse_sqrt(Word32 *frac, Word16t *exp);
Word32 D_UTIL_inverse_sqrt(Word32 L_x);

void D_UTIL_signal_up_scale(Word16t x[], Word16t lg, Word16t exp);
void D_UTIL_signal_down_scale(Word16t x[], Word16t lg, Word16t exp);


void D_UTIL_dec_synthesis(Word16t Aq[], Word16t exc[], Word16t Q_new,
                          Word16t synth16k[], Word16t prms, Word16t HfIsf[],
                          Word16t mode, Word16t newDTXState, Word16t bfi,
                          Decoder_State *st);
void D_UTIL_preemph(Word16t x[], Word16t mu, Word16t lg, Word16t *mem);


#endif
