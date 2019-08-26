/*
 *===================================================================
 *  3GPP AMR Wideband Floating-point Speech Codec
 *===================================================================
 */
#include <math.h>
#include <string.h>
//#include <assert.h> // deng

#include "typedef.h"
#include "dec_main.h"
#include "dec_lpc.h"
#include "dec_util.h"


#define L_SUBFR      64       /* Subframe size                    */
#define L_SUBFR16k   80       /* Subframe size at 16kHz           */
#define M16k         20       /* Order of LP filter               */
#define PREEMPH_FAC  22282    /* preemphasis factor (0.68 in Q15) */
#define FAC4         4
#define FAC5         5
#define UP_FAC       20480    /* 5/4 in Q14                       */
#define INV_FAC5     6554     /* 1/5 in Q15                       */
#define NB_COEF_UP   12
#define L_FIR        31
#define MODE_7k      0
#define MODE_24k     8


extern const Word16t D_ROM_pow2[];
extern const Word16t D_ROM_isqrt[];
extern const Word16t D_ROM_log2[];
extern const Word16t D_ROM_fir_up[];
extern const Word16t D_ROM_fir_up_X[];  // deng
extern const Word16t D_ROM_fir_6k_7k[];
extern const Word16t D_ROM_fir_7k[];
extern const Word16t D_ROM_hp_gain[];

#ifdef WIN32
#pragma warning( disable : 4310)
#endif

static int clip_to_sint16(int v)
{
#if  1
	// proper to ARM
	if (v > (short)0x7FFF)
	  v = (short)0x7FFF;
	else if (v < (short)0x8000)
	  v = (short)0x8000;
	return v;
#else
	int u;
	u = v + (1<<15); // note saturate
	if ( u & (~0xFFFF)) {
		u = ((-u) >> 31) & 0xFFFF;
		v = u - (1<<15);
	}
	return v;
#endif
}

Word16t D_UTIL_saturate(Word32 v)
{
#if 1
	return clip_to_sint16(v);
#else
	Word16t out;
	if ((v < MAX_16) & (v > MIN_16))
		out = (Word16t)v;
 	else  {
		if (v > 0)
			out = MAX_16;
		else
			out = MIN_16;
	}
	return(out);
#endif
}


void D_UTIL_l_extract(Word32 L_32, Word16t *hi, Word16t *lo)
{
   *hi = (Word16t)(L_32 >> 16);
   *lo = (Word16t)((L_32 >> 1) - (*hi * 32768));
}
/*
__inline Word32 L_Comp (Word16t hi, Word16t lo)
{
	return ((Word32)hi << 16) + ((Word32)lo) * 2;
}
*/

Word32 D_UTIL_mpy_32(Word16t hi1, Word16t lo1, Word16t hi2, Word16t lo2)
{
    Word32 L_32;
	L_32 = hi1 * hi2;
	L_32 += (hi1 * lo2) >> 15;
	L_32 += (lo1 * hi2) >> 15;
	L_32  <<= 1;
    return (L_32);
}

Word32 D_UTIL_mpy_32_16(Word16t hi, Word16t lo, Word16t n)
{
    Word32 L_32;
	L_32 = hi * n;
	L_32 += (lo * n) >> 15;
	L_32 <<= 1;
	return (L_32);
}

Word16t D_UTIL_random(Word16t *seed)
{
   /// static Word16t seed = 21845;
   *seed = (short)(*seed * 31821L + 13849L); //deng, NOTE HERE: short !!!
   return(*seed);
}

static __inline
Word32 D_UTIL_dot_prod(const Word16t* x, const Word16t *y, int n)
{
   Word32 i;
   Word32 sum;

   sum = 0L;
#if 1
   // faster due to data cache, sequential ?
   for (i = 0; i < n; i++)
   {
      sum +=  (x[i] * y[i]);  // loop unrolling ???
   }
#else
   i = n;
   while ( --i >= 0 ) {
	   sum += x[i] * y[i];
   }
#endif

   return sum;
}

static __inline 
Word32 D_UTIL_dot_prod_rev(const Word16t* x, const Word16t *y, int n)
{
   Word32 i;
   Word32 sum;

   sum = 0L;
   for (i = 0; i < n; i++)
   {
      sum +=  (x[i] * y[-i]);
   }

   return sum;
}

static __inline 
Word32 D_UTIL_interpol_X(const Word16t* fir, const int L,const int MM, const Word16t *x, int frac)
{
   // Polyphase form
   //
#if 0
   Word32 i;
   Word32 sum;

   x   +=  - MM + 1;
   fir +=  ((L - 1) - frac) * (2 * MM);
   sum = 0L;

   for (i = 0; i < 2 * MM; i+= 1)
   {
      sum +=  (x[i]   * fir[i]);
      //sum +=  (x[i+1] * fir[i+1]);
   }
   return sum;
#else
   x   +=  - MM + 1;
   fir +=  ((L - 1) - frac) * (2 * MM);

   return  D_UTIL_dot_prod(x, fir, 2*MM);
#endif
}



/*
 * D_UTIL_pow2
 *
 * Parameters:
 *    exponant    I: (Q0) Integer part.      (range: 0 <= val <= 30)
 *    fraction    I: (Q15) Fractionnal part. (range: 0.0 <= val < 1.0)
 *
 * Function:
 *    L_x = pow(2.0, exponant.fraction)         (exponant = interger part)
 *        = pow(2.0, 0.fraction) << exponant
 *
 *    Algorithm:
 *
 *    The function Pow2(L_x) is approximated by a table and linear
 *    interpolation.
 *
 *    1 - i = bit10 - b15 of fraction,   0 <= i <= 31
 *    2 - a = bit0 - b9   of fraction
 *    3 - L_x = table[i] << 16 - (table[i] - table[i + 1]) * a * 2
 *    4 - L_x = L_x >> (30-exponant)     (with rounding)
 *
 * Returns:
 *    range 0 <= val <= 0x7fffffff
 */
