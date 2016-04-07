/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>

#include "Layer.h"
#include "SurfaceFlinger.h"
#include "SurfaceTextureLayer.h"

namespace android {
// ---------------------------------------------------------------------------


SurfaceTextureLayer::SurfaceTextureLayer(const sp<SurfaceFlinger>& flinger, const sp<Layer>& layer)
    : BufferQueue(), flinger(flinger) {
    usehwinit     = false;
    mLayer  = layer;
}

SurfaceTextureLayer::~SurfaceTextureLayer() {
    // remove ourselves from SurfaceFlinger's list. We do this asynchronously
    // because we don't know where this dtor is called from, it could be
    // called with the mStateLock held, leading to a dead-lock (it actually
    // happens).
    class MessageCleanUpList : public MessageBase {
        sp<SurfaceFlinger> flinger;
        wp<IBinder> gbp;
    public:
        MessageCleanUpList(const sp<SurfaceFlinger>& flinger, const wp<IBinder>& gbp)
            : flinger(flinger), gbp(gbp) { }
        virtual bool handler() {
            Mutex::Autolock _l(flinger->mStateLock);
            flinger->mGraphicBufferProducerList.remove(gbp);
            return true;
        }
    };
    flinger->postMessageAsync(
            new MessageCleanUpList(flinger, static_cast<BnGraphicBufferProducer*>(this)) );
}

status_t SurfaceTextureLayer::disconnect(int api) 
{
    status_t err = BufferQueue::disconnect(api);

	if(err == NO_ERROR)
	{
		switch (api) 
	    {
			case NATIVE_WINDOW_API_MEDIA_HW:
	        case NATIVE_WINDOW_API_CAMERA_HW:
	        {
	            sp<Layer> layer(mLayer.promote());
	            usehwinit     = false;
	            if (layer != NULL) 
	            {
	                Rect Crop(0,0,0,0);
	                layer->setTextureInfo(Crop, 0);
	            }
	        }
	        default:
	            break;
	    }
	}
    return err;
}

int SurfaceTextureLayer::setParameter(uint32_t cmd,uint32_t value) 
{
    int res = 0;

	BufferQueue::setParameter(cmd,value);
	
    sp<Layer> layer(mLayer.promote());
    if (layer != NULL) 
    {
    	if(cmd == HWC_LAYER_SETINITPARA)
    	{
    		layerinitpara_t  *layer_info;
    		
    		layer_info = (layerinitpara_t  *)value;

            if(IsHardwareRenderSupport())
            {
                const Rect Crop(0,0,layer_info->w,layer_info->h);
                
    		    layer->setTextureInfo(Crop,layer_info->format);

                usehwinit = true;
            }
    	}

        if(usehwinit == true)
        {
        	//res = layer->setDisplayParameter(cmd,value);
        }
    }
    
    return res;
}


uint32_t SurfaceTextureLayer::getParameter(uint32_t cmd) 
{
    uint32_t res = 0;
    
	if(cmd == NATIVE_WINDOW_CMD_GET_SURFACE_TEXTURE_TYPE) {
		return 0;
	}

    sp<Layer> layer(mLayer.promote());
    if (layer != NULL) 
    {
        //res = layer->getDisplayParameter(cmd);
    }
    
    return res;
}

// ---------------------------------------------------------------------------
}; // namespace android
