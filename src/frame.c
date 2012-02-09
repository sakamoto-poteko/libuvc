/**
 * @defgroup frame Frame processing
 */
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

/** @brief Allocate a frame structure
 * @ingroup frame
 *
 * @param data_bytes Number of bytes to allocate, or zero
 * @return New frame, or NULL on error
 */
uvc_frame_t *uvc_allocate_frame(size_t data_bytes) {
  uvc_frame_t *frame = malloc(sizeof(*frame));

  if (!frame)
    return NULL;

  bzero(frame, sizeof(*frame));

  if (data_bytes > 0) {
    frame->data_bytes = data_bytes;
    frame->data = malloc(data_bytes);

    if (!frame->data) {
      free(frame);
      return NULL;
    }
  }

  return frame;
}

/** @brief Free a frame structure
 * @ingroup frame
 *
 * @param frame Frame to destroy
 */
void uvc_free_frame(uvc_frame_t *frame) {
  if (frame->data_bytes > 0)
    free(frame->data);

  free(frame);
}

static inline unsigned char sat(int i) {
  return (unsigned char)( i >= 255 ? 255 : (i < 0 ? 0 : i));
}

/** @brief Duplicate a frame, preserving color format
 * @ingroup frame
 *
 * @param in Original frame
 * @param out Duplicate frame
 */
uvc_error_t uvc_duplicate_frame(uvc_frame_t *in, uvc_frame_t *out) {
  out->width = in->width;
  out->height = in->height;
  out->color_format = in->color_format;
  out->step = in->step;
  out->sequence = in->sequence;
  out->capture_time = in->capture_time;
  out->source = in->source;

  if (!out->data || out->data_bytes != in->data_bytes) {
    out->data_bytes = in->data_bytes;
    out->data = realloc(out->data, out->data_bytes);
  }

  if (!out->data)
    return UVC_ERROR_NO_MEM;

  memcpy(out->data, in->data, in->data_bytes);

  return UVC_SUCCESS;
}

#define YUYV2RGB_2(pyuv, prgb) { \
    float r = 1.402f * ((pyuv)[3]-128); \
    float g = -0.34414f * ((pyuv)[1]-128) - 0.71414f * ((pyuv)[3]-128); \
    float b = 1.772f * ((pyuv)[1]-128); \
    (prgb)[0] = sat(pyuv[0] + r); \
    (prgb)[1] = sat(pyuv[0] + g); \
    (prgb)[2] = sat(pyuv[0] + b); \
    (prgb)[3] = sat(pyuv[2] + r); \
    (prgb)[4] = sat(pyuv[2] + g); \
    (prgb)[5] = sat(pyuv[2] + b); \
    }
#define IYUYV2RGB_2(pyuv, prgb) { \
    int r = (22987 * ((pyuv)[3] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[1] - 128) - 11698 * ((pyuv)[3] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[1] - 128)) >> 14; \
    (prgb)[0] = sat(*(pyuv) + r); \
    (prgb)[1] = sat(*(pyuv) + g); \
    (prgb)[2] = sat(*(pyuv) + b); \
    (prgb)[3] = sat((pyuv)[2] + r); \
    (prgb)[4] = sat((pyuv)[2] + g); \
    (prgb)[5] = sat((pyuv)[2] + b); \
    }
#define IYUYV2RGB_16(pyuv, prgb) IYUYV2RGB_8(pyuv, prgb); IYUYV2RGB_8(pyuv + 16, prgb + 24);
#define IYUYV2RGB_8(pyuv, prgb) IYUYV2RGB_4(pyuv, prgb); IYUYV2RGB_4(pyuv + 8, prgb + 12);
#define IYUYV2RGB_4(pyuv, prgb) IYUYV2RGB_2(pyuv, prgb); IYUYV2RGB_2(pyuv + 4, prgb + 6);

/** @brief Convert a frame from YUYV to RGB
 * @ingroup frame
 *
 * @param in YUYV frame
 * @param out RGB frame
 */
uvc_error_t uvc_yuyv2rgb(uvc_frame_t *in, uvc_frame_t *out) {
  size_t need_bytes;

  if (in->color_format != UVC_COLOR_FORMAT_YUYV)
    return UVC_ERROR_INVALID_PARAM;

  out->width = in->width;
  out->height = in->height;
  out->color_format = UVC_COLOR_FORMAT_RGB;
  out->step = in->width * 3;
  out->sequence = in->sequence;
  out->capture_time = in->capture_time;
  out->source = in->source;

  need_bytes = out->step * out->height;

  if (!out->data || out->data_bytes != need_bytes) {
    out->data_bytes = out->step * out->height;
    out->data = realloc(out->data, out->data_bytes);
  }

  if (!out->data)
    return UVC_ERROR_NO_MEM;

  uint8_t *pyuv = in->data;
  uint8_t *prgb = out->data;
  uint8_t *prgb_end = prgb + out->data_bytes;

  while (prgb < prgb_end) {
    IYUYV2RGB_8(pyuv, prgb);

    prgb += 3 * 8;
    pyuv += 2 * 8;
  }

  return UVC_SUCCESS;
}