Word32 D_UTIL_pow2(Word16t exponant, Word16t fraction)
{
	Word32 L_x, tmp, i, exp;
	Word16t a;

	L_x = fraction * 32;          /* L_x = fraction<<6             */
	i = L_x >> 15;                /* Extract b10-b16 of fraction   */
	a = (Word16t)(L_x);            /* Extract b0-b9   of fraction   */
	a = (Word16t)(a & (Word16t)0x7fff);
	L_x = D_ROM_pow2[i] << 16;    /* table[i] << 16                */
	tmp = D_ROM_pow2[i] - D_ROM_pow2[i + 1];  /* table[i] - table[i+1] */
	tmp = L_x - ((tmp * a) << 1); /* L_x -= tmp*a*2                */
	exp = 30 - exponant;
	if (exp <= 31) {
		L_x = tmp >> exp;
		if ((1 << (exp - 1)) & tmp)
			L_x++;
	}
	else {
		L_x = 0;
	}

	return(L_x);
}


Word16t D_UTIL_norm_s (Word16t var1)
{
   Word16t var_out;
   if(var1 == 0)
      var_out = 0;
   else {
      if(var1 == -1)
         var_out = 15;
      else {
         if(var1 < 0)
            var1 = (Word16t)~var1;
		 var_out = 0;
		 if ( var1 < 0x80) {
			 var1 <<= 8;
			 var_out += 8;
		 }

		 while( var1 < 0x4000 ) {
            var1 <<= 1;
			var_out ++;
		 }
      }
   }
   return(var_out);	// [0,15]
}

Word16t D_UTIL_norm_l (Word32 L_var1)
{
#if defined(_WIN32_WCE) && defined(_ARM_) 
	int var_out;
	if ( L_var1 == 0)
		var_out = 0; 
	else {
		if(L_var1 == -1)
			var_out = 31;
		else {
			if(L_var1 < 0)
				L_var1 = ~L_var1;
			var_out = _CountLeadingZeros(L_var1) - 1;
		}
	}
	return var_out; // [0,31]
#else
   //Word16t var_out;
   int var_out;
   var_out = 0; 
   if ( L_var1 != 0)
   {
      if(L_var1 == (Word32)0xffffffffL)
         var_out = 31;
      else {
         if(L_var1 < 0)
            L_var1 = ~L_var1;
		 if ( L_var1 < 0x00008000L) {
			 L_var1 <<= 16;
			 var_out = 16;
		 }
		 /*
		 if ( L_var1 < 0x00800000L) {
			 L_var1 <<= 8;
			 var_out += 8;
		 }
		 if ( L_var1 < 0x08000000L) {
			 L_var1 <<= 4;
			 var_out += 4;
		 }
		 */
		 while ( L_var1 < 0x40000000L) {
            L_var1 <<= 1;
			var_out ++;
		 }
      }
   }

   return (var_out); // [0,31]
#endif
}



/*
 * D_UTIL_dot_product12
 *
 * Parameters:
 *    x        I: 12bit x vector
 *    y        I: 12bit y vector
 *    lg       I: vector length
 *    exp      O: exponent of result (0..+30)
 *
 * Function:
 *    Compute scalar product of <x[],y[]> using accumulator.
 *    The result is normalized (in Q31) with exponent (0..30).
 *
 * Returns:
 *    Q31 normalised result (1 < val <= -1)
 */
Word32 D_UTIL_dot_product12(Word16t x[], Word16t y[], Word16t lg, Word16t *exp)
{
   Word32 sum, sft;

#if 0
   int i;
   sum = 0L;
   for(i = 0; i < lg; i++)
   {
      sum += x[i] * y[i];
   }
#else
   sum = D_UTIL_dot_prod( x, y, lg);
#endif

   sum = (sum << 1) + 1;

   /* Normalize acc in Q31 */
   sft = D_UTIL_norm_l(sum);
   sum = sum << sft;
   *exp = (Word16t)(30 - sft);   /* exponent = 0..30 */

   return(sum);
}


/*
 * D_UTIL_normalised_inverse_sqrt
 *
 * Parameters:
 *    frac     I/O: (Q31) normalized value (1.0 < frac <= 0.5)
 *    exp      I/O: exponent (value = frac x 2^exponent)
 *
 * Function:
 *    Compute 1/sqrt(value).
 *    If value is negative or zero, result is 1 (frac=7fffffff, exp=0).
 *
 *    The function 1/sqrt(value) is approximated by a table and linear
 *    interpolation.
 *    1. If exponant is odd then shift fraction right once.
 *    2. exponant = -((exponant - 1) >> 1)
 *    3. i = bit25 - b30 of fraction, 16 <= i <= 63 ->because of normalization.
 *    4. a = bit10 - b24
 *    5. i -= 16
 *    6. fraction = table[i]<<16 - (table[i] - table[i+1]) * a * 2
 *
 * Returns:
 *    void
 */
