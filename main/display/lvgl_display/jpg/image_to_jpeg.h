// image_to_jpeg.h - Efficient JPEG encoding interface for image conversion
// JPEG encoding implementation that saves about 8KB SRAM
#pragma once
#include "sdkconfig.h"
#ifndef CONFIG_IDF_TARGET_ESP32

#include <stdint.h>
#include <stddef.h>

#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S3)
// ESP32-P4 uses V4L2 headers from esp_video component
#include <linux/videodev2.h>
#else
// ESP32-S3 and other chips: define common V4L2 pixel formats
#define V4L2_PIX_FMT_RGB565 0x50424752  // 'RGBP'
#define V4L2_PIX_FMT_RGB565X 0x52474250 // 'PRGB'
#define V4L2_PIX_FMT_RGB24 0x33424752   // 'RGB3'
#define V4L2_PIX_FMT_YUYV 0x56595559    // 'YUYV'
#define V4L2_PIX_FMT_YUV422P 0x36315559 // 'YU16'
#define V4L2_PIX_FMT_YUV420 0x32315559  // 'YU12'
#define V4L2_PIX_FMT_GREY 0x59455247    // 'GREY'
#define V4L2_PIX_FMT_UYVY 0x59565955    // 'UYVY'
#define V4L2_PIX_FMT_JPEG 0x4745504A    // 'JPEG'
#endif

typedef uint32_t v4l2_pix_fmt_t;

#ifdef __cplusplus
extern "C"
{
#endif

    // JPEG output callback function type
    // arg: user custom parameter, index: current data index, data: JPEG data block, len: data block length
    // Return: actual bytes processed
    typedef size_t (*jpg_out_cb)(void *arg, size_t index, const void *data, size_t len);

    /**
     * @brief Efficiently convert image format to JPEG
     *
     * This function uses optimized JPEG encoder with main features:
     * - Saves about 8KB SRAM (static variables changed to heap allocation)
     * - Supports multiple image format inputs
     * - High quality JPEG output
     *
     * @param src       Source image data
     * @param src_len   Source image data length
     * @param width     Image width
     * @param height    Image height
     * @param format    Image format (PIXFORMAT_RGB565, PIXFORMAT_RGB888, etc)
     * @param quality   JPEG quality (1-100)
     * @param out       Output JPEG data pointer (needs to be freed by caller)
     * @param out_len   Output JPEG data length
     *
     * @return true success, false failure
     */
    bool image_to_jpeg(uint8_t *src, size_t src_len, uint16_t width, uint16_t height,
                       v4l2_pix_fmt_t format, uint8_t quality, uint8_t **out, size_t *out_len);

    /**
     * @brief Convert image format to JPEG (callback version)
     *
     * Uses callback function to handle JPEG output data, suitable for streaming or chunked processing:
     * - Saves about 8KB SRAM (static variables changed to heap allocation)
     * - Supports streaming output without pre-allocating large buffer
     * - Processes JPEG data chunk by chunk via callback function
     *
     * @param src       Source image data
     * @param src_len   Source image data length
     * @param width     Image width
     * @param height    Image height
     * @param format    Image format
     * @param quality   JPEG quality (1-100)
     * @param cb        Output callback function
     * @param arg       User parameter passed to callback function
     *
     * @return true success, false failure
     */
    bool image_to_jpeg_cb(uint8_t *src, size_t src_len, uint16_t width, uint16_t height,
                          v4l2_pix_fmt_t format, uint8_t quality, jpg_out_cb cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif // ndef CONFIG_IDF_TARGET_ESP32