/*
 *  RenderingEnine2.cpp
 *  HelloArrow
 *
 *  Created by apple on 12-1-27.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */


#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <cmath>
#include "IRenderingEngine.hpp"
#include <stdio.h>
#include <stdlib.h>
#include "logger.h"
#include <string.h>
#define LOG_TAG "ESV2"

#define STRINGIFY(A) #A

typedef struct Vertex {
    float Position[2];
	float Color[4];
	float Texture[2];
}Vertex;

#include "./shaders/Simple.vert"
#include "./shaders/Simple.frag"

class RenderingEngine2 : public IRenderingEngine {
public:
    RenderingEngine2();
	~RenderingEngine2();
    void Initialize(int vw,int vh, int width, int height);
	void Release();
    void Render(int zoom, const void *in_y,const void *in_u, const void *in_v) const;
 	void ApplyView(int adjust_size, float xS, float yS, int mirror);
    void ChangeView(int vw, int vh);
	
private:
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    GLuint BuildProgram(const char* vShader, const char* fShader) const;
	void ApplyOrtho(int vw, int vh) const;

	void AssignTexturePoint(float *xs,float *ys, int mirror);
	
	int	m_adjustSize;
	
    GLuint m_simpleProgram;
//    GLuint m_framebuffer;
//    GLuint m_renderbuffer;
	GLuint m_TextureYUV[3];
	GLsizei tw;
	GLsizei th;
	
	GLfloat viewWidth;
	GLfloat viewHeight;
	GLfloat picWidth;
	GLfloat	picHeight;
	Vertex shape[4];

};

IRenderingEngine* CreateRenderer2()
{
    return new RenderingEngine2();
}

RenderingEngine2::~RenderingEngine2()
{
	//release resource
	/*
	it's not need for android
	glDeleteRenderbuffers(1,&m_renderbuffer);
	glDeleteFramebuffers(1, &m_framebuffer);
	*/
	glDeleteProgram(m_simpleProgram);	

}


RenderingEngine2::RenderingEngine2()
{
/*
	it's not need for android
    glGenRenderbuffers(1, &m_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer);
	
	// Create the framebuffer object and attach the color buffer.
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
							  GL_COLOR_ATTACHMENT0,
							  GL_RENDERBUFFER,
							  m_renderbuffer);
							  */
	m_simpleProgram = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
	LOG_INFO("current use OpenglES 2.0");
	glUseProgram(m_simpleProgram);
	
	m_adjustSize = 1;
}

void RenderingEngine2::Release()
{
	GLuint positionSlot = glGetAttribLocation(m_simpleProgram, "Position");
	GLuint ColorSlot = glGetAttribLocation(m_simpleProgram, "SourceColor");
    GLuint textureSlot = glGetAttribLocation(m_simpleProgram, "TextureCoord");
    glDisableVertexAttribArray(positionSlot);
    glDisableVertexAttribArray(ColorSlot);	
    glDisableVertexAttribArray(textureSlot);		
	glDeleteTextures(3, m_TextureYUV);

}

static int make_2n( int value )
{
    return value;
    /*
 	int n1 = value;
	--n1;
	n1 |= n1 >> 16;
	n1 |= n1 >> 8;
	n1 |= n1 >> 4;
	n1 |= n1 >> 2;
	n1 |= n1 >> 1;
	++n1;
	return n1;	
     */
}

/*
#define X_C2 480/2
#define Y_C2 320/2

#define D_W2	512
#define D_H2 256

#define F_W2 400
#define F_H2 240

#define ts (((float)F_W2)/D_W2)
#define tt (((float)F_H2)/D_H2)

Vertex Vertices[4] = {
	{{-F_W2/2+X_C2,-F_H2/2+Y_C2 }, {ts,tt}},
	{{F_W2/2+X_C2,-F_H2/2+Y_C2 }, {0.0,tt}},
	{{-F_W2/2+X_C2,F_H2/2+Y_C2 }, {ts,0.0}},
	{{F_W2/2+X_C2,F_H2/2+Y_C2 }, {0.0,0.0}},
};
*/