void D_UTIL_normalised_inverse_sqrt(Word32 *frac, Word16t *exp)
{
   Word32 i, tmp;
   Word16t a;

   if(*frac <= (Word32)0)
   {
      *exp = 0;
      *frac = 0x7fffffffL;
      return;
   }

   if((*exp & 0x1) == 1)   /* If exponant odd -> shift right */
   {
      *frac = *frac >> 1;
   }
   *exp = (Word16t)(-((*exp - 1) >> 1));
   *frac = *frac >> 9;
   i = *frac >>16;      /* Extract b25-b31   */
   *frac = *frac >> 1;
   a = (Word16t)(*frac); /* Extract b10-b24   */
   a = (Word16t)(a & (Word16t)0x7fff);
   i = i - 16;
   *frac = D_ROM_isqrt[i] << 16; /* table[i] << 16    */
   tmp = D_ROM_isqrt[i] - D_ROM_isqrt[i + 1];   /* table[i] - table[i+1]) */
   *frac = *frac - ((tmp * a) << 1);   /* frac -=  tmp*a*2  */

   return;
}


/*
 * D_UTIL_inverse_sqrt
 *
 * Parameters:
 *    L_x     I/O: (Q0) input value (range: 0<=val<=7fffffff)
 *
 * Function:
 *    Compute 1/sqrt(L_x).
 *    If value is negative or zero, result is 1 (7fffffff).
 *
 *    The function 1/sqrt(value) is approximated by a table and linear
 *    interpolation.
 *    1. Normalization of L_x
 *    2. call Normalised_Inverse_sqrt(L_x, exponant)
 *    3. L_y = L_x << exponant
 *
 * Returns:
 *    (Q31) output value (range: 0 <= val < 1)
 */
Word32 D_UTIL_inverse_sqrt(Word32 L_x)
{
   Word32 L_y;
   Word16t exp;

   exp = D_UTIL_norm_l(L_x);
   L_x = (L_x << exp);   /* L_x is normalized */
   exp = (Word16t)(31 - exp);
   D_UTIL_normalised_inverse_sqrt(&L_x, &exp);

   if(exp < 0)
      L_y = (L_x >> -exp);   /* denormalization   */
   else
      L_y = (L_x << exp);   /* denormalization   */

   return(L_y);
}


/*
 * D_UTIL_normalised_log2
 *
 * Parameters:
 *    L_x      I: input value (normalized)
 *    exp      I: norm_l (L_x)
 *    exponent O: Integer part of Log2.   (range: 0<=val<=30)
 *    fraction O: Fractional part of Log2. (range: 0<=val<1)
 *
 * Function:
 *    Computes log2(L_x, exp),  where   L_x is positive and
 *    normalized, and exp is the normalisation exponent
 *    If L_x is negative or zero, the result is 0.
 *
 *    The function Log2(L_x) is approximated by a table and linear
 *    interpolation. The following steps are used to compute Log2(L_x)
 *
 *    1. exponent = 30 - norm_exponent
 *    2. i = bit25 - b31 of L_x;  32 <= i <= 63  (because of normalization).
 *    3. a = bit10 - b24
 *    4. i -= 32
 *    5. fraction = table[i] << 16 - (table[i] - table[i + 1]) * a * 2
 *
 *
 * Returns:
 *    void
 */
static void D_UTIL_normalised_log2(Word32 L_x, Word16t exp, Word16t *exponent,
                                   Word16t *fraction)
{
   Word32 i, a, tmp;
   Word32 L_y;

   if (L_x <= 0)
   {
      *exponent = 0;
      *fraction = 0;
      return;
   }

   *exponent = (Word16t)(30 - exp);

   L_x = L_x >> 10;
   i = L_x >> 15;         /* Extract b25-b31               */
   a = L_x;               /* Extract b10-b24 of fraction   */
   a = a & 0x00007fff;
   i = i - 32;
   L_y = D_ROM_log2[i] << 16;               /* table[i] << 16        */
   tmp = D_ROM_log2[i] - D_ROM_log2[i + 1]; /* table[i] - table[i+1] */
   L_y = L_y - ((tmp * a) << 1);            /* L_y -= tmp*a*2        */
   *fraction = (Word16t)(L_y >> 16);

   return;
}


/*
 * D_UTIL_log2
 *
 * Parameters:
 *    L_x      I: input value
 *    exponent O: Integer part of Log2.   (range: 0<=val<=30)
 *    fraction O: Fractional part of Log2. (range: 0<=val<1)
 *
 * Function:
 *    Computes log2(L_x),  where   L_x is positive.
 *    If L_x is negative or zero, the result is 0.
 *
 * Returns:
 *    void
 */
void D_UTIL_log2(Word32 L_x, Word16t *exponent, Word16t *fraction)
{
   Word16t exp;

   exp = D_UTIL_norm_l(L_x);
   D_UTIL_normalised_log2((L_x <<exp), exp, exponent, fraction);
}

void D_UTIL_signal_up_scale(Word16t x[], Word16t lg, Word16t exp)
{
    Word32 i;
	//assert( (lg & 1) == 0);// lg == 0x138
   for (i = 0; i < lg; i += 2)
    {
		Word32 tmp0,tmp1;
		tmp0 = x[i  ] << exp;
		tmp1 = x[i+1] << exp;
	
        x[i  ] = D_UTIL_saturate(tmp0);
        x[i+1] = D_UTIL_saturate(tmp1);
    }
}


void D_UTIL_signal_down_scale(Word16t x[], Word16t lg, Word16t exp)
{
   Word32 i, tmp;
   // deng, compute: x = (x + (1<<(exp-1))) >> exp;
   //assert( exp >= 0);
   if ( exp == 0)
	   return;
   //if ( exp > 0) 
   {
	   tmp = 1 << (exp-1);
	   for(i = 0; i < lg; i++)
	   {
		  x[i] = (Word16t)((x[i] + tmp) >> exp);
	   }
   }
}


