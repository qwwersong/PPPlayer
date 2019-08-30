#include "common.h"
#include "playercore_platform.h"
#include <android/native_window.h> // requires ndk r5 or newer
#include <android/native_window_jni.h> // requires ndk r5 or newer
#include <EGL/egl.h> // requires ndk r5 or newer
#include <GLES/gl.h>
#include <unistd.h>
#include "IRenderingEngine.hpp"
#include "logger.h"

#undef  LOG
#define LOG_TAG "droid_video"

#define ES_VER_1 0

typedef struct DisplayContext
{
	EGLDisplay _display;
	EGLSurface _surface;
	EGLContext _context;
	int 		_width;
	int 		_height;
	struct IRenderingEngine *m_renderingEngine;
	ANativeWindow *_window;	
	/*
	EGLContext oldctx;
	EGLSurface olddsrf;
	EGLSurface oldrsrf;
	EGLDisplay olddsp;*/
} DisplayContext;


static int  Android_open_video(video_device *dev,int w, int h);
static void Android_close_video(video_device *dev);
static void Android_render_video(video_device *dev, uint8_t *src[3], int stride[3], int w, int h);

static int Video_Available(const long parm)
{
	if( parm != VIDEO_RENDER ) return 0;
	return(1);
}

static void *Video_CreateDevice(const void *parm, int size)
{
	video_device *dev = (video_device *)vpc_mem_alloc(sizeof(video_device));
	if( !dev ) return 0;
	dev->open_video = Android_open_video;
	dev->close_video = Android_close_video;
	dev->render_img = Android_render_video;
	return dev;
}

static void Video_Release(void *p)
{
	LOGD("Video_Release 0 \n");
	vpc_mem_free(p);
}

static const node_boot_func Android_video_boot_func =
{
	"android video",
	"Android Video",
	AV_DEVICE,
	VIDEO_RENDER,
	PRIORITY_STANDARD,
	0,
	Video_Available,
	Video_CreateDevice,
	Video_Release
};

static void destroy(DisplayContext *ctx) {

	if( ctx->_display )
	{
		//LOG_INFO("---------------------Destroying context");
		int a = 0;
		a = eglMakeCurrent(ctx->_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		//eglMakeCurrent(ctx->olddsp, ctx->olddsrf, ctx->olddsrf, ctx->oldctx);
		
		LOG_INFO("------------------eglMakeCurrent in release %d\r\n",a);
		
		if(ctx->_context)
			eglDestroyContext(ctx->_display, ctx->_context);
		LOG_INFO("------------------eglDestroyContext");

		if(ctx->_surface)
			eglDestroySurface(ctx->_display, ctx->_surface);
		LOG_INFO("------------------eglDestroySurface");

		eglTerminate(ctx->_display);
		//eglReleaseThread();
	}
	
    ctx->_display = EGL_NO_DISPLAY;
    ctx->_surface = EGL_NO_SURFACE;
    ctx->_context = EGL_NO_CONTEXT;
    ctx->_width = 0;
	ctx-> _height = 0;
    return;
}

static bool initialize(DisplayContext *ctx)
{
	LOGE("创建EGL环境");
    static const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };
     static const EGLint attribs2[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 5,
        EGL_GREEN_SIZE, 6,
        EGL_RED_SIZE, 5,
        EGL_NONE
    };
	
	static EGLint AttribList[] = 
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLDisplay display;
    EGLConfig config;    
    EGLint numConfigs;
    EGLint format;
    EGLSurface surface;
    EGLContext context;
    EGLint width;
    EGLint height;
    GLfloat ratio;
	
	/*
	ctx->oldctx = eglGetCurrentContext();
	ctx->olddsrf = eglGetCurrentSurface(EGL_DRAW);
	ctx->oldrsrf = eglGetCurrentSurface(EGL_READ);
	ctx->olddsp = eglGetCurrentDisplay();*/
	LOG_INFO("Keep current opengl status");
	
	if(!ctx->_window) return false;
    
    LOG_INFO("Initializing context,windows=%x",ctx->_window);
    
    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        LOG_ERROR("eglGetDisplay() returned error %d", eglGetError());
        return false;
    }
	 ctx->_display = display;
	 
	 if (!eglInitialize(display, 0, 0)) {
        LOG_ERROR("eglInitialize() returned error %d", eglGetError());
        return false;
    }

	//es 1

