#ifndef UVCCAMERA_H
#define UVCCAMERA_H

#include <mutex>

#include <queue>

#include "libuvc/libuvc.h"

namespace libuvc {

}

class FrameQueue {
public:
    FrameQueue() {}
    virtual ~FrameQueue();

    uvc_frame_t *pop()
    {
        std::lock_guard<std::mutex> _lock(_queue_lock);
        if (_queue.empty()) {
            return NULL;
        }

        auto frame = _queue.front();
        _queue.pop();
        return frame;
    }

    void push(uvc_frame_t *item)
    {
        std::lock_guard<std::mutex> _lock(_queue_lock);
        _queue.push(item);
    }

    void clear()
    {
        std::lock_guard<std::mutex> _lock(_queue_lock);
        std::queue<uvc_frame_t *>().swap(_queue);
    }

private:
    std::queue<uvc_frame_t *> _queue;
    std::mutex _queue_lock;
};


class UVCCamera
{
public:
    UVCCamera() {}

    int init(uvc_frame_format format = UVC_FRAME_FORMAT_YUYV,
             int frame_width = 640,
             int frame_height = 480,
             int fps = 30);
    int deinit();
    int start_streaming();
    void stop_streaming();
    void clear_frames();
    void set_exposure_mode(uint8_t mode)
    {
        uvc_set_ae_mode(_dev_h, mode);
    }
    void set_exposure_time(uint32_t time)
    {
        uvc_set_exposure_abs(_dev_h, time);
    }

    uvc_frame_t* poll_frame();

private:
    uvc_context_t           *_ctx   = nullptr;
    uvc_device_t            *_dev   = nullptr;
    uvc_device_handle_t     *_dev_h = nullptr;
    uvc_stream_ctrl_t       _strm_ctrl;
    uvc_stream_handle_t     *_strm_h= nullptr;

    FrameQueue                  _frame_queue;

    void add_frame(uvc_frame_t *frame);
    static void stream_callback(uvc_frame_t *frame, void *user_ptr);
};

#endif