/*
 * D_UTIL_deemph_32
 *
 * Parameters:
 *    x_hi           I: input signal (bit31..16)
 *    x_lo           I: input signal (bit15..4)
 *    y              O: output signal (x16)
 *    mu             I: (Q15) deemphasis factor
 *    L              I: vector size
 *    mem          I/O: memory (y[-1])
 *
 * Function:
 *    Filtering through 1/(1-mu z^-1)
 *
 * Returns:
 *    void
 */
static void D_UTIL_deemph_32(Word16t x_hi[], Word16t x_lo[], Word16t y[],
                             Word16t mu, Word16t L, Word16t *mem)
{
   Word32 i, fac;
   Word32 tmp;

   fac = mu >> 1;   /* Q15 --> Q14 */

   /* L_tmp = hi<<16 + lo<<4 */
   tmp = (x_hi[0] << 12) + x_lo[0];
   tmp = (tmp << 6) + (*mem * fac);
   tmp = (tmp + 0x2000) >> 14;
   y[0] = D_UTIL_saturate(tmp);

   for(i = 1; i < L; i++)
   {
      tmp = (x_hi[i] << 12) + x_lo[i];
      tmp = (tmp << 6) + (y[i - 1] * fac);
      tmp = (tmp + 0x2000) >> 14;
      y[i] = D_UTIL_saturate(tmp);
   }

   *mem = y[L - 1];

   return;
}


/*
 * D_UTIL_synthesis_32
 *
 * Parameters:
 *    a              I: LP filter coefficients
 *    m              I: order of LP filter
 *    exc            I: excitation
 *    Qnew           I: exc scaling = 0(min) to 8(max)
 *    sig_hi         O: synthesis high
 *    sig_lo         O: synthesis low
 *    lg             I: size of filtering
 *
 * Function:
 *    Perform the synthesis filtering 1/A(z).
 *
 * Returns:
 *    void
 */

static void D_UTIL_synthesis_32(Word16t a[], Word16t m, Word16t exc[],
                                Word16t Qnew, Word16t sig_hi[], Word16t sig_lo[],
                                Word16t lg)
{
   Word32 i, a0, s;
   Word32 tmp, tmp2;
   int j;

   // See if a[0] is scaled 
   s = D_UTIL_norm_s((Word16t)a[0]) - 2;

   a0 = a[0] >> (4 + Qnew);   // input / 16 and >>Qnew 

   //adjust pointers
   a ++;
   sig_lo --;
   sig_hi --;

   // Do the filtering. 
   for(i = 0; i < lg; i++)
   {
      tmp  = 0;
      tmp2 = 0;
      for(j = 0; j < m; j++)
      {
         tmp  += a[j] * sig_lo[-j];  //  mla instruction
         tmp2 += a[j] * sig_hi[-j];
      }

	  tmp  = -tmp;
	  tmp2 = exc[0] * a0 - tmp2;

      tmp  = ((tmp >> (15 - 4)) + (tmp2 << 1)) << s;

      sig_hi[1] = (Word16t)(tmp >> 13);      // sig_hi = bit16 to bit31 of synthesis 
      sig_lo[1] = (Word16t)((tmp >> 1) - (sig_hi[1] * 4096));  // sig_lo = bit4 to bit15 of synthesis 

  	  // increment pointers
	  sig_hi ++;
	  sig_lo ++;
	  exc    ++;
   }

   return;
}

/*
 * D_UTIL_hp50_12k8
 *
 * Parameters:
 *    signal       I/O: signal
 *    lg             I: lenght of signal
 *    mem          I/O: filter memory [6]
 *
 * Function:
 *    2nd order high pass filter with cut off frequency at 50 Hz.
 *
 *    Algorithm:
 *
 *    y[i] = b[0]*x[i] + b[1]*x[i-1] + b[2]*x[i-2]
 *                     + a[1]*y[i-1] + a[2]*y[i-2];
 *
 *    b[3] = {0.989501953f, -1.979003906f, 0.989501953f};
 *    a[3] = {1.000000000F,  1.978881836f,-0.966308594f};
 *
 *
 * Returns:
 *    void
 */
static void D_UTIL_hp50_12k8(Word16t signal[], Word16t lg, Word16t mem[])
{
   static const Word16t coefx[6] = {
	16211, -8021, 32422, -16042, 8106, -16212
   };  

   Word32 i, L_tmp;
   Word16t y2_hi, y2_lo, y1_hi, y1_lo, x0, x1, x2;

   y2_hi = mem[0];
   y2_lo = mem[1];
   y1_hi = mem[2];
   y1_lo = mem[3];
   x0 = mem[4];
   x1 = mem[5];

   for(i = 0; i < lg; i++)
   {
      x2 = x1;
      x1 = x0;
      x0 = signal[i];

      // y[i] = b[0]*x[i] + b[1]*x[i-1] + b140[2]*x[i-2] + a[1]*y[i-1] + a[2] * y[i-2];  
      L_tmp = 8192L;   // rounding to maximise precision 

	  L_tmp = L_tmp + (y1_lo * coefx[0]);
      L_tmp = L_tmp + (y2_lo * coefx[1]);
      L_tmp = L_tmp >> 14;
      L_tmp = L_tmp + (y1_hi * coefx[2]);
      L_tmp = L_tmp + (y2_hi * coefx[3]);
      L_tmp = L_tmp + ((x0+x2) * coefx[4]);
      L_tmp = L_tmp + (x1    * coefx[5]);

      L_tmp = L_tmp << 2;  // coeff Q11 --> Q14 
      y2_hi = y1_hi;
      y2_lo = y1_lo;
      D_UTIL_l_extract(L_tmp, &y1_hi, &y1_lo);
      L_tmp = (L_tmp + 0x4000) >> 15;   // coeff Q14 --> Q15 with saturation 
      signal[i] = D_UTIL_saturate(L_tmp);

   }
   mem[0] = y2_hi;
   mem[1] = y2_lo;
   mem[2] = y1_hi;
   mem[3] = y1_lo;
   mem[4] = x0;
   mem[5] = x1;

   return;
}