void RenderingEngine2::Initialize(int vw,int vh, int width, int height)
{
	LOG_INFO("nt vw=%d,int vh=%d, int width=%d, int height=%d\r\n",vw, vh, width, height);
	//texture size
	tw = make_2n(width);
	th = make_2n(height);
	
	picWidth = width;
	picHeight = height;
	
	viewWidth = vw;
	viewHeight = vh;
    
    glViewport(0, 0, vw, vh);
	ApplyOrtho(vw, vh);

	
	void *pixels = malloc(tw*th);
	
	memset(pixels, 0,tw*th);

	//set texture information
	glGenTextures(3, m_TextureYUV);
	//////////////////////////y
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureYUV[0]);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, tw, 
				 th, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
	
	GLint location = glGetUniformLocation(m_simpleProgram, "SamplerY");
	glUniform1i(location, 0);
	
	memset(pixels, 128,tw*th);
	
	//////////////////////U
	glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_TextureYUV[1]);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, tw/2, 
				 th/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
	
	location = glGetUniformLocation(m_simpleProgram, "SamplerU");
	glUniform1i(location, 1);
	
	
	//v
	glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_TextureYUV[2]);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, tw/2, 
				 th/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
	
	location = glGetUniformLocation(m_simpleProgram, "SamplerV");
	glUniform1i(location, 2);
	glDisable(GL_DEPTH_TEST);
	free((void*)pixels);
}

void RenderingEngine2::AssignTexturePoint( float *xs,float *ys, int mirror)
{	
	int res_width, res_height;
	float scale = 1.0;
	GLsizei stride = sizeof(Vertex);
	
	float S0,T0,Smax,Tmax;
	float xc = viewWidth/2.0;
	float yc = viewHeight/2.0;
	
	xc=yc=0.0;
	
	S0 = 0.0;
	T0 = 0.0;
	Smax = ((float)picWidth)/tw;
	Tmax = ((float)picHeight)/th;
	
	if( m_adjustSize )
	{
		float f1 = viewWidth/((float)viewHeight);
		float f2 = 0;
		
		res_width = f1*picHeight;
		
		if( res_width <= picWidth ){
			res_height= picHeight;
		}
		else {
			res_width = picWidth;
			res_height= picWidth/f1;			
		}
		
		if( res_width%2 ) res_width += 1;
		if( res_height%2 ) res_height += 1;
		
		f1 = res_width/((float)picWidth);
		f2 = res_height/((float)picHeight);
		
		S0 = (Smax - f1)/2.0;
		T0 = (Tmax - f2 )/2.0;
		
		Smax -= S0;
		Tmax -= T0;
		
		if(xs)
			*xs = viewWidth/((float)res_width);
		
		if( ys )
			*ys = viewHeight/((float)res_height);
	}
	else{
		res_width = picWidth;
		res_height = picHeight;
	}
	LOG_INFO("S0=%f,T0=%f,Smax=%f,Tmax=%f,reswith=%d,resheight=%d\r\n",S0,T0,Smax,Tmax,res_width,res_height);
	
	shape[0].Position[0] = -res_width/2.0+xc;
	shape[0].Position[1] = -res_height/2.0+yc;
	shape[0].Color[0] = 1.0;
	shape[0].Color[1] = 0.0;
	shape[0].Color[2] = 0.0;
	shape[0].Color[3] = 1.0;

	if ( mirror )
	{
		shape[0].Texture[0] = Smax;
		shape[0].Texture[1] = Tmax;
	}
	else
	{
		shape[0].Texture[0] = S0;
		shape[0].Texture[1] = Tmax;
	}
	
	shape[1].Position[0] = res_width/2.0+xc;
	shape[1].Position[1] = -res_height/2.0+yc;
	shape[1].Color[0] = 0.0;
	shape[1].Color[1] = 1.0;
	shape[1].Color[2] = 0.0;
	shape[1].Color[3] = 1.0;

	if ( mirror )
	{
		shape[1].Texture[0] = S0;
		shape[1].Texture[1] = Tmax;
	}
	else
	{
		shape[1].Texture[0] = Smax;
		shape[1].Texture[1] = Tmax;
	}
	
	shape[2].Position[0] = -res_width/2.0+xc;
	shape[2].Position[1] = res_height/2.0+yc;
	shape[2].Color[0] = 0.0;
	shape[2].Color[1] = 0.0;
	shape[2].Color[2] = 1.0;
	shape[2].Color[3] = 1.0;

	if ( mirror )
	{
		shape[2].Texture[0] = Smax;
		shape[2].Texture[1] = T0;
	}
	else
	{
		shape[2].Texture[0] = S0;
		shape[2].Texture[1] = T0;
	}
	
	shape[3].Position[0] = res_width/2.0+xc;
	shape[3].Position[1] = res_height/2.0+yc;	
	shape[3].Color[0] = 1.0;
	shape[3].Color[1] = 1.0;
	shape[3].Color[2] = 0.0;
	shape[3].Color[3] = 1.0;

	if ( mirror )
	{
		shape[3].Texture[0] = S0;
		shape[3].Texture[1] = T0;
	}
	else
	{
		shape[3].Texture[0] = Smax;
		shape[3].Texture[1] = T0;
	}
	
	GLuint positionSlot = glGetAttribLocation(m_simpleProgram, "Position");
	GLuint ColorSlot = glGetAttribLocation(m_simpleProgram, "SourceColor");
    GLuint textureSlot = glGetAttribLocation(m_simpleProgram, "TextureCoord");

    glEnableVertexAttribArray(positionSlot);
    glEnableVertexAttribArray(ColorSlot);	
    glEnableVertexAttribArray(textureSlot);	

	const GLvoid* pCoords = &shape[0].Position[0];
	const GLvoid* pColor = &shape[0].Color[0];
	const GLvoid* pTC = &shape[0].Texture[0];
	
	glVertexAttribPointer(positionSlot, 2, GL_FLOAT, GL_FALSE, stride, pCoords);
	glVertexAttribPointer(ColorSlot, 4, GL_FLOAT, GL_FALSE, stride, pColor);
	glVertexAttribPointer(textureSlot, 2, GL_FLOAT, GL_FALSE, stride, pTC);	
}


