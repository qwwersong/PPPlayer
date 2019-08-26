/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#ifndef DEC_ACELP_H
#define DEC_ACELP_H

#include "typedef.h"

void D_ACELP_decode_2t(Word16t index, Word16t code[]);
void D_ACELP_decode_4t(Word16t index[], Word16t nbbits, Word16t code[]);
void D_ACELP_phase_dispersion(Word16t gain_code, Word16t gain_pit, Word16t code[],
                              Word16t mode, Word16t disp_mem[]);

#endif

