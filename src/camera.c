#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev2.h>

#include "camera.h"

#define CAM_WIDTH  640
#define CAM_HEIGHT 480
#define CAM_NUM_BUFFERS 4

/*
 * Linux 3.0 kernel 32-bit timeval og v4l2_buffer struktur.
 * Nødvendig fordi moderne musl på ARMv7 bruker 64-bit time_t,
 * mens Linux 3.0-kjernen forventer 32-bit time_t (68 bytes struct).
 */
struct v4l2_timeval_k32 {
    int32_t tv_sec;
    int32_t tv_usec;
};

struct v4l2_buffer_k32 {
    uint32_t index;
    uint32_t type;
    uint32_t bytesused;
    uint32_t flags;
    uint32_t field;
    struct v4l2_timeval_k32 timestamp;
    struct v4l2_timecode timecode;
    uint32_t sequence;
    uint32_t memory;
    union {
        uint32_t offset;
        unsigned long userptr;
        void *planes;
        int32_t fd;
    } m;
    uint32_t length;
    uint32_t reserved2;
    uint32_t reserved;
};

#define VIDIOC_QUERYBUF_K32 _IOWR('V', 9,  struct v4l2_buffer_k32)
#define VIDIOC_QBUF_K32     _IOWR('V', 15, struct v4l2_buffer_k32)
#define VIDIOC_DQBUF_K32    _IOWR('V', 17, struct v4l2_buffer_k32)

struct CamBuffer {
    void *start;
    size_t length;
};

static int g_cam_fd = -1;
static bool g_cam_streaming = false;
static struct CamBuffer g_buffers[CAM_NUM_BUFFERS];
static uint32_t g_buf_count = 0;
static uint8_t *g_current_frame = NULL;
static bool g_cam_mirror = true; /* Standard selfie-speil */
static int  g_cam_brightness = 0; /* -100 .. +100 */
static int  g_cam_contrast = 0;   /* -50 .. +50 */

/* Test-mønster hvis kamera ikke er tilgjengelig */
static uint8_t *g_fallback_frame = NULL;
static uint32_t g_anim_tick = 0;

static void generate_test_pattern(uint8_t *frame) {
    g_anim_tick++;
    int cx = (CAM_WIDTH / 2) + (int)(60.0f * (g_anim_tick % 60 - 30) / 30.0f);
    int cy = CAM_HEIGHT / 2;

    for (int y = 0; y < CAM_HEIGHT; y++) {
        for (int x = 0; x < CAM_WIDTH; x++) {
            int dx = x - cx;
            int dy = y - cy;
            int dist = (dx * dx + dy * dy);

            /* Konsentriske sirkler og retro rutenett */
            int val = (dist / 120) % 256;
            if ((x % 32 == 0) || (y % 32 == 0)) val = 220;
            if (x < 10 || x > CAM_WIDTH - 10 || y < 10 || y > CAM_HEIGHT - 10) val = 255;
            frame[y * CAM_WIDTH + x] = (uint8_t)val;
        }
    }
}

bool camera_init(void) {
    if (g_cam_fd >= 0) return true;

    g_current_frame = (uint8_t *)malloc(CAM_WIDTH * CAM_HEIGHT);
    g_fallback_frame = (uint8_t *)malloc(CAM_WIDTH * CAM_HEIGHT);

    g_cam_fd = open("/dev/video0", O_RDWR | O_NONBLOCK);
    if (g_cam_fd < 0) {
        printf("[!] Kunne ikke aapne /dev/video0: %s. Bruker virtuell cybersensor.\n", strerror(errno));
        return true; /* Kjorer med testmoenster slik at appen fungerer */
    }

    /* 1. Sett bildeformat (NV21 eller YUV420, 640x480) */
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = CAM_WIDTH;
    fmt.fmt.pix.height = CAM_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(g_cam_fd, VIDIOC_S_FMT, &fmt) < 0) {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
        ioctl(g_cam_fd, VIDIOC_S_FMT, &fmt);
    }

    /* 2. Be om mmap buffere fra Allwinner CSI kjerne-driver */
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = CAM_NUM_BUFFERS;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(g_cam_fd, VIDIOC_REQBUFS, &req) < 0 || req.count == 0) {
        printf("[!] VIDIOC_REQBUFS feilet: %s. Bruker virtuell sensor.\n", strerror(errno));
        return true;
    }
    g_buf_count = req.count;

    /* 3. Query buffers og mmap (med 32-bit kjerne-kompatibilitet) */
    bool mmap_ok = true;
    for (uint32_t i = 0; i < g_buf_count; i++) {
        struct v4l2_buffer_k32 buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(g_cam_fd, VIDIOC_QUERYBUF_K32, &buf) < 0) {
            /* Prøv standard querybuf som fallback */
            struct v4l2_buffer sbuf;
            memset(&sbuf, 0, sizeof(sbuf));
            sbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            sbuf.memory = V4L2_MEMORY_MMAP;
            sbuf.index = i;
            if (ioctl(g_cam_fd, VIDIOC_QUERYBUF, &sbuf) < 0) {
                mmap_ok = false;
                break;
            }
            buf.length = sbuf.length;
            buf.m.offset = sbuf.m.offset;
        }

        g_buffers[i].length = buf.length;
        g_buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, g_cam_fd, buf.m.offset);
        if (g_buffers[i].start == MAP_FAILED) {
            mmap_ok = false;
            break;
        }

        /* Kø buffer i maskinvaren */
        ioctl(g_cam_fd, VIDIOC_QBUF_K32, &buf);
    }

    if (!mmap_ok) {
        printf("[!] mmap av videobuf feilet. Kjorer virtuell sensor.\n");
        return true;
    }

    /* 4. Start videostrom */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(g_cam_fd, VIDIOC_STREAMON, &type) >= 0) {
        g_cam_streaming = true;
        printf("[*] Allwinner A13 CSI Kamera aktivert! (640x480 V4L2 streaming)\n");
    }

    return true;
}