__inline static Word32 limitx( Word32 v)
{
#if 1
	// ORG
   if ((v < 536846336) & (v > -536879104))
      v = (v + 0x2000) >> 14;
   else if( v >= 536846336) // bug , was: v > 536846336
      v = 32767;
   else
      v = -32768;
#else
	// more proper constant for ARM ??
	//(w,s) = (16,14) 
	//range [-536879104, 536862720) 
	//range [0xDFFFE000L, 0x1FFFE000L) 
	if( v >= (Word32)0x1FFFE000L)
      v = 32767;
    else if ( v < (Word32)0xDFFFE000L )
      v = -32768;
    else
      v = (v + 0x2000) >> 14;
#endif
   return v;
}

/*
 * D_UTIL_interpol
 *
 * Parameters:
 *    x           I: input vector
 *    fir         I: filter coefficient
 *    frac        I: fraction (0..resol)
 *    up_samp     I: resolution
 *    nb_coef     I: number of coefficients
 *
 * Function:
 *    Fractional interpolation of signal at position (frac/up_samp)
 *
 * Returns:
 *    result of interpolation
 */
/* ORG
Word32 D_UTIL_interpol(Word16t *x, const Word16t *fir, Word16t frac,
                       Word16t resol, Word16t nb_coef)
{
   Word32 i, k;
   Word32 sum;
   x   = x - nb_coef + 1;
   sum = 0L;
   for(i = 0, k = ((resol - 1) - frac); i < 2 * nb_coef; i++,  k += resol )
   {
      sum +=  (x[i] * fir[k]);
   }
   return sum;
}
*/


/*
 * D_UTIL_up_samp
 *
 * Parameters:
 *    res_d          I: signal to upsampling
 *    res_u          O: upsampled output
 *    L_frame        I: length of output
 *
 * Function:
 *    Upsampling
 *
 * Returns:
 *    void
 */
static void D_UTIL_up_samp(Word16t *sig_d, Word16t *sig_u, Word16t L_frame)
{
   Word32 pos, i, j;
   Word16t frac;

   Word32 sum;


   pos = 0;   /* position with 1/5 resolution */

   for(j = 0; j < L_frame; j++)
   {
      i = (pos * INV_FAC5) >> 15;   /* integer part = pos * 1/5 */
      frac = (Word16t)(pos - ((i << 2) + i));   /* frac = pos - (pos/5)*5   */
     
	  // deng
      sum = D_UTIL_interpol_X(D_ROM_fir_up_X, FAC5, NB_COEF_UP, &sig_d[i], frac);

	  sig_u[j] = (Word16t)limitx(sum);   /* saturation can occur here */

      pos = pos + FAC4;   /* position + 4/5 */
   }

   return;
}



/*
 * D_UTIL_oversamp_16k
 *
 * Parameters:
 *    sig12k8        I: signal to oversampling
 *    lg             I: length of input
 *    sig16k         O: oversampled signal
 *    mem          I/O: memory (2*12)
 *
 * Function:
 *    Oversampling from 12.8kHz to 16kHz
 *
 * Returns:
 *    void
 */
static void D_UTIL_oversamp_16k(Word16t sig12k8[], Word16t lg, Word16t sig16k[],
                                Word16t mem[])
{
   Word16t lg_up;
   Word16t signal[L_SUBFR + (2 * NB_COEF_UP)];

   memcpy(signal, mem, (2 * NB_COEF_UP) * sizeof(Word16t));
   memcpy(signal + (2 * NB_COEF_UP), sig12k8, lg * sizeof(Word16t));
   lg_up = (Word16t)(((lg * UP_FAC) >> 15) << 1);
   D_UTIL_up_samp(signal + NB_COEF_UP, sig16k, lg_up);
   memcpy(mem, signal + lg, (2 * NB_COEF_UP) * sizeof(Word16t));

   return;
}


/*
 * D_UTIL_hp400_12k8
 *
 * Parameters:
 *    signal       I/O: signal
 *    lg             I: lenght of signal
 *    mem          I/O: filter memory [6]
 *
 * Function:
 *    2nd order high pass filter with cut off frequency at 400 Hz.
 *
 *    Algorithm:
 *
 *    y[i] = b[0]*x[i] + b[1]*x[i-1] + b[2]*x[i-2]
 *                     + a[1]*y[i-1] + a[2]*y[i-2];
 *
 *    b[3] = {0.893554687, -1.787109375,  0.893554687};
 *    a[3] = {1.000000000,  1.787109375, -0.864257812};
 *
 *
 * Returns:
 *    void
 */
