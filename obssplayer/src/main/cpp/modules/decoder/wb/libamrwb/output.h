#ifdef _DEBUG

#ifndef OUTPUT_H
#define OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
	int sampRateOut;
	int nChans;
	int bitsPerSample;
}AACFrameInfo;

void WriteWAVHeader(FILE* fp, AACFrameInfo* pFrameInfo);

void UpdateWAVHeader(char* pFileName);

#ifdef __cplusplus
}
#endif

#endif

#endif //_DEBUG