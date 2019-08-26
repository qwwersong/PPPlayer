/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#ifndef DEC_GAIN_H
#define DEC_GAIN_H

#include "typedef.h"

void D_GAIN_init(Word16t *mem);
void D_GAIN_decode(Word16t index, Word16t nbits, Word16t code[], Word16t *gain_pit,
                   Word32 *gain_cod, Word16t bfi, Word16t prev_bfi,
                   Word16t state, Word16t unusable_frame, Word16t vad_hist,
                   Word16t *mem);
void D_GAIN_adaptive_control(Word16t *sig_in, Word16t *sig_out, Word16t l_trm);
void D_GAIN_lag_concealment_init(Word16t lag_hist[]);
void D_GAIN_lag_concealment(Word16t gain_hist[], Word16t lag_hist[], Word32 *T0,
                            Word16t *old_T0, Word16t *seed,
                            Word16t unusable_frame);
void D_GAIN_adaptive_codebook_excitation(Word16t exc[], Word32 T0, Word32 frac);
void D_GAIN_pitch_sharpening(Word16t *x, Word32 pit_lag, Word16t sharp);
Word16t D_GAIN_find_voice_factor(Word16t exc[], Word16t Q_exc, Word16t gain_pit,
                               Word16t code[], Word16t gain_code,
                               Word16t L_subfr);

#endif