#define IYUYV2BGR_2(pyuv, pbgr) { \
    int r = (22987 * ((pyuv)[3] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[1] - 128) - 11698 * ((pyuv)[3] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[1] - 128)) >> 14; \
    (pbgr)[0] = sat(*(pyuv) + b); \
    (pbgr)[1] = sat(*(pyuv) + g); \
    (pbgr)[2] = sat(*(pyuv) + r); \
    (pbgr)[3] = sat((pyuv)[2] + b); \
    (pbgr)[4] = sat((pyuv)[2] + g); \
    (pbgr)[5] = sat((pyuv)[2] + r); \
    }
#define IYUYV2BGR_16(pyuv, pbgr) IYUYV2BGR_8(pyuv, pbgr); IYUYV2BGR_8(pyuv + 16, pbgr + 24);
#define IYUYV2BGR_8(pyuv, pbgr) IYUYV2BGR_4(pyuv, pbgr); IYUYV2BGR_4(pyuv + 8, pbgr + 12);
#define IYUYV2BGR_4(pyuv, pbgr) IYUYV2BGR_2(pyuv, pbgr); IYUYV2BGR_2(pyuv + 4, pbgr + 6);

/** @brief Convert a frame from YUYV to BGR
 * @ingroup frame
 *
 * @param in YUYV frame
 * @param out BGR frame
 */
uvc_error_t uvc_yuyv2bgr(uvc_frame_t *in, uvc_frame_t *out) {
  size_t need_bytes;

  if (in->color_format != UVC_COLOR_FORMAT_YUYV)
    return UVC_ERROR_INVALID_PARAM;

  out->width = in->width;
  out->height = in->height;
  out->color_format = UVC_COLOR_FORMAT_BGR;
  out->step = in->width * 3;
  out->sequence = in->sequence;
  out->capture_time = in->capture_time;
  out->source = in->source;

  need_bytes = out->step * out->height;

  if (!out->data || out->data_bytes != need_bytes) {
    out->data_bytes = out->step * out->height;
    out->data = realloc(out->data, out->data_bytes);
  }

  if (!out->data)
    return UVC_ERROR_NO_MEM;

  uint8_t *pyuv = in->data;
  uint8_t *pbgr = out->data;
  uint8_t *pbgr_end = pbgr + out->data_bytes;

  while (pbgr < pbgr_end) {
    IYUYV2BGR_8(pyuv, pbgr);

    pbgr += 3 * 8;
    pyuv += 2 * 8;
  }

  return UVC_SUCCESS;
}

#define IUYVY2RGB_2(pyuv, prgb) { \
    int r = (22987 * ((pyuv)[2] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[0] - 128) - 11698 * ((pyuv)[2] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[0] - 128)) >> 14; \
    (prgb)[0] = sat((pyuv)[1] + r); \
    (prgb)[1] = sat((pyuv)[1] + g); \
    (prgb)[2] = sat((pyuv)[1] + b); \
    (prgb)[3] = sat((pyuv)[3] + r); \
    (prgb)[4] = sat((pyuv)[3] + g); \
    (prgb)[5] = sat((pyuv)[3] + b); \
    }
#define IUYVY2RGB_16(pyuv, prgb) IUYVY2RGB_8(pyuv, prgb); IUYVY2RGB_8(pyuv + 16, prgb + 24);
#define IUYVY2RGB_8(pyuv, prgb) IUYVY2RGB_4(pyuv, prgb); IUYVY2RGB_4(pyuv + 8, prgb + 12);
#define IUYVY2RGB_4(pyuv, prgb) IUYVY2RGB_2(pyuv, prgb); IUYVY2RGB_2(pyuv + 4, prgb + 6);

/** @brief Convert a frame from UYVY to RGB
 * @ingroup frame
 * @param ini UYVY frame
 * @param out RGB frame
 */