#if ES_VER_1
    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
        LOG_ERROR("eglChooseConfig() returned error %d", eglGetError());
        destroy(ctx);
        return false;
    }
#else
	//es 2
  if (!eglChooseConfig(display, attribs2, &config, 1, &numConfigs)) {
        LOG_ERROR("eglChooseConfig() returned error %d", eglGetError());
        destroy(ctx);
        return false;
    }
#endif

  if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        LOG_ERROR("eglGetConfigAttrib() returned error %d", eglGetError());
        destroy(ctx);
        return false;
    }

   ANativeWindow_setBuffersGeometry(ctx->_window, 0, 0, format);

    if (!(surface = eglCreateWindowSurface(display, config, ctx->_window, 0))) {
        LOG_ERROR("eglCreateWindowSurface() returned error %d", eglGetError());
        destroy(ctx);
        return false;
    }
	
	ctx->_surface = surface;
#if ES_VER_1
    if (!(context = eglCreateContext(display, config, 0, 0))) {
        LOG_ERROR("eglCreateContext() returned error %d", eglGetError());
        destroy(ctx);
        return false;
    }
#else
	if (!(context = eglCreateContext(display, config, EGL_NO_CONTEXT, AttribList))) {
        LOG_ERROR("eglCreateContext() returned error %d", eglGetError());
        destroy(ctx);
        return false;
    }	
#endif
    ctx->_context = context;
    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOG_ERROR("eglMakeCurrent() returned error %d", eglGetError());
        destroy(ctx);
        return false;
    }

    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        LOG_ERROR("eglQuerySurface() returned error %d", eglGetError());
        destroy(ctx);
        return false;
    }
	LOG_ERROR("eglQuerySurface() width=%d, height=%d", width,height);
    ctx->_width = width;
    ctx->_height = height;
    return true;
}

void Android_Video_register(void *ptx)
{
	RegisterTools(ptx,&Android_video_boot_func, 0);
}

int Android_open_video(video_device *dev, int w, int h)
{
	LOGE("打开视频设备");
	DisplayContext *p_display_data = (DisplayContext*)vpc_mem_alloc(sizeof(DisplayContext));
	vpc_android_video *android_video = (vpc_android_video*)dev->extern_parm;
	if(!p_display_data) return 0;
	p_display_data->_window = android_video->_window;
	dev->device_pdd = p_display_data;
	initialize(p_display_data);
	LOG_INFO("Android_opened_video,from the thread=%d",gettid());
	return 1;
}

void Android_close_video(video_device *dev)
{
	DisplayContext *p_display_data = (DisplayContext*)dev->device_pdd;
	if(!p_display_data) return;
	
	if(p_display_data->m_renderingEngine)
	{
		p_display_data->m_renderingEngine->Release();
		delete p_display_data->m_renderingEngine;
		p_display_data->m_renderingEngine = 0;
	}
	destroy(p_display_data);
	vpc_mem_free(p_display_data);
	dev->device_pdd = 0;
	
	LOGD("Android_close_video from the thread id=%d",gettid());
}

// void android_extra_routine(void *pc)
// {
// 	fmp_context *ptx = (fmp_context *)pc;
// 	media_stream *stream = 0;
// 	if(ptx){
//  		stream = ptx->stream[TM_MEDIA_VIDEO];
//  	}
//  	if(stream && stream->out_device)
//  	{
// 		video_device *dev					= (video_device*)stream->out_device;
// 		vpc_android_video *android_video 	= (vpc_android_video*)dev->extern_parm;
// 		if( android_video && android_video->surfacechanged ){
// 			vpc_redraw_frame(pc);
// 		}
//  	}
// }

/**
 * 渲染
 * @param dev
 * @param src
 * @param stride
 * @param w
 * @param h
 */
