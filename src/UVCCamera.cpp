#include "libuvc/UVCCamera.h"

int UVCCamera::init(uvc_frame_format format, int frame_width, int frame_height, int fps)
{
    uvc_error_t res;

    res = uvc_init(&_ctx, NULL);

    if (res < 0) {
        uvc_perror(res, "uvc_init");
        return res;
    }

    res = uvc_find_device(_ctx, &_dev, 0, 0, NULL);
    if (res < 0) {
        uvc_perror(res, "uvc_find_device");
        return res;
    }

    res = uvc_open(_dev, &_dev_h);

    if (res < 0) {
        uvc_perror(res, "uvc_open");
        return res;
    }

    res = uvc_get_stream_ctrl_format_size(_dev_h, &_strm_ctrl,
                                          format, frame_width, frame_height, fps);
    if (res < 0) {
        uvc_perror(res, "uvc_get_stream_ctrl_format_size");
        return res;
    }

    return res;
}

int UVCCamera::deinit()
{
    uvc_close(_dev_h);
    uvc_unref_device(_dev);
    uvc_exit(_ctx);
}

int UVCCamera::start_streaming()
{
    uvc_error_t res;
    res = uvc_start_streaming(_dev_h, &_strm_ctrl, UVCCamera::stream_callback, this, 0);

    if (res < 0) {
        uvc_perror(res, "start streaming");
        return res;
    }

    return res;
}

void UVCCamera::stop_streaming()
{
    uvc_stop_streaming(_dev_h);
}

uvc_frame_t* UVCCamera::poll_frame()
{
    return _frame_queue.pop();
}

void UVCCamera::stream_callback(uvc_frame_t *frame, void *user_ptr)
{
    auto cam = static_cast<UVCCamera *>(user_ptr);
    cam->_frame_queue.push(frame);
}

void UVCCamera::add_frame(uvc_frame_t *frame)
{
    _frame_queue.push(frame);
}

void UVCCamera::clear_frames()
{
    _frame_queue.clear();
}

FrameQueue::~FrameQueue() {}