uvc_error_t uvc_uyvy2rgb(uvc_frame_t *in, uvc_frame_t *out) {
  size_t need_bytes;

  if (in->color_format != UVC_COLOR_FORMAT_UYVY)
    return UVC_ERROR_INVALID_PARAM;

  out->width = in->width;
  out->height = in->height;
  out->color_format = UVC_COLOR_FORMAT_RGB;
  out->step = in->width *3;
  out->sequence = in->sequence;
  out->capture_time = in->capture_time;
  out->source = in->source;

  need_bytes = out->step * out->height;

  if (!out->data || out->data_bytes != need_bytes) {
    out->data_bytes = need_bytes;
    out->data = realloc(out->data, need_bytes);
  }

  if (!out->data)
    return UVC_ERROR_NO_MEM;

  uint8_t *pyuv = in->data;
  uint8_t *prgb = out->data;
  uint8_t *prgb_end = prgb + out->data_bytes;

  while (prgb < prgb_end) {
    IUYVY2RGB_8(pyuv, prgb);

    prgb += 3 * 8;
    pyuv += 2 * 8;
  }

  return UVC_SUCCESS;
}

#define IUYVY2BGR_2(pyuv, pbgr) { \
    int r = (22987 * ((pyuv)[2] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[0] - 128) - 11698 * ((pyuv)[2] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[0] - 128)) >> 14; \
    (pbgr)[0] = sat((pyuv)[1] + b); \
    (pbgr)[1] = sat((pyuv)[1] + g); \
    (pbgr)[2] = sat((pyuv)[1] + r); \
    (pbgr)[3] = sat((pyuv)[3] + b); \
    (pbgr)[4] = sat((pyuv)[3] + g); \
    (pbgr)[5] = sat((pyuv)[3] + r); \
    }
#define IUYVY2BGR_16(pyuv, pbgr) IUYVY2BGR_8(pyuv, pbgr); IUYVY2BGR_8(pyuv + 16, pbgr + 24);
#define IUYVY2BGR_8(pyuv, pbgr) IUYVY2BGR_4(pyuv, pbgr); IUYVY2BGR_4(pyuv + 8, pbgr + 12);
#define IUYVY2BGR_4(pyuv, pbgr) IUYVY2BGR_2(pyuv, pbgr); IUYVY2BGR_2(pyuv + 4, pbgr + 6);

/** @brief Convert a frame from UYVY to BGR
 * @ingroup frame
 * @param ini UYVY frame
 * @param out BGR frame
 */
uvc_error_t uvc_uyvy2bgr(uvc_frame_t *in, uvc_frame_t *out) {
  size_t need_bytes;

  if (in->color_format != UVC_COLOR_FORMAT_UYVY)
    return UVC_ERROR_INVALID_PARAM;

  out->width = in->width;
  out->height = in->height;
  out->color_format = UVC_COLOR_FORMAT_BGR;
  out->step = in->width *3;
  out->sequence = in->sequence;
  out->capture_time = in->capture_time;
  out->source = in->source;

  need_bytes = out->step * out->height;

  if (!out->data || out->data_bytes != need_bytes) {
    out->data_bytes = need_bytes;
    out->data = realloc(out->data, need_bytes);
  }

  if (!out->data)
    return UVC_ERROR_NO_MEM;

  uint8_t *pyuv = in->data;
  uint8_t *pbgr = out->data;
  uint8_t *pbgr_end = pbgr + out->data_bytes;

  while (pbgr < pbgr_end) {
    IUYVY2BGR_8(pyuv, pbgr);

    pbgr += 3 * 8;
    pyuv += 2 * 8;
  }

  return UVC_SUCCESS;
}

/** @brief Convert a frame to RGB
 * @ingroup frame
 *
 * @param in non-RGB frame
 * @param out RGB frame
 */
uvc_error_t uvc_any2rgb(uvc_frame_t *in, uvc_frame_t *out) {
  switch (in->color_format) {
    case UVC_COLOR_FORMAT_YUYV:
      return uvc_yuyv2rgb(in, out);
    case UVC_COLOR_FORMAT_UYVY:
      return uvc_uyvy2rgb(in, out);
    case UVC_COLOR_FORMAT_RGB:
      return uvc_duplicate_frame(in, out);
    default:
      return UVC_ERROR_NOT_SUPPORTED;
  }
}

/** @brief Convert a frame to BGR
 * @ingroup frame
 *
 * @param in non-BGR frame
 * @param out BGR frame
 */
uvc_error_t uvc_any2bgr(uvc_frame_t *in, uvc_frame_t *out) {
  switch (in->color_format) {
    case UVC_COLOR_FORMAT_YUYV:
      return uvc_yuyv2bgr(in, out);
    case UVC_COLOR_FORMAT_UYVY:
      return uvc_uyvy2bgr(in, out);
    case UVC_COLOR_FORMAT_BGR:
      return uvc_duplicate_frame(in, out);
    default:
      return UVC_ERROR_NOT_SUPPORTED;
  }
}