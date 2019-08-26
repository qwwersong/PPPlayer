/*
 *  RenderingEnine2.cpp
 *  HelloArrow
 *
 *  Created by apple on 12-1-27.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */


#include <EGL/egl.h> // requires ndk r5 or newer
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "IRenderingEngine.hpp"
#include <stdio.h>
#include "logger.h"
#define LOG_TAG "ESV1"

typedef struct Vertex {
    float Position[2];
	float Color[4];
	float Texture[2];
}Vertex;

class RenderingEngine1 : public IRenderingEngine {
public:
    RenderingEngine1();
	~RenderingEngine1();
    void Initialize(int vw,int vh, int width, int height);
	void Release();
    void Render(int zoom, const void *in_y,const void *in_u, const void *in_v) const;
 	void ApplyView(int zoom, float xS, float yS, int mirror);
    void ChangeView(int vw, int vh);
private:
	void ApplyOrtho(int vw, int vh) const;
    GLuint m_framebuffer;
    GLuint m_renderbuffer;
	GLuint m_TextureYUV[3];
	GLsizei tw;
	GLsizei th;
	
	GLfloat viewWidth;
	GLfloat viewHeight;
	GLfloat picWidth;
	GLfloat	picHeight;
	Vertex shape[4];

};

IRenderingEngine* CreateRenderer1()
{
    return new RenderingEngine1();
}

RenderingEngine1::~RenderingEngine1()
{
	//release resource
	/* it's not need for android
	glDeleteRenderbuffersOES(1,&m_renderbuffer);
	glDeleteFramebuffersOES(1, &m_framebuffer);
	*/
}


RenderingEngine1::RenderingEngine1()
{
/*it's not need for android
    glGenRenderbuffersOES(1, &m_renderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffer);
	
	// Create the framebuffer object and attach the color buffer.
    glGenFramebuffersOES(1, &m_framebuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES,
							  GL_COLOR_ATTACHMENT0_OES,
							  GL_RENDERBUFFER_OES,
							  m_renderbuffer);
							  */
	LOG_INFO("current use OpenglES 1.0");
}

void RenderingEngine1::Release()
{
	glDeleteTextures(1, m_TextureYUV);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);	
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);	
}

static int make_2n( int value )
{
	int n1 = value;
	--n1;
	n1 |= n1 >> 16;
	n1 |= n1 >> 8;
	n1 |= n1 >> 4;
	n1 |= n1 >> 2;
	n1 |= n1 >> 1;
	++n1;
	return n1;	
}

void RenderingEngine1::Initialize(int vw,int vh, int width, int height)
{
	printf("nt vw=%d,int vh=%d, int width=%d, int height=%d/n",vw, vh, width, height);
	//texture size
	tw = make_2n(width);
	th = make_2n(height);
	
	picWidth = width;
	picHeight = height;
	
	viewWidth = vw;
	viewHeight = vh;
    
    glViewport(0, 0, vw, vh);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(-vw/2, vw/2, -vh/2, vh/2, -1, 1);

	
	GLsizei stride = sizeof(Vertex);

	
	float Smax,Tmax;
	float xc = vw/2.0;
	float yc = vh/2.0;
	
	xc=yc=0.0;

	
	Smax = ((float)width)/tw;
	Tmax = ((float)height)/th;
	
	shape[0].Position[0] = -picWidth/2.0+xc;
	shape[0].Position[1] = -picHeight/2.0+yc;
	shape[0].Color[0] = 1.0;
	shape[0].Color[1] = 0.0;
	shape[0].Color[2] = 0.0;
	shape[0].Color[3] = 1.0;
	shape[0].Texture[0] = 0;
	shape[0].Texture[1] = Tmax;
	
	shape[1].Position[0] = picWidth/2.0+xc;
	shape[1].Position[1] = -picHeight/2.0+yc;
	shape[1].Color[0] = 0.0;
	shape[1].Color[1] = 1.0;
	shape[1].Color[2] = 0.0;
	shape[1].Color[3] = 1.0;
	shape[1].Texture[0] = Smax;
	shape[1].Texture[1] = Tmax;
	
	
	shape[2].Position[0] = -picWidth/2.0+xc;
	shape[2].Position[1] = picHeight/2.0+yc;
	shape[2].Color[0] = 0.0;
	shape[2].Color[1] = 0.0;
	shape[2].Color[2] = 1.0;
	shape[2].Color[3] = 1.0;
	shape[2].Texture[0] = 0.0;
	shape[2].Texture[1] = 0.0;
	
	shape[3].Position[0] = picWidth/2.0+xc;
	shape[3].Position[1] = picHeight/2.0+yc;	
	shape[3].Color[0] = 1.0;
	shape[3].Color[1] = 1.0;
	shape[3].Color[2] = 0.0;
	shape[3].Color[3] = 1.0;

	shape[3].Texture[0] = Smax;
	shape[3].Texture[1] = 0.0;

	const GLvoid* pCoords = &shape[0].Position[0];
	const GLvoid* pColor = &shape[0].Color[0];
	const GLvoid* pTC = &shape[0].Texture[0];
	
	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
//    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glVertexPointer(2, GL_FLOAT, stride, pCoords);
//    glColorPointer(4, GL_FLOAT, stride, pColor);
	glTexCoordPointer(2, GL_FLOAT, stride, pTC);

	//set texture information
	glGenTextures(1, m_TextureYUV);
    glBindTexture(GL_TEXTURE_2D, m_TextureYUV[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, tw, 
				 th, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	
	
}

void RenderingEngine1::Render(int zoom,const void *in_y,const void *in_u, const void *in_v) const
{
	glClearColor(0.0, 0.0, 0.0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
	if(in_y )
	{
		glBindTexture(GL_TEXTURE_2D, m_TextureYUV[0]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0,tw, 
						th, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, (const GLvoid*)in_y);
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderingEngine1::ChangeView(int vw, int vh)
{
	viewWidth = vw;
	viewHeight = vh;
    
    glViewport(0, 0, vw, vh);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(-vw/2, vw/2, -vh/2, vh/2, -1, 1);
    
}

void RenderingEngine1::ApplyView(int adjust_size,float xS, float yS, int mirror)
{
	float a,b,c,d;
	//glTranslatef(-([self bounds].size.width - video_width )/2 * xScale, -([self bounds].size.height - video_height ) /2 * yScale, 0.0f);
	//glScalef(xScale, yScale, 1.0);
//	if (zoom)
//	{
//		c = -(viewWidth - picWidth )/2.0 * xS;
//		d = -(viewHeight - picHeight )/2.0 * yS;
//		a = xS;
//		b = yS;
		
//	}
//	else
	{
		a = xS;
		b = yS;
		c = d = 0.0;
	}
	
    float mView[16] = {
        a, 0, 0, 0,
		0, b, 0, 0,
        0, 0, 1.0, 0,
        c, d, 0, 1.0
    };
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(a, b, 0);
}
