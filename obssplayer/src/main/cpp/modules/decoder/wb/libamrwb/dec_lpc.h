/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#ifndef DEC_LPC_H
#define DEC_LPC_H

#include "typedef.h"

void D_LPC_isf_noise_d(Word16t *indice, Word16t *isf_q);
void D_LPC_isf_isp_conversion(Word16t isf[], Word16t isp[], Word16t m);
void D_LPC_isp_a_conversion(Word16t isp[], Word16t a[], Word32 adaptive_scaling, 
                            Word16t m);
void D_LPC_a_weight(Word16t a[], Word16t ap[], Word16t gamma, Word16t m);
void D_LPC_isf_2s3s_decode(Word16t *indice, Word16t *isf_q, Word16t* past_isfq,
                           Word16t *isfold, Word16t *isf_buf, Word16t bfi);
void D_LPC_isf_2s5s_decode(Word16t *indice, Word16t *isf_q, Word16t *past_isfq,
                           Word16t *isfold, Word16t *isf_buf, Word16t bfi);
void D_LPC_int_isp_find(Word16t isp_old[], Word16t isp_new[],
                        const Word16t frac[], Word16t Az[]);
void D_LPC_isf_extrapolation(Word16t HfIsf[]);

#endif

