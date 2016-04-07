/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>
#include <utils/Timers.h>

#include <binder/Parcel.h>
#include <binder/IInterface.h>

#include <gui/IGraphicBufferProducer.h>
#include <hardware/hwcomposer.h>

namespace android {
// ----------------------------------------------------------------------------

enum {
    REQUEST_BUFFER = IBinder::FIRST_CALL_TRANSACTION,
    SET_BUFFER_COUNT,
    DEQUEUE_BUFFER,
    QUEUE_BUFFER,
    CANCEL_BUFFER,
    QUERY,
    CONNECT,
    DISCONNECT,
    SET_PARAMETER,
    GET_PARAMETER,
    SET_CROP,
    SET_TRANSFORM,
    SET_SCALINGMODE,
    SET_TIMESTEAP,
};

class BpGraphicBufferProducer : public BpInterface<IGraphicBufferProducer>
{
public:
    BpGraphicBufferProducer(const sp<IBinder>& impl)
        : BpInterface<IGraphicBufferProducer>(impl)
    {
    }

    virtual status_t requestBuffer(int bufferIdx, sp<GraphicBuffer>* buf) {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(bufferIdx);
        status_t result =remote()->transact(REQUEST_BUFFER, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        bool nonNull = reply.readInt32();
        if (nonNull) {
            *buf = new GraphicBuffer();
            reply.read(**buf);
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setBufferCount(int bufferCount)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(bufferCount);
        status_t result =remote()->transact(SET_BUFFER_COUNT, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t dequeueBuffer(int *buf, sp<Fence>* fence, bool async,
            uint32_t w, uint32_t h, uint32_t format, uint32_t usage) {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(async);
        data.writeInt32(w);
        data.writeInt32(h);
        data.writeInt32(format);
        data.writeInt32(usage);
        status_t result = remote()->transact(DEQUEUE_BUFFER, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        *buf = reply.readInt32();
        bool nonNull = reply.readInt32();
        if (nonNull) {
            *fence = new Fence();
            reply.read(**fence);
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t queueBuffer(int buf,
            const QueueBufferInput& input, QueueBufferOutput* output) {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(buf);
        data.write(input);
        status_t result = remote()->transact(QUEUE_BUFFER, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        memcpy(output, reply.readInplace(sizeof(*output)), sizeof(*output));
        result = reply.readInt32();
        return result;
    }

    virtual void cancelBuffer(int buf, const sp<Fence>& fence) {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(buf);
        data.write(*fence.get());
        remote()->transact(CANCEL_BUFFER, data, &reply);
    }

    virtual int query(int what, int* value) {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(what);
        status_t result = remote()->transact(QUERY, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        value[0] = reply.readInt32();
        result = reply.readInt32();
        return result;
    }

    virtual status_t connect(const sp<IBinder>& token,
            int api, bool producerControlledByApp, QueueBufferOutput* output) {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeStrongBinder(token);
        data.writeInt32(api);
        data.writeInt32(producerControlledByApp);
        status_t result = remote()->transact(CONNECT, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        memcpy(output, reply.readInplace(sizeof(*output)), sizeof(*output));
        result = reply.readInt32();
        return result;
    }

    virtual status_t disconnect(int api) {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(api);
        status_t result =remote()->transact(DISCONNECT, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

	virtual int setParameter(uint32_t cmd,uint32_t value) 
    {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(cmd);
        if(cmd == HWC_LAYER_SETINITPARA)
        {
        	layerinitpara_t  *layer_info = (layerinitpara_t  *)value;	        	
        	data.write((void *)value,sizeof(layerinitpara_t));
        }
        else if(cmd == HWC_LAYER_SETFRAMEPARA)
        {
        	data.write((void *)value,sizeof(libhwclayerpara_t));
        }
        else if(cmd == HWC_LAYER_SET3DMODE)
        {
        	data.write((void *)value,sizeof(video3Dinfo_t));
        }
        else
        {
        	data.writeInt32(value);
    	}
        status_t result = remote()->transact(SET_PARAMETER, data, &reply);
        if (result != NO_ERROR) 
        {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual uint32_t getParameter(uint32_t cmd) 
    {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(cmd);
        status_t result =remote()->transact(GET_PARAMETER, data, &reply);
        if (result != NO_ERROR) 
        {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setCrop(const Rect& reg) {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeFloat(reg.left);
        data.writeFloat(reg.top);
        data.writeFloat(reg.right);
        data.writeFloat(reg.bottom);
        status_t result = remote()->transact(SET_CROP, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setCurrentTransform(uint32_t transfrom)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(transfrom);
        status_t result =remote()->transact(SET_TRANSFORM, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setCurrentScalingMode(int mode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt32(mode);
        status_t result =remote()->transact(SET_SCALINGMODE, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setTimestamp(int64_t timestamp)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IGraphicBufferProducer::getInterfaceDescriptor());
        data.writeInt64(timestamp);
        status_t result =remote()->transact(SET_TIMESTEAP, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }
};

IMPLEMENT_META_INTERFACE(GraphicBufferProducer, "android.gui.IGraphicBufferProducer");

// ----------------------------------------------------------------------

status_t BnGraphicBufferProducer::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case REQUEST_BUFFER: {
            CHECK_INTERFACE(IGraphicBufferProducer, data, reply);
            int bufferIdx   = data.readInt32();
            sp<GraphicBuffer> buffer;
            int result = requestBuffer(bufferIdx, &buffer);
            reply->writeInt32(buffer != 0);
            if (buffer != 0) {
                reply->write(*buffer);
            }
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_BUFFER_COUNT: {
            CHECK_INTERFACE(IGraphicBufferProducer, data, reply);
            int bufferCount = data.readInt32();
            int result = setBufferCount(bufferCount);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case DEQUEUE_BUFFER: {
            CHECK_INTERFACE(IGraphicBufferProducer, data, reply);
            bool async      = data.readInt32();
            uint32_t w      = data.readInt32();
            uint32_t h      = data.readInt32();
            uint32_t format = data.readInt32();
            uint32_t usage  = data.readInt32();
            int buf;
            sp<Fence> fence;
            int result = dequeueBuffer(&buf, &fence, async, w, h, format, usage);
            reply->writeInt32(buf);
            reply->writeInt32(fence != NULL);
            if (fence != NULL) {
                reply->write(*fence);
            }
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case QUEUE_BUFFER: {
            CHECK_INTERFACE(IGraphicBufferProducer, data, reply);
            int buf = data.readInt32();
            QueueBufferInput input(data);
            QueueBufferOutput* const output =
                    reinterpret_cast<QueueBufferOutput *>(
                            reply->writeInplace(sizeof(QueueBufferOutput)));
            status_t result = queueBuffer(buf, input, output);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case CANCEL_BUFFER: {
            CHECK_INTERFACE(IGraphicBufferProducer, data, reply);
            int buf = data.readInt32();
            sp<Fence> fence = new Fence();
            data.read(*fence.get());
            cancelBuffer(buf, fence);
            return NO_ERROR;
        } break;
        case QUERY: {
            CHECK_INTERFACE(IGraphicBufferProducer, data, reply);
            int value;
            int what = data.readInt32();
            int res = query(what, &value);
            reply->writeInt32(value);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        case CONNECT: {
            CHECK_INTERFACE(IGraphicBufferProducer, data, reply);
            sp<IBinder> token = data.readStrongBinder();
            int api = data.readInt32();
            bool producerControlledByApp = data.readInt32();
            QueueBufferOutput* const output =
                    reinterpret_cast<QueueBufferOutput *>(
                            reply->writeInplace(sizeof(QueueBufferOutput)));
            status_t res = connect(token, api, producerControlledByApp, output);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        case DISCONNECT: {
            CHECK_INTERFACE(IGraphicBufferProducer, data, reply);
            int api = data.readInt32();
            status_t res = disconnect(api);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
		case SET_CROP: {
            Rect reg;
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            reg.left = data.readFloat();
            reg.top = data.readFloat();
            reg.right = data.readFloat();
            reg.bottom = data.readFloat();
            status_t result = setCrop(reg);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_TRANSFORM: {
            uint32_t transform;
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            transform = data.readInt32();
            status_t result = setCurrentTransform(transform);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_SCALINGMODE: {
            uint32_t scalingmode;
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            scalingmode = data.readInt32();
            status_t result = setCurrentScalingMode(scalingmode);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_TIMESTEAP: {
            uint32_t timestamp;
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            timestamp = data.readInt64();
            status_t result = setTimestamp(timestamp);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_PARAMETER: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            uint32_t cmd    = (uint32_t)data.readInt32();
            uint32_t value;
           	if(cmd == HWC_LAYER_SETINITPARA)
	        {
	        	layerinitpara_t  layer_info;
	        	
	        	data.read((void *)&layer_info,sizeof(layerinitpara_t));
	        	
	        	value = (uint32_t)&layer_info;
	        }
	        else if(cmd == HWC_LAYER_SETFRAMEPARA)
	        {
	        	libhwclayerpara_t  frame_info;
	        	
	        	data.read((void *)&frame_info,sizeof(libhwclayerpara_t));
	        	
	        	value = (uint32_t)&frame_info;
	        }
	        else if(cmd == HWC_LAYER_SET3DMODE)
	        {
	        	video3Dinfo_t _3d_info;
	        	data.read((void *)&_3d_info, sizeof(video3Dinfo_t));
	        	value = (uint32_t)&_3d_info;
	        }
	        else
	        {
	        	value    = (uint32_t)data.readInt32();
	        }
            int res = setParameter(cmd,value);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        case GET_PARAMETER: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            uint32_t cmd    = (uint32_t)data.readInt32();
            uint32_t res = getParameter(cmd);
            reply->writeInt32((int32_t)res);
            return NO_ERROR;
        } break;
    }
    return BBinder::onTransact(code, data, reply, flags);
}

// ----------------------------------------------------------------------------

IGraphicBufferProducer::QueueBufferInput::QueueBufferInput(const Parcel& parcel) {
    parcel.read(*this);
}

size_t IGraphicBufferProducer::QueueBufferInput::getFlattenedSize() const {
    return sizeof(timestamp)
         + sizeof(isAutoTimestamp)
         + sizeof(crop)
         + sizeof(scalingMode)
         + sizeof(transform)
         + sizeof(async)
         + fence->getFlattenedSize();
}

size_t IGraphicBufferProducer::QueueBufferInput::getFdCount() const {
    return fence->getFdCount();
}

status_t IGraphicBufferProducer::QueueBufferInput::flatten(
        void*& buffer, size_t& size, int*& fds, size_t& count) const
{
    if (size < getFlattenedSize()) {
        return NO_MEMORY;
    }
    FlattenableUtils::write(buffer, size, timestamp);
    FlattenableUtils::write(buffer, size, isAutoTimestamp);
    FlattenableUtils::write(buffer, size, crop);
    FlattenableUtils::write(buffer, size, scalingMode);
    FlattenableUtils::write(buffer, size, transform);
    FlattenableUtils::write(buffer, size, async);
    return fence->flatten(buffer, size, fds, count);
}

status_t IGraphicBufferProducer::QueueBufferInput::unflatten(
        void const*& buffer, size_t& size, int const*& fds, size_t& count)
{
    size_t minNeeded =
              sizeof(timestamp)
            + sizeof(isAutoTimestamp)
            + sizeof(crop)
            + sizeof(scalingMode)
            + sizeof(transform)
            + sizeof(async);

    if (size < minNeeded) {
        return NO_MEMORY;
    }

    FlattenableUtils::read(buffer, size, timestamp);
    FlattenableUtils::read(buffer, size, isAutoTimestamp);
    FlattenableUtils::read(buffer, size, crop);
    FlattenableUtils::read(buffer, size, scalingMode);
    FlattenableUtils::read(buffer, size, transform);
    FlattenableUtils::read(buffer, size, async);

    fence = new Fence();
    return fence->unflatten(buffer, size, fds, count);
}

}; // namespace android
