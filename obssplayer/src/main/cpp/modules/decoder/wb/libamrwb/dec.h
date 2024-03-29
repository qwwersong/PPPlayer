/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#ifndef DEC_H
#define DEC_H

#include "typedef.h"

void D_MAIN_reset(void *st, Word16t reset_all);
Word32 D_MAIN_init(void **spd_state);
void D_MAIN_close(void **spd_state);
Word32 D_MAIN_decode(Word16t mode, Word16t prms[], Word16t synth16k[],
                     void *spd_state, UWord8 frame_type);

#endif

