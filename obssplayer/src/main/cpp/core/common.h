#ifndef __COMMON_H__
#define __COMMON_H__

#define ENABLE_FFMPEG   1
#define SUPPORT_TIVC	0

#define URL_SCHEME_CHARS                        \
	"abcdefghijklmnopqrstuvwxyz"                \
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"                \
	"0123456789+-."

#include "playercore_platform.h"
#include "data_type.h"
#include "os.h"
#include "memory_manager.h"
#include "codec_layer.h"
#include "avout_layer.h"
#include "playercore.h"
#include "source_layer.h"
#include "playercore_base.h"
#include "audio_out.h"
#include "video_out.h"
#include "../modules/demux/demuxs.h"
#include "../modules/decoder/decoders.h"
#include <math.h>
#include <string.h>

#endif //__COMMON_H__
