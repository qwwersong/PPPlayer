/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#ifndef DEC_MAIN_H
#define DEC_MAIN_H

#include "typedef.h"
#include "dec_dtx.h"

#define L_FRAME      256      /* Frame size                          */
#define PIT_MAX      231      /* Maximum pitch lag                   */
#define L_INTERPOL   (16 + 1) /* Length of filter for interpolation  */
#define L_MEANBUF    3
#define L_FILT       12       /* Delay of up-sampling filter         */
#define L_FILT16k    15       /* Delay of down-sampling filter       */
#define M16k         20       /* Order of LP filter                  */

typedef struct
{
   Word32 mem_gc_thres;             /* threshold for noise enhancer        */
   Word16t mem_exc[(L_FRAME + 1) + PIT_MAX + L_INTERPOL];/* old excitation vector */
   Word16t mem_isf_buf[L_MEANBUF * M];/* isf buffer(frequency domain)        */
   Word16t mem_hf[2 * L_FILT16k];    /* HF band-pass filter memory          */
   Word16t mem_hf2[2 * L_FILT16k];   /* HF band-pass filter memory          */
   Word16t mem_hf3[2 * L_FILT16k];   /* HF band-pass filter memory          */
   Word16t mem_oversamp[2 * L_FILT]; /* synthesis oversampled filter memory */
   Word16t mem_gain[23];             /* gain decoder memory                 */
   Word16t mem_syn_hf[M16k];         /* HF synthesis memory                 */
   Word16t mem_isp[M];               /* old isp (immittance spectral pairs) */
   Word16t mem_isf[M];               /* old isf (frequency domain)          */
   Word16t mem_isf_q[M];             /* past isf quantizer                  */
   Word16t mem_syn_hi[M];            /* modified synthesis memory (MSB)     */
   Word16t mem_syn_lo[M];            /* modified synthesis memory (LSB)     */
   Word16t mem_ph_disp[8];           /* phase dispersion memory             */
   Word16t mem_sig_out[6];           /* hp50 filter memory for synthesis    */
   Word16t mem_hp400[6];             /* hp400 filter memory for synthesis   */
   Word16t mem_lag[5];               /* LTP lag history                     */
   Word16t mem_subfr_q[4];           /* old maximum scaling factor          */
   Word16t mem_tilt_code;            /* tilt of code                        */
   Word16t mem_q;                    /* old scaling factor                  */
   Word16t mem_deemph;               /* speech deemph filter memory         */
   Word16t mem_seed;                 /* random memory for frame erasure     */
   Word16t mem_seed2;                /* random memory for HF generation     */
   Word16t mem_seed3;                /* random memory for lag concealment   */
   Word16t mem_T0;                   /* old pitch lag                       */
   Word16t mem_T0_frac;              /* old pitch fraction lag              */
   /*was: UWord16*/ Word16t mem_vad_hist; /* VAD history                         */
   D_DTX_State *dtx_decSt;
   UWord8 mem_bfi;                  /* Previous BFI                        */
   UWord8 mem_state;                /* BGH state machine memory            */
   UWord8 mem_first_frame;          /* First frame indicator               */

} Decoder_State;

#endif