void D_UTIL_hp400_12k8(Word16t signal[], Word16t lg, Word16t mem[])
{

   Word32 i, L_tmp;
   Word16t y2_hi, y2_lo, y1_hi, y1_lo, x0, x1, x2;

   y2_hi = mem[0];
   y2_lo = mem[1];
   y1_hi = mem[2];
   y1_lo = mem[3];
   x0 = mem[4];
   x1 = mem[5];

   for(i = 0; i < lg; i++)
   {
      x2 = x1;
      x1 = x0;
      x0 = signal[i];

      /* y[i] = b[0]*x[i] + b[1]*x[i-1] + b140[2]*x[i-2]  */
      /* + a[1]*y[i-1] + a[2] * y[i-2];  */
      L_tmp = 8192L + (y1_lo * 29280);
      L_tmp = L_tmp + (y2_lo * (-14160));
      L_tmp = (L_tmp >> 14);
      L_tmp = L_tmp + (y1_hi * 58560);
      L_tmp = L_tmp + (y2_hi * (-28320));
      L_tmp = L_tmp + (x0 * 1830);
      L_tmp = L_tmp + (x1 * (-3660));
      L_tmp = L_tmp + (x2 * 1830);
      L_tmp = (L_tmp << 1);   /* coeff Q12 --> Q13 */
      y2_hi = y1_hi;
      y2_lo = y1_lo;
      D_UTIL_l_extract(L_tmp, &y1_hi, &y1_lo);

      /* signal is divided by 16 to avoid overflow in energy computation */
      signal[i] = (Word16t)((L_tmp + 0x8000) >> 16);
   }
   mem[0] = y2_hi;
   mem[1] = y2_lo;
   mem[2] = y1_hi;
   mem[3] = y1_lo;
   mem[4] = x0;
   mem[5] = x1;

   return;
}


/*
 * D_UTIL_synthesis
 *
 * Parameters:
 *    a              I: LP filter coefficients
 *    m              I: order of LP filter
 *    x              I: input signal
 *    y              O: output signal
 *    lg             I: size of filtering
 *    mem          I/O: initial filter states
 *    update_m       I: update memory flag
 *
 * Function:
 *    Perform the synthesis filtering 1/A(z).
 *
 * Returns:
 *    void
 */


static void D_UTIL_synthesis(Word16t a[], Word16t m, Word16t x[], Word16t y[],
                             Word16t lg, Word16t mem[], Word16t update)
{
   Word32 i, tmp, s;
   Word16t y_buf[L_SUBFR16k + M16k], a0;
   Word16t *yy;
   int j;

   yy = &y_buf[m];

   // See if a[0] is scaled 
   s = D_UTIL_norm_s(a[0]) - 2;
   // copy initial filter states into synthesis buffer 
   memcpy(y_buf, mem, m * sizeof(Word16t));

   a0 = (Word16t)(a[0] >> 1);   // input / 2 
      
   //adjust pointers
   a ++;
   yy --;
   y --;

   // Do the filtering. 
   for (i = 0; i < lg; i++)
   {
	   // much faster !!
      tmp = 0;
      for (j = 0; j < m; j++)
      {
         tmp += a[j] * yy[-j];
      }
	  tmp = x[0] * a0 - tmp;
      tmp <<= s;
      y[1] = yy[1] = (Word16t)((tmp + 0x800) >> 12);

  	  // increment pointers
	  x ++;
	  y ++;
	  yy ++;
   }

   // Update memory if required 
   if (update)
   {
      memcpy(mem, &y_buf[lg], m * sizeof(Word16t));
   }

   return;
}


/*
 * D_UTIL_bp_6k_7k
 *
 * Parameters:
 *    signal       I/O: signal
 *    lg             I: lenght of signal
 *    mem          I/O: filter memory [4]
 *
 * Function:
 *    15th order band pass 6kHz to 7kHz FIR filter.
 *
 * Returns:
 *    void
 */
void D_UTIL_bp_6k_7k(Word16t signal[], Word16t lg, Word16t mem[])
{
   Word16t x[L_SUBFR16k + (L_FIR - 1)];
   Word32 i, tmp;

   for(i = 0; i < (L_FIR - 1); i++)
   {
      x[i] = (Word16t)mem[i];   /* gain of filter = 4 */
   }

   for(i = 0; i < lg; i++)
   {
      x[i + L_FIR - 1] = signal[i] >> 2;   /* gain of filter = 4 */
   }

   for(i = 0; i < lg; i++)
   {
#if 0
	  int j;
      tmp = 0;
      for(j = 0; j < L_FIR; j++)
      {
         tmp += x[i + j] * D_ROM_fir_6k_7k[j];
      }
#else
	  tmp = D_UTIL_dot_prod( &x[i], D_ROM_fir_6k_7k, L_FIR);
#endif

      signal[i] = (Word16t)((tmp + 0x4000) >> 15);
   }

   for(i = 0; i < (L_FIR - 1); i++)
   {
      mem[i] = (Word16t)x[lg + i];   /* gain of filter = 4 */
   }

   return;
}


/*
 * D_UTIL_hp_7k
 *
 * Parameters:
 *    signal          I/O: ISF vector
 *    lg                I: length of signal
 *    mem             I/O: memory (30)
 *
 * Function:
 *    15th order high pass 7kHz FIR filter
 *
 * Returns:
 *    void
 */