GLuint RenderingEngine2::BuildProgram(const char* vertexShaderSource,
                                      const char* fragmentShaderSource) const
{
    GLuint vertexShader = BuildShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = BuildShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vertexShader);
    glAttachShader(programHandle, fragmentShader);
    glLinkProgram(programHandle);
    
    GLint linkSuccess;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetProgramInfoLog(programHandle, sizeof(messages), 0, &messages[0]);
        LOG_INFO("build program err");
        exit(1);
    }
    
    return programHandle;
}
GLuint RenderingEngine2::BuildShader(const char* source, GLenum shaderType) const
{
    GLuint shaderHandle = glCreateShader(shaderType);
    glShaderSource(shaderHandle, 1, &source, 0);
    glCompileShader(shaderHandle);
	GLint compileSuccess;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
    
    if (compileSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
        LOG_INFO("build shader err");
        exit(1);
    }
    
    return shaderHandle;
}


void RenderingEngine2::Render(int zoom,const void *in_y,const void *in_u, const void *in_v) const
{
    glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.0, 0.0, 0.0, 1);
	if(in_y && in_u && in_v)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_TextureYUV[0]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0,tw, 
						th, GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid*)in_y);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_TextureYUV[1]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0,tw/2, 
						th/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid*)in_u);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_TextureYUV[2]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0,tw/2, 
						th/2,  GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid*)in_v);

	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderingEngine2::ChangeView(int vw, int vh)
{
	return;
	/*
	viewWidth = vw;
	viewHeight = vh;
	AssignTexturePoint(0,0);
    
    glViewport(0, 0, vw, vh);
	ApplyOrtho(vw, vh);
	*/
}

void RenderingEngine2::ApplyView(int adjust_size,float xS, float yS, int mirror)
{
	float a,b,c,d,s1=1.0,s2=1.0;
	
	m_adjustSize = adjust_size;
	AssignTexturePoint( &s1, &s2, mirror);
	
	if( m_adjustSize ){
		xS = s1;
		yS = s2;
	}

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
	

	
    GLint modelviewUniform = glGetUniformLocation(m_simpleProgram, "Modelview");
    glUniformMatrix4fv(modelviewUniform, 1, 0, &mView[0]);

}


void RenderingEngine2::ApplyOrtho(int vw,int vh) const
{
	GLfloat r,l,t,b;
	l = -vw/2;
	r = vw/2;
	t = vh/2;
	b = -vh/2;
	GLfloat u =  2.0f / (r - l);
	GLfloat v =  2.0f / (t - b);
	GLfloat x = - (r + l) / (r - l);
	GLfloat y = - (t + b) / (t - b);
	float ortho[16] = {
		u,		0.0f,	0.0f, 0.0f,
		0.0f,    v,		0.0f, 0.0f,
		0.0f,	0.0f,  -1.0f, 0.0f,
		x,		y,		0.0f, 1.0f};
	
    
    GLint projectionUniform = glGetUniformLocation(m_simpleProgram, "Projection");
    glUniformMatrix4fv(projectionUniform, 1, 0, &ortho[0]);
}

