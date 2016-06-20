#ifndef UVCCAMERA_H
#define UVCCAMERA_H

#include <queue>
#include <mutex>
#include <condition_variable>

#include "libuvc/libuvc.h"

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

    uvc_frame_t* poll_frame();

private:
    uvc_context_t           *_ctx   = nullptr;
    uvc_device_t            *_dev   = nullptr;
    uvc_device_handle_t     *_dev_h = nullptr;
    uvc_stream_ctrl_t       _strm_ctrl;
    uvc_stream_handle_t     *_strm_h= nullptr;

    std::queue<uvc_frame_t *>   _frame_queue;
    std::mutex                  _frame_queue_mutex;
    mutable std::condition_variable _frame_queue_has_frame;

    void add_frame(uvc_frame_t *frame);
    static void stream_callback(uvc_frame_t *frame, void *user_ptr);
};

#endif