static void D_UTIL_hp_7k(Word16t signal[], Word16t lg, Word16t mem[])
{

   Word32 i, tmp;
   Word16t x[L_SUBFR16k + (L_FIR - 1)];

   memcpy(x, mem, (L_FIR - 1) * sizeof(Word16t));
   memcpy(&x[L_FIR - 1], signal, lg * sizeof(Word16t));

   for(i = 0; i < lg; i++)
   {
#if 0
	  int j;
      tmp = 0;
      for(j = 0; j < L_FIR; j++)
      {
         tmp += x[i + j] * D_ROM_fir_7k[j];
      }
#else
	  tmp = D_UTIL_dot_prod( &x[i], D_ROM_fir_7k, L_FIR);
#endif
      signal[i] = (Word16t)((tmp + 0x4000) >> 15);
   }

   memcpy(mem, x + lg, (L_FIR - 1) * sizeof(Word16t));

   return;
}


/*
 * D_UTIL_Dec_synthesis
 *
 * Parameters:
 *    Aq             I: quantized Az
 *    exc            I: excitation at 12kHz
 *    Q_new          I: scaling performed on exc
 *    synth16k       O: 16kHz synthesis signal
 *    prms           I: parameters
 *    HfIsf        I/O: High frequency ISF:s
 *    mode           I: codec mode
 *    newDTXState    I: dtx state
 *    bfi            I: bad frame indicator
 *    st           I/O: State structure
 *
 * Function:
 *    Synthesis of signal at 16kHz with HF extension.
 *
 * Returns:
 *    void
 */
void D_UTIL_dec_synthesis(Word16t Aq[], Word16t exc[], Word16t Q_new,
                          Word16t synth16k[], Word16t prms, Word16t HfIsf[],
                          Word16t mode, Word16t newDTXState, Word16t bfi,
                          Decoder_State *st)
{
   Word32 tmp, i;
   Word16t exp;
   Word16t ener, exp_ener;
   Word32 fac;
   Word16t synth_hi[M + L_SUBFR], synth_lo[M + L_SUBFR];
   Word16t synth[L_SUBFR];
   Word16t HF[L_SUBFR16k];   /* High Frequency vector      */
   Word16t Ap[M16k + 1];
   Word16t HfA[M16k + 1];
   Word16t HF_corr_gain;
   Word16t HF_gain_ind;
   Word32 gain1, gain2;
   Word16t weight1, weight2;

   /*
    * Speech synthesis
    *
    * - Find synthesis speech corresponding to exc2[].
    * - Perform fixed deemphasis and hp 50hz filtering.
    * - Oversampling from 12.8kHz to 16kHz.
    */
   memcpy(synth_hi, st->mem_syn_hi, M * sizeof(Word16t));
   memcpy(synth_lo, st->mem_syn_lo, M * sizeof(Word16t));
   D_UTIL_synthesis_32(Aq, M, exc, Q_new, synth_hi + M, synth_lo + M, L_SUBFR);
   memcpy(st->mem_syn_hi, synth_hi + L_SUBFR, M * sizeof(Word16t));
   memcpy(st->mem_syn_lo, synth_lo + L_SUBFR, M * sizeof(Word16t));
   D_UTIL_deemph_32(synth_hi + M, synth_lo + M, synth, PREEMPH_FAC, L_SUBFR,
      &(st->mem_deemph));
   D_UTIL_hp50_12k8(synth, L_SUBFR, st->mem_sig_out);
   D_UTIL_oversamp_16k(synth, L_SUBFR, synth16k, st->mem_oversamp);

   /*
    * HF noise synthesis
    *
    * - Generate HF noise between 5.5 and 7.5 kHz.
    * - Set energy of noise according to synthesis tilt.
    *     tilt > 0.8 ==> - 14 dB (voiced)
    *     tilt   0.5 ==> - 6 dB  (voiced or noise)
    *     tilt < 0.0 ==>   0 dB  (noise)
    */

   /* generate white noise vector */
   for(i = 0; i < L_SUBFR16k; i++)
   {
      HF[i] = (Word16t)(D_UTIL_random(&(st->mem_seed2)) >> 3);
   }

   /* energy of excitation */
   D_UTIL_signal_down_scale(exc, L_SUBFR, 3);
   Q_new = (Word16t)(Q_new - 3);
   ener = (Word16t)(D_UTIL_dot_product12(exc, exc, L_SUBFR, &exp_ener) >> 16);
   exp_ener = (Word16t)(exp_ener - (Q_new << 1));

   /* set energy of white noise to energy of excitation */
   tmp = (Word16t)(D_UTIL_dot_product12(HF, HF, L_SUBFR16k, &exp) >> 16);

   if(tmp > ener)
   {
      tmp = tmp >> 1;   /* Be sure tmp < ener */
      exp = (Word16t)(exp + 1);
   }

   tmp = (tmp << 15) / ener;

   if(tmp > 32767)
   {
      tmp = 32767;
   }

   tmp = tmp << 16;   /* result is normalized */
   exp = (Word16t)(exp - exp_ener);
   D_UTIL_normalised_inverse_sqrt(&tmp, &exp);

   /* L_tmp x 2, L_tmp in Q31 */
   /* tmp = 2 x sqrt(ener_exc/ener_hf) */
   if(exp >= 0)
   {
      tmp = tmp >> (15 - exp);
   }
   else
   {
      tmp = tmp >> (-exp);
      tmp = tmp >> 15;
   }

   /* saturation */
   if(tmp > 0x7FFF)
   {
      tmp = 0x7FFF;
   }

   for(i = 0; i < L_SUBFR16k; i++)
   {
      HF[i] = (Word16t)((HF[i] * tmp) >> 15);
   }

   /* find tilt of synthesis speech (tilt: 1=voiced, -1=unvoiced) */
   D_UTIL_hp400_12k8(synth, L_SUBFR, st->mem_hp400);
   tmp = 0L;

   for(i = 0; i < L_SUBFR; i++)
   {
      tmp = tmp + (synth[i] * synth[i]);
   }

   tmp = (tmp << 1) + 1;
   exp = D_UTIL_norm_l(tmp);
   ener = (Word16t)((tmp << exp) >> 16);   /* ener = r[0] */
   tmp = 0L;

   for(i = 1; i < L_SUBFR; i++)
   {
      tmp = tmp + (synth[i] * synth[i - 1]);
   }

   tmp = (tmp << 1) + 1;
   tmp = (tmp << exp) >> 16;   /* tmp = r[1] */

   if(tmp > 0)
   {
      fac = ((tmp << 15) / ener);

      if(fac > 32767)
      {
         fac = 32767;
      }
   }
   else
   {
      fac = 0;
   }

   /* modify energy of white noise according to synthesis tilt */
   gain1 = (32767 - fac);
   gain2 = ((32767 - fac) * 20480) >> 15;
   gain2 = (gain2 << 1);

   if(gain2 > 32767)
      gain2 = 32767;

   if(st->mem_vad_hist > 0)
   {
      weight1 = 0;
      weight2 = 32767;
   }
   else
   {
      weight1 = 32767;
      weight2 = 0;
   }

   tmp = (weight1 * gain1) >> 15;
   tmp = tmp + ((weight2 * gain2) >> 15);

   if(tmp != 0)
   {
      tmp = tmp + 1;
   }

   if(tmp < 3277)
   {
      tmp = 3277;   /* 0.1 in Q15 */
   }

   if((mode == MODE_24k) & (bfi == 0))
   {
      /* HF correction gain */
      HF_gain_ind = prms;
      HF_corr_gain = D_ROM_hp_gain[HF_gain_ind];

      /* HF gain */
      for(i = 0; i < L_SUBFR16k; i++)
      {
         HF[i] = (Word16t)(((HF[i] * HF_corr_gain) >> 15) << 1);
      }
   }
   else
   {
      for(i = 0; i < L_SUBFR16k; i++)
      {
         HF[i] = (Word16t)((HF[i] * tmp) >> 15);
      }
   }

   if((mode <= MODE_7k) & (newDTXState == SPEECH))
   {
      D_LPC_isf_extrapolation(HfIsf);
      D_LPC_isp_a_conversion(HfIsf, HfA, 0, M16k);
      D_LPC_a_weight(HfA, Ap, 29491, M16k);   /* fac=0.9 */
      D_UTIL_synthesis(Ap, M16k, HF, HF, L_SUBFR16k, st->mem_syn_hf, 1);
   }
   else
   {
      /* synthesis of noise: 4.8kHz..5.6kHz --> 6kHz..7kHz */
      D_LPC_a_weight(Aq, Ap, 19661, M);   /* fac=0.6 */
      D_UTIL_synthesis(Ap, M, HF, HF, L_SUBFR16k, st->mem_syn_hf + (M16k - M), 1);
   }

   /* noise High Pass filtering (1ms of delay) */
   D_UTIL_bp_6k_7k(HF, L_SUBFR16k, st->mem_hf);

   if(mode == MODE_24k)
   {
      /* Low Pass filtering (7 kHz) */
      D_UTIL_hp_7k(HF, L_SUBFR16k, st->mem_hf3);
   }

   /* add filtered HF noise to speech synthesis */
   for(i = 0; i < L_SUBFR16k; i++)
   {
      tmp = (synth16k[i] + HF[i]);
      synth16k[i] = D_UTIL_saturate(tmp);
   }

   return;
}