void camera_close(void) {
    if (g_cam_streaming && g_cam_fd >= 0) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(g_cam_fd, VIDIOC_STREAMOFF, &type);
        g_cam_streaming = false;
    }

    for (uint32_t i = 0; i < g_buf_count; i++) {
        if (g_buffers[i].start && g_buffers[i].start != MAP_FAILED) {
            munmap(g_buffers[i].start, g_buffers[i].length);
            g_buffers[i].start = NULL;
        }
    }
    g_buf_count = 0;

    if (g_cam_fd >= 0) {
        close(g_cam_fd);
        g_cam_fd = -1;
    }

    if (g_current_frame) {
        free(g_current_frame);
        g_current_frame = NULL;
    }
    if (g_fallback_frame) {
        free(g_fallback_frame);
        g_fallback_frame = NULL;
    }
}

bool camera_is_available(void) {
    return (g_cam_fd >= 0 && g_cam_streaming);
}

const uint8_t *camera_grab_y_frame(void) {
    if (!g_cam_streaming || g_cam_fd < 0) {
        /* Bruk det animerte retro-testmønsteret */
        if (g_fallback_frame) {
            generate_test_pattern(g_fallback_frame);
            return g_fallback_frame;
        }
        return NULL;
    }

    /* Sjekk om det er en ledig ramme klar på kamerasensoren */
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(g_cam_fd, &fds);
    struct timeval tv = { .tv_sec = 0, .tv_usec = 10000 }; /* 10ms timeout */

    int r = select(g_cam_fd + 1, &fds, NULL, NULL, &tv);
    if (r <= 0) {
        /* Ingen ny ramme enna, returner forrige */
        return g_current_frame;
    }

    struct v4l2_buffer_k32 buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(g_cam_fd, VIDIOC_DQBUF_K32, &buf) < 0) {
        /* Fallback til standard dqbuf hvis k32 feiler */
        struct v4l2_buffer sbuf;
        memset(&sbuf, 0, sizeof(sbuf));
        sbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        sbuf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(g_cam_fd, VIDIOC_DQBUF, &sbuf) < 0) {
            return g_current_frame;
        }
        buf.index = sbuf.index;
    }

    /* De forste 640x480 bytene i NV21/YUV420 er ren Y-luminans */
    if (buf.index < g_buf_count && g_buffers[buf.index].start) {
        memcpy(g_current_frame, g_buffers[buf.index].start, CAM_WIDTH * CAM_HEIGHT);
    }

    /* Legg bufferen tilbake i maskinvarekoen for neste ramme */
    ioctl(g_cam_fd, VIDIOC_QBUF_K32, &buf);

    return g_current_frame;
}

void camera_set_mirror(bool mirror) {
    g_cam_mirror = mirror;
}

bool camera_get_mirror(void) {
    return g_cam_mirror;
}

void camera_adjust_brightness(int delta) {
    g_cam_brightness += delta;
    if (g_cam_brightness < -100) g_cam_brightness = -100;
    if (g_cam_brightness > 100)  g_cam_brightness = 100;
}

int camera_get_brightness(void) {
    return g_cam_brightness;
}

void camera_adjust_contrast(int delta) {
    g_cam_contrast += delta;
    if (g_cam_contrast < -50) g_cam_contrast = -50;
    if (g_cam_contrast > 50)  g_cam_contrast = 50;
}

int camera_get_contrast(void) {
    return g_cam_contrast;
}

bool camera_save_snapshot(const char *filepath, CameraRenderMode mode, int w, int h) {
    (void)mode;
    if (!g_current_frame && !g_fallback_frame) return false;
    const uint8_t *src = g_current_frame ? g_current_frame : g_fallback_frame;

    FILE *f = fopen(filepath, "w");
    if (!f) return false;

    fprintf(f, "# tabOS ASCII Camera Snapshot (%dx%d)\n", w, h);
    static const char *ramp = " .:-=+*#%@";
    int ramp_len = (int)strlen(ramp);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int src_x = (x * CAM_WIDTH) / w;
            int src_y = (y * CAM_HEIGHT) / h;
            if (g_cam_mirror) src_x = CAM_WIDTH - 1 - src_x;

            int lum = src[src_y * CAM_WIDTH + src_x] + g_cam_brightness;
            if (lum < 0) lum = 0;
            if (lum > 255) lum = 255;

            int idx = (lum * ramp_len) / 256;
            fputc(ramp[idx], f);
        }
        fputc('\n', f);
    }

    fclose(f);
    return true;
}
