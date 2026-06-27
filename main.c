#define PCV_IMPLEMENTATION
#include "pcv.h"

int main(void) {
    const char *path = "/home/motorbreath/programming/pcv/data/img.ppm";
    pcv_result r;

    Image img = pcv_load_ppm(path);
    if (!img.data) {
	fprintf(stderr, "load failed: %s\n", path);
	return 1;
    }
    printf("Image = %d x %d\n", img.w, img.h);

    Image gray = pcv_alloc(img.w, img.h, PCV_GRAY);
    if (!gray.data) {
	fprintf(stderr, "alloc failed\n");
	return 1;
    }
    pcv_grayscale(img, gray);
    r = pcv_save("output/gray.pgm", gray, PCV_FMT_PGM);
    if (r != PCV_OK) {
	fprintf(stderr, "save: %s\n", pcv_strerror(r));
	return 1;
    }
    
    Image hflipped = pcv_alloc(img.w, img.h, img.c);
    if (!hflipped.data) {
	fprintf(stderr, "alloc failed\n");
	return 1;
    }
    pcv_flip(img, hflipped, PCV_FLIP_H);
    if (!hflipped.data) {
	fprintf(stderr, "flip_h: %s\n", pcv_strerror(r));
	return 1;
    }
    r = pcv_save("output/hflip.ppm", hflipped, PCV_FMT_PPM);
    if (r != PCV_OK) {
	fprintf(stderr, "save: %s\n", pcv_strerror(r));
	return 1;
    }

    Image vflipped = pcv_alloc(img.w, img.h, img.c);
    if (!vflipped.data) {
	fprintf(stderr, "alloc failed\n");
	return 1;
    }
    pcv_flip(img, vflipped, PCV_FLIP_V);
    if (!vflipped.data) {
	fprintf(stderr, "flip_v: %s\n", pcv_strerror(r));
	return 1;
    }
    r = pcv_save("output/vflip.ppm", vflipped, PCV_FMT_PPM);
    if (r != PCV_OK) {
	fprintf(stderr, "save: %s\n", pcv_strerror(r));
	return 1;
    }

    Image sub = pcv_sub(img, 24, 24, 100, 100);
    if (!sub.data) {
	fprintf(stderr, "sub failed\n");
	return 1;
    }
    r = pcv_save("output/sub.ppm", sub, PCV_FMT_PPM);
    if (r != PCV_OK) {
	fprintf(stderr, "save: %s\n", pcv_strerror(r));
	return 1;
    }

    pcv_free(&img);
    pcv_free(&hflipped);
    pcv_free(&vflipped);

    return 0;
}