/*
 * D_UTIL_preemph
 *
 * Parameters:
 *    x            I/O: signal
 *    mu             I: preemphasis factor
 *    lg             I: vector size
 *    mem          I/O: memory (x[-1])
 *
 * Function:
 *    Filtering through 1 - mu z^-1
 *
 *
 * Returns:
 *    void
 */
void D_UTIL_preemph(Word16t x[], Word16t mu, Word16t lg, Word16t *mem)
{
   Word32 i, L_tmp;
   Word16t temp;

   temp = x[lg - 1];

   for(i = lg - 1; i > 0; i--)
   {
      L_tmp = x[i] << 15;
      L_tmp = L_tmp - (x[i - 1] * mu);
      x[i] = (Word16t)((L_tmp + 0x4000) >> 15);
   }

   L_tmp = x[0] << 15;
   L_tmp = L_tmp - (*mem * mu);
   x[0] = (Word16t)((L_tmp + 0x4000) >> 15);
   *mem = temp;

   return;
}

//////////////////////////////////////////////////////////////////

extern const Word16t D_ROM_inter4_2_X[];  // deng
#define UP_SAMP         4
#define L_INTERPOL2     16

void D_GAIN_adaptive_codebook_excitation(Word16t exc[], Word32 T0, Word32 frac)
{
   Word32 j;
   Word32 sum;
   Word16t *x;

   x = &exc[ - T0];
   frac = -(frac);

   if(frac < 0)
   {
      frac = (frac + UP_SAMP);
      x--;
   }

   for(j = 0; j < L_SUBFR + 1; j++)
   {
	   //deng
	  sum = D_UTIL_interpol_X(D_ROM_inter4_2_X, UP_SAMP, L_INTERPOL2, x, frac);

      sum = (sum + 0x2000) >> 14;
      exc[j] = D_UTIL_saturate(sum);

      x++;
   }
   return;
}

