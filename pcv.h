#ifndef PCV_H
#define PCV_H

#include <stdint.h>

typedef enum {
    PCV_GRAY = 1,
    PCV_RGB = 3,
} pcv_channels;

typedef enum {
    PCV_OK	   = 0,
    PCV_ERR_IO	   = 1,
    PCV_ERR_FORMAT = 2,
} pcv_result;

typedef enum {
    PCV_FLIP_H    = 0,
    PCV_FLIP_V    = 1,
    PCV_FLIP_BOTH = 2
} pcv_axis;

typedef enum {
    PCV_FMT_PGM,   /* P5 binary, 1 channel  */
    PCV_FMT_PPM,   /* P6 binary, 3 channels */
} pcv_format;

typedef struct { uint8_t *data; int w, h, c, stride; } Image;

/* error convention:
 *   producers (alloc/sub/load_ppm) return Image; failure = data == NULL.
 *   ops (save/flip_*) return pcv_result; PCV_OK == success.
 * TODO: producers carry no reason code. add an optional `pcv_result *err`
 *       out-param to a producer if/when the caller needs the why.
 */
/* --- Error handling --- */
const char      *pcv_strerror(pcv_result r);

/* --- Lifetime --- */
Image            pcv_alloc(int w, int h, int c);
void             pcv_free(Image *img);
Image            pcv_sub(Image img, int x, int y, int w, int h);

/* --- File I/O --- */
Image		 pcv_load_ppm(const char *path);
pcv_result	 pcv_save(const char *path, Image img, pcv_format fmt);

/* --- Pointwise operations --- */
void             pcv_grayscale(Image src, Image dst);

/* --- Geometric transforms --- */
void             pcv_flip(Image src, Image dst, pcv_axis axis);

#endif /* PCV_H */

#ifdef PCV_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* --- Internal --- */
static uint8_t *pcv__px(Image img, int x, int y) {
    return img.data + (size_t)y * img.stride + (size_t)x * img.c;
}

static int pcv__get_dim(FILE *f) {
    int d = fgetc(f);
    while (isspace(d)) d = fgetc(f);

    int val = 0;
    while(isdigit(d)) {
	val = val * 10 + (d - '0');
	d = fgetc(f);
    }
    return val;
}

static void pcv__assert_dims(Image src, Image dst) {
    assert(src.w==dst.w && "width mismatch");
    assert(src.h==dst.h && "height mismatch");
    assert(src.c==dst.c && "channel mismatch");
    /* TODO: remove later */
    assert(src.stride==dst.stride && "stride mismatch");
}

static void pcv__assert_grayscale_dims(Image src, Image dst) {
    assert(src.w==dst.w && "width mismatch");
    assert(src.h==dst.h && "height mismatch");
    assert(src.c==PCV_RGB && "src has to be RGB input");
    assert(dst.c==PCV_GRAY && "dst has to have 1 channel");
}

/* --- Error handling --- */
const char *pcv_strerror(pcv_result r) {
    switch (r) {
    case PCV_OK		: return "OK";
    case PCV_ERR_IO	: return "IO ERROR";
    case PCV_ERR_FORMAT	: return "FORMAT MISMATCH";
    default		: return "UNKNOWN";
    }
}

/* --- Lifetime --- */
Image pcv_alloc(int w, int h, int c) {
    Image img = {0};
    size_t n = (size_t)w * h * c;
    img.data = malloc(n);
    if (!img.data) return img;

    img.w = w;
    img.h = h;
    img.c = c;
    img.stride = w*c;

    return img;
}

void pcv_free(Image *img) {
    free(img->data);
    *img = (Image){0};
}

Image pcv_sub(Image img, int x, int y, int w, int h) {
    Image sub = {0};
    if (x + w > img.w || y + h > img.h) return sub;

    sub.data = pcv__px(img, x, y);
    sub.w = w;
    sub.h = h;
    sub.c = img.c;
    sub.stride = img.stride;
    return sub;
}

/* --- File I/O --- */
Image pcv_load_ppm(const char* path) {
    Image img = {0};
    FILE *f = fopen(path, "rb");
    unsigned char magic[2];
    if (!f) return img;

    size_t ret = fread(magic, sizeof(*magic), ARRAY_SIZE(magic), f);
    if (ret != ARRAY_SIZE(magic)) return img;

    if (magic[0] != 'P' || magic[1] != '6') return img;

    img.w = pcv__get_dim(f);
    img.h = pcv__get_dim(f);
    img.c = PCV_RGB;
    img.stride = img.w * PCV_RGB;

    int maxval = pcv__get_dim(f);
    if (maxval > 255) return img;

    size_t n = (size_t)img.w * img.h * PCV_RGB;
    img.data = malloc(n);
    if (!img.data) {
	return img;
    }

    size_t npix = (size_t)img.w * img.h;
    size_t got = fread(img.data, PCV_RGB, npix, f);
    if (got != npix) {
	return img;
    }

    fclose(f);
    return img;
}

pcv_result pcv_save(const char *path, Image img, pcv_format fmt) {
    const char *magic;
    int want_c;
    switch (fmt) {
    case PCV_FMT_PGM: magic = "P5"; want_c = PCV_GRAY; break;
    case PCV_FMT_PPM: magic = "P6"; want_c = PCV_RGB;  break;
    default: return PCV_ERR_FORMAT;
    }
    if (img.c != want_c) return PCV_ERR_FORMAT;

    FILE *f = fopen(path, "wb");
    if (!f) return PCV_ERR_IO;

    fprintf(f, "%s\n%d %d\n255\n", magic, img.w, img.h);

    for (int y = 0; y < img.h; y++) {
	size_t wrote = fwrite(img.data + y*img.stride, img.c, img.w, f);
	if (wrote != (size_t)img.w) return PCV_ERR_IO;
    }

    fclose(f);
    return PCV_OK;
}

/* --- Pointwise operations --- */
void pcv_grayscale(Image src, Image dst) {
    pcv__assert_grayscale_dims(src, dst);
    for (int x = 0; x < src.w; x++) {
	for (int y = 0; y < src.h; y++) {
	    uint8_t *p = pcv__px(src, x, y);
	    uint8_t *d = pcv__px(dst, x, y);
	    d[0] = (77*p[0] + 150*p[1] + 29*p[2]) >> 8;
	}
    }
}

/* --- Geometric transforms --- */
void pcv_flip(Image src, Image dst, pcv_axis axis) {
    pcv__assert_dims(src, dst);
    int sx, sy;
    for (int x = 0; x < src.w; x++) {
	for (int y = 0; y < src.h; y++) {
	    sx = (axis == PCV_FLIP_H || axis == PCV_FLIP_BOTH) ? src.w - 1 - x : x;
	    sy = (axis == PCV_FLIP_V || axis == PCV_FLIP_BOTH) ? src.h - 1 - y : y;
	    uint8_t *s = pcv__px(src, sx, sy);
	    uint8_t *d = pcv__px(dst, x, y);
	    memcpy(d, s, src.c);
	}
    }
}

#endif /* PCV_IMPLEMENTATION */
