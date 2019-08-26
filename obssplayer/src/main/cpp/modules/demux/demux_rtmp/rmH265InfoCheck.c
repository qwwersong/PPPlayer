#include "../../decoder/H265/rmComType.h"

/********************************* Frame Type Check *****************************************************/
#define STARTCODE1_CHECK(p)  (!(*((p)+0)) && !(*((p)+1)) && (*((p)+2)==1))               //00 00 01
#define STARTCODE2_CHECK(p)  (!(*((p)+0)) && !(*((p)+1)) && !(*((p)+2))&& (*((p)+3)==1))  //00 00 00 01

typedef struct  
{
	RM_S32   pic_nal_len;
	RM_U8    *nal_start_pos;
	RM_S32	 max_emul_3;
	RM_S32	 emu_offsets[1024];
}rmH265Dec;

static const RM_U8 RM_VLC_LEN[512]={
	14,13,12,12,11,11,11,11,10,10,10,10,10,10,10,10,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

static const RM_U8 RM_UE_VAL[512]={
	31,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
	7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

typedef struct {
	RM_U32 cache_a;
	RM_U32 cache_b;
	RM_S32 bit_pos;
	RM_S32 first_byte;
	const RM_U8 *p_bit_start;
	const RM_U8 *p_bit_ptr;
	const RM_U8 *p_bit_end;
}BIT_STREAM;

typedef enum
{
	B_SLICE,
	P_SLICE,
	I_SLICE,
	B_SLICE_SKIP,        //the picture is a sub-layer non-reference picture 
	SLICE_NULL
} SLICE_TYPE;

/* bit stream read */
#define BYTESWAP(a) ((a<<24) | ((a<<8)&0x00ff0000) | ((a>>8)&0x0000ff00) | (a>>24));
#define UPDATE_CACHE(p_bs)	\
{\
	RM_U32 tmp;\
	if (p_bs->bit_pos < 0){\
	RM_S32 nleft = p_bs->p_bit_end - p_bs->p_bit_ptr - 8;\
	if (nleft >= 4)\
{\
	tmp = *(RM_U32*)p_bs->p_bit_ptr;\
	tmp = BYTESWAP(tmp);\
	p_bs->cache_b = (tmp << (-p_bs->bit_pos));\
	p_bs->bit_pos += 32;\
	p_bs->cache_a |= (tmp >> p_bs->bit_pos);\
	p_bs->p_bit_ptr += 4;\
	}\
		else if(nleft > 0)\
{\
	RM_S32 shift = 32 - (nleft<<3);\
	tmp = (*p_bs->p_bit_ptr++);\
	while (--nleft > 0)\
{\
	tmp = (tmp << 8) | (*p_bs->p_bit_ptr++);\
	}\
	tmp <<= shift;\
	p_bs->cache_b = (tmp << (-p_bs->bit_pos));\
	p_bs->bit_pos += 32;\
	p_bs->cache_a |= (tmp >> p_bs->bit_pos);\
	p_bs->p_bit_ptr += (shift>>3);\
	}\
		else{\
		p_bs->bit_pos += 32;\
		p_bs->p_bit_ptr += 4;\
	}\
	}\
}

#define UPDATE_CACHE_LARGE(p_bs) \
{\
	RM_U32 tmp;\
	while (p_bs->bit_pos <= -64)\
	{\
	p_bs->p_bit_ptr += 4;\
	p_bs->bit_pos += 32;\
	}\
	if (p_bs->bit_pos <= -32)\
	{\
	tmp = *(RM_U32*)p_bs->p_bit_ptr;\
	tmp = BYTESWAP(tmp);\
	p_bs->bit_pos += 32;\
	p_bs->cache_a = (tmp << (-p_bs->bit_pos));\
	tmp = *(RM_U32*)(p_bs->p_bit_ptr+4);\
	tmp = BYTESWAP(tmp);\
	p_bs->cache_b = (tmp << (-p_bs->bit_pos));\
	p_bs->bit_pos += 32;\
	p_bs->cache_a |= (tmp >> p_bs->bit_pos);\
	p_bs->p_bit_ptr += 8;\
	}\
	else if (p_bs->bit_pos < 0)\
	{\
	tmp = *(RM_U32*)p_bs->p_bit_ptr;\
	tmp = BYTESWAP(tmp);\
	p_bs->cache_b = (tmp << (-p_bs->bit_pos));\
	p_bs->bit_pos += 32;\
	p_bs->cache_a |= (tmp >> p_bs->bit_pos);\
	p_bs->p_bit_ptr += 4;\
	}\
}

#define SHOW_BITS(p_bs,n) ((RM_U32)(p_bs->cache_a) >> (32-(n)))
#define FLUSH_BITS(p_bs,n)	\
{\
	RM_S32 nTmp = n;	\
	p_bs->bit_pos -= nTmp;\
	p_bs->cache_a   = (p_bs->cache_a << nTmp)|(p_bs->cache_b >> (32-nTmp));\
	p_bs->cache_b <<= nTmp;\
	}

static __inline RM_S32 GetBits(BIT_STREAM* p_bs,RM_S32 n)
{
	RM_S32 i = SHOW_BITS(p_bs,n);
	FLUSH_BITS(p_bs,n);
	return i;
}

RM_S32 ReadUV(BIT_STREAM *pBits, RM_S32 LenInBits)
{
	RM_S32 inf;
	inf = GetBits(pBits,LenInBits);
	UPDATE_CACHE(pBits);
	return inf;
}

static const RM_U8 RM_BIG_LEN_TAB[256]={
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static __inline RM_S32 big_len( RM_U32 val )
{
	RM_S32 len = 0;
	if (val & 0xffff0000) {
		val >>= 16;
		len += 16;
	}
	if (val & 0xff00) {
		val >>= 8;
		len += 8;
	}
	len += RM_BIG_LEN_TAB[val];
	return len;
}

#define FLUSH_BITS_LARGE(p_bs,n)	\
{\
	RM_S32 nTmp = n;	\
	p_bs->bit_pos -= nTmp;\
	if (nTmp < 32) \
{\
	p_bs->cache_a   = (p_bs->cache_a << nTmp)|(p_bs->cache_b >> (32-nTmp));\
	p_bs->cache_b <<= nTmp;\
}\
	else\
{\
	p_bs->cache_a = p_bs->cache_b << (nTmp & 31);\
	p_bs->cache_b = 0;\
	UPDATE_CACHE_LARGE(p_bs);\
}\
}

RM_S32 ReadUEV(BIT_STREAM *pBits)
{
	RM_U32 value;
	RM_S32 len;
	value = SHOW_BITS(pBits,32);
	if (value >= (1 << 27)) {
		value >>= 32-9;
		FLUSH_BITS(pBits,RM_VLC_LEN[value]);
		UPDATE_CACHE(pBits);
		return RM_UE_VAL[value];
	} else {
		len = 2*big_len(value)-31;
		value >>= len;
		value--;
		FLUSH_BITS_LARGE(pBits,32-len);
		UPDATE_CACHE(pBits);
		return value;
	}
}

static __inline RM_BOOL ReadOneBit(BIT_STREAM* p_bs)
{
	RM_BOOL i = (RM_BOOL)SHOW_BITS(p_bs, 1);
	FLUSH_BITS(p_bs,1);
	return i;
}

static RM_U8* GetNextNal(RM_U8* pCurr,RM_U32* size)
{
	RM_U8 *p = pCurr;  
	RM_U8 *endPos = pCurr+(*size)-8;
	for (; p < endPos; p++)
	{
		if (STARTCODE2_CHECK(p))
		{
			p+=4;
			break;
		}
		else if (STARTCODE1_CHECK(p))
		{
			p+=3;
			break;
		}
	}
	if(p >= endPos)
		p = NULL;
	else
		*size -= (p - pCurr);
	return p;
}

RM_S32 ebsp2rbsp(rmH265Dec* p_dec, RM_U8 *p_inbuf, RM_S32 inbuf_len, 
				 RM_S32 *used_len, RM_S32 *p_nalu_start_pos)
{
	RM_BOOL first_sc = RM_FALSE;
	RM_S32 i,  j, nalu_start_pos = 0;
	/*ebsp to rbsp*/
	//find 0x00 00 03 or 0x00 00 1(2)
	for (i = 0; i <= inbuf_len-2 ;) {
		if (p_inbuf[i + 2] > 0x3) {//check [2]
			i += 3;
			continue;
		} else if (p_inbuf[i] == 0 && p_inbuf [i + 1] == 0) {//check [0] and [1]
			if (p_inbuf[i + 2] == 0x3){ //escape
				//escape = RM_TRUE;
				break;
			}else if (p_inbuf[i + 2] > 0) {//next start code
				if (first_sc == RM_FALSE) {//find first start code
					i += 3;
					nalu_start_pos = i;
					first_sc = RM_TRUE;
					continue;
				}
				*used_len = i;
				goto FIND_NALU;//break;//find 2rd sc,== 
			}
		}
		i++;
	}

	if (first_sc == RM_FALSE) {// no nalu
		return RM_RETURN_INPUT_NO_ENOUGH;
	}
	//process escape and left buffer
	#if 0
	p_dec->max_emul_3 = 0;
	for (j = i; j <= inbuf_len-2 ;) {
		if (p_inbuf[j + 2] > 0x3) {// find 0x 00 00 
			p_inbuf[i++] = p_inbuf[j++];
			p_inbuf[i++] = p_inbuf[j++];
		} else if (p_inbuf[j] == 0 && p_inbuf [j + 1] == 0) {
			if (p_inbuf[j + 2] == 0x3){ //escape
				p_inbuf[i++] = 0;
				p_inbuf[i++] = 0;
				p_dec->emu_offsets[p_dec->max_emul_3++] = i - nalu_start_pos - 2;
				j += 3;				
				continue;
			}else if (p_inbuf[j + 2] > 0) {//next start code
				break;
			}
		}

		p_inbuf[i++] = p_inbuf[j++];
	}
	if (j + 2 > inbuf_len) {
		while (j < inbuf_len) {
			p_inbuf[i++] = p_inbuf[j++];
		}
	}
	*used_len = j;
	#endif
FIND_NALU:
	p_dec->pic_nal_len = i - nalu_start_pos - 2;
	p_dec->nal_start_pos = p_inbuf + nalu_start_pos + 2;

	*p_nalu_start_pos = nalu_start_pos;

	return RM_RETURN_OK;
}

static __inline void InitBitStream(BIT_STREAM* p_bs, const RM_U8 *stream, RM_S32 len)
{
	RM_U32 tmp;
	RM_S32 align4, i, shif_bit;

	p_bs->p_bit_end = stream + len + 8;	
	p_bs->p_bit_start = stream;
	align4 = (RM_S32)(4 - (((unsigned long)stream) & 3));
	shif_bit = 24;

	p_bs->cache_a = 0;
	for (i = 0; i < (align4 < len ? align4 : len); i++){
		p_bs->cache_a |= (stream[i]<<shif_bit);
		shif_bit     -= 8;
	}

	stream += align4;
	p_bs->p_bit_ptr = stream + 4;
	if (align4+4 <= len)
	{
		tmp = *((RM_U32*)stream);
		p_bs->cache_b = BYTESWAP(tmp);
	}
	else if (align4 >= len)
	{
		p_bs->cache_b = 0;
	}
	else
	{
		len -= align4;
		shif_bit = 24;
		p_bs->cache_b = 0;
		for (i = 0; i < len; i++){
			p_bs->cache_b |= (stream[i]<<shif_bit);
			shif_bit     -= 8;
		}
	}
	if (4 == align4) {
		p_bs->bit_pos   = 32;
	} else {
		p_bs->cache_a |= (p_bs->cache_b >> (align4*8));
		p_bs->cache_b<<= (4 -align4)*8;
		p_bs->bit_pos   = 32 - (4 - align4)*8;
	}
}

static SLICE_TYPE SliceTypeParser(RM_PBYTE pData, RM_U32 m_Lens)
{
	RM_S32 used_len;
	rmH265Dec pDec;
	RM_S32 ret = RM_RETURN_OK;
	RM_S32 nalu_start_pos;
	RM_U32 nal_unit_type;
	RM_PBYTE pIn = pData;
	BIT_STREAM p_bs;
	RM_U32 first_slice_in_pic_flag;
	RM_U32 new_PPS_ID;
	RM_S32 slice_type;

	ret = ebsp2rbsp(&pDec, pIn, m_Lens, &used_len, &nalu_start_pos);
	nal_unit_type  = (pIn[nalu_start_pos + 0] >> 1) & 0x3f;
	InitBitStream(&p_bs, pDec.nal_start_pos, pDec.pic_nal_len);
	first_slice_in_pic_flag = ReadOneBit( &p_bs);

	if (nal_unit_type == 16||nal_unit_type == 17
		||nal_unit_type == 18|| nal_unit_type == 19
		||nal_unit_type == 20|| nal_unit_type == 21 )
	{
		ReadOneBit(&p_bs);
	}

	new_PPS_ID = ReadUEV(&p_bs);
	slice_type = ReadUEV(&p_bs);

	if ((nal_unit_type == 0 || nal_unit_type == 0
		|| nal_unit_type == 2 || nal_unit_type == 4
		|| nal_unit_type == 6 || nal_unit_type == 8
		|| nal_unit_type == 10 || nal_unit_type == 12
		|| nal_unit_type == 14) && (slice_type == 0x0))
	{
		return B_SLICE_SKIP;
	}

	return (SLICE_TYPE)slice_type;
}

/*
 * Get H265 Frame Type Info
 *
 * i: pData, input buffer, the bitstream must have startcode(00 00 01 or 00 00 00 01)
 * i: m_lens: input length
 * o: SLICE_TYPE: 0 --- B Frame, 1 --- P Frame, 2 --- I frame, 3 --- B drop frame
 */ 

SLICE_TYPE rmFrameTypeCheck(RM_PBYTE pData, RM_U32 m_Lens)
{
	RM_PBYTE pIn = pData;
	RM_S32   m_FrameType = 0;
	RM_BOOL  m_long = RM_FALSE;

	if (STARTCODE2_CHECK(pIn))
	{
		pIn += 4;
		m_Lens -= 4;
		m_long = RM_TRUE;
	}
	else if(STARTCODE1_CHECK(pIn))
	{
		pIn += 3;
		m_Lens -= 3;
	}else
		return SLICE_NULL;
    while(pIn != NULL)
    {
		m_FrameType = (*(pIn)>>1) & 0x3f;
		if ((16 <= m_FrameType) && (m_FrameType <= 21)) 
		{
			return I_SLICE;
		}
		else if(m_FrameType != 32 && m_FrameType != 33 && m_FrameType != 34)
		{
			if (m_long )
			{
				pIn -= 4;
				m_Lens += 4;
			}
			else
			{
				pIn -= 3;
				m_Lens += 3;			
			}
			return SliceTypeParser(pIn, m_Lens);
		}

		if(m_FrameType == 32 || m_FrameType == 33 || m_FrameType == 34)   //First I frame has VPS,SPS,PPS Nal Unit
		    pIn = GetNextNal(pIn,&m_Lens);
		else
			break;
    }
	return SLICE_NULL;
}

