/*
 *  IRenderingEngine.h
 *  HelloArrow
 *
 *  Created by apple on 12-1-27.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

// Creates an instance of the renderer and sets up various OpenGL state.

struct IRenderingEngine* CreateRenderer1();
struct IRenderingEngine* CreateRenderer2();

// Interface to the OpenGL ES renderer; consumed by GLView.
struct IRenderingEngine {
    virtual void Initialize(int vw,int vh, int width, int height) = 0;
    virtual void Release() = 0;
    virtual void ChangeView(int vw, int vh )=0;
    virtual void Render(int zoom,const void *in_y,const void *in_u, const void *in_v) const = 0;
	virtual void ApplyView(int adjust_size, float xS, float yS, int mirror)  = 0;
    virtual ~IRenderingEngine() {}
};