void Android_render_video(video_device *dev, uint8_t *src[3], int stride[3], int w, int h)
{
	DisplayContext *p_display_data = (DisplayContext*)(dev->device_pdd);
	if( (!w || !h || !src[0] || !stride[0]) && !p_display_data->m_renderingEngine )
		return;
	int src_width = 0, src_height=0, dst_width=0, dst_height=0, width=0, height=0, s=0;
	
	vpc_android_video *android_video = (vpc_android_video*)dev->extern_parm;
	
	pthread_mutex_lock(&android_video->render_mutex);

	if( android_video->surfacechanged ){
		//release old
		if(p_display_data->m_renderingEngine)
		{
			p_display_data->m_renderingEngine->Release();
			delete p_display_data->m_renderingEngine;
			p_display_data->m_renderingEngine = 0;
		}
		destroy(p_display_data);
		
		//create new one
		p_display_data->_window = android_video->_window;
		initialize(p_display_data);
		android_video->surfacechanged = 0;	
		LOG_INFO("surface-changed, so create new opengles");
	}

#if 0
	if(p_display_data->ready == 0)
	{
		unsigned char* pDisplay = NULL;
		if(p_display_data->rgb_buf==NULL)
			p_display_data->rgb_buf = (unsigned char*)vpc_mem_alloc(w*h*2);
		else
			memset(p_display_data->rgb_buf,0,w*h*2);

		unsigned char* p_rgb_tmp = p_display_data->rgb_buf;
		if(p_rgb_tmp)
		{
			
			yuv420_2_rgb565((uint16_t *)p_rgb_tmp,
				 src[0],
				 src[1],
				 src[2],
				 w,
				 h,
				 stride[0],
				 stride[1],
				 (w)<<1);
			
			if(p_display_data->p_Post_Surface)
			{
				p_display_data->p_Post_Surface(p_display_data->p_Surface, p_rgb_tmp, w, h, p_display_data->nSdkLevel );
			}
		}
		p_display_data->ready == 1;	
	}
#endif

	if( !p_display_data->m_renderingEngine )
	{
	#if 0   
		glDisable(GL_DITHER);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
		glClearColor(0, 0, 0, 0);
		glEnable(GL_CULL_FACE);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		
		glViewport(0, 0, width, height);

		ratio = (GLfloat) width / height;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustumf(-ratio, ratio, -1, 1, 1, 10);
		
	#else
		#if ES_VER_1
		p_display_data->m_renderingEngine = CreateRenderer1();
		#else
		p_display_data->m_renderingEngine = CreateRenderer2();
		#endif

		LOG_INFO("m_renderingEngine=%x",p_display_data->m_renderingEngine);
		p_display_data->m_renderingEngine->Initialize(p_display_data->_width,p_display_data->_height,w,h);

		int v = android_video->scale_method;
		float xScale = 1.0,yScale = 1.0;
		if (v == VIDEO_SCALE_FIT)
		{
			xScale = (float)p_display_data->_width / w;
			yScale = (float)p_display_data->_height / h;
			if(xScale < yScale ) yScale = xScale;
			else xScale = yScale;
			p_display_data->m_renderingEngine->ApplyView(0,xScale,yScale, android_video->mirror);		
		}
		else if(v == VIDEO_SCALE_FILL)
		{
			xScale = (float)p_display_data->_width / w;
			yScale = (float)p_display_data->_height / h;
			p_display_data->m_renderingEngine->ApplyView(1,xScale,yScale, android_video->mirror);		
		}
		
	#endif
		LOG_INFO("Success init opengles device,rendereng=%x",p_display_data->m_renderingEngine);		
	}
	
	if( android_video->changeSetting )
	{
		int v = android_video->scale_method;
		float xScale = 1.0,yScale = 1.0;
		if (v == VIDEO_SCALE_FIT)
		{
			xScale = (float)p_display_data->_width / w;
			yScale = (float)p_display_data->_height / h;
			if(xScale < yScale ) yScale = xScale;
			else xScale = yScale;
			p_display_data->m_renderingEngine->ApplyView(0,xScale,yScale, android_video->mirror);
		}
		else if(v == VIDEO_SCALE_FILL)
		{
			xScale = (float)p_display_data->_width / w;
			yScale = (float)p_display_data->_height / h;
			p_display_data->m_renderingEngine->ApplyView(1,xScale,yScale, android_video->mirror);
		}
		
		android_video->changeSetting = 0;
	}
	
	if(p_display_data->m_renderingEngine)
	{
		p_display_data->m_renderingEngine->Render(0,src[0],src[1],src[2]);
		if (!eglSwapBuffers(p_display_data->_display, p_display_data->_surface))
		{
			 LOG_ERROR("eglSwapBuffers() returned error %d", eglGetError());
		}
	}
	pthread_mutex_unlock(&android_video->render_mutex);	
}


