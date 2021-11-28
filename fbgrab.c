/* C */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* POSIX */
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

/* Defines */
#define SCR_WIDTH           (240)
#define SCR_HEIGHT          (320)
#define SCR_DEPTH           (24)
#define RGB666_TO_RGB888(c) ((((c) & (0x3F << 0)) <<  2) | (((c) & (0x3F <<  6)) << 4) | (((c) & (0x3F << 12)) <<  6))

typedef struct {
	int32_t width;
	int32_t height;
	uint32_t size;
	uint32_t depth;
	uint8_t bpp;
	uint32_t bytes;
} display_t;

/* See: https://en.wikipedia.org/wiki/BMP_file_format */
#pragma pack(push, 1)
typedef struct {
	/* Bitmap file header */
	uint16_t file_magic;
	uint32_t file_size;
	uint32_t bytes_reserved;
	uint32_t bitmap_start;
	/* DIB header (bitmap information header) */
	uint32_t dib_header_size;
	int32_t bitmap_width;
	int32_t bitmap_height;
	uint16_t color_planes;
	uint16_t bitmap_bpp;
	uint32_t compression_method;
	uint32_t bitmap_size;
	int32_t bitmap_width_ppm;
	int32_t bitmap_height_ppm;
	uint32_t num_of_colors;
	uint32_t num_of_important_colors;
} bmp_header_t;
#pragma pack(pop)

static int32_t ErrUsage(void) {
	fprintf(
		stderr,
		"Usage:\n"
		"\t./fbgrab <device> <BMP image file>\n\n"
		"Example:\n"
		"\t./fbgrab /dev/fb/0 screenshot1.bmp\n"
		"\t./fbgrab /dev/fb/1 screenshot2.bmp\n"
		"\t./fbgrab /dev/fb/0 stdout > screenshot3.bmp\n"
	);
	return 1;
}

static int32_t ErrFile(const char *aFileName, const char *aMode) {
	fprintf(stderr, "Cannot open '%s' file for %s.\n", aFileName, aMode);
	return 1;
}

static uint32_t *CreateBitmapFromFile(uint8_t *a_fb_mmap, const display_t *aDisplay) {
	int32_t y, x;
	uint32_t *lBitmapRgb888 = malloc(aDisplay->size * sizeof(uint32_t));
	for (y = 0; y < aDisplay->height; ++y)
		for (x = 0; x < aDisplay->width; ++x) {
			uint32_t lPixelRgb666 = 0x000000;
			uint8_t r = *a_fb_mmap; ++a_fb_mmap;
			uint8_t g = *a_fb_mmap; ++a_fb_mmap;
			uint8_t b = *a_fb_mmap; ++a_fb_mmap;
			lPixelRgb666 = (b << 16) | (g << 8) | r;
			lBitmapRgb888[x + y * aDisplay->width] = RGB666_TO_RGB888(lPixelRgb666);
		}
	return lBitmapRgb888;
}

static void WriteBmpHeader(FILE *aWriteFile, const display_t *aDisplay) {
	bmp_header_t lBmpHeader;
	memset(&lBmpHeader, 0, sizeof(bmp_header_t));
	lBmpHeader.file_magic = 0x4D42;
	lBmpHeader.file_size = aDisplay->size * aDisplay->bpp + 14 + 40; /* RGB888/24/3, BMP header, DIB header. */
	lBmpHeader.bitmap_start = 0x00000036;
	lBmpHeader.dib_header_size = 0x00000028;
	lBmpHeader.bitmap_width = aDisplay->width;
	lBmpHeader.bitmap_height = aDisplay->height;
	lBmpHeader.color_planes = 0x0001;
	lBmpHeader.bitmap_bpp = 0x0018;
	lBmpHeader.bitmap_size = aDisplay->bytes; /* RGB888/24/3. */
	fwrite(&lBmpHeader, sizeof(bmp_header_t), 1, aWriteFile);
}

static void WriteBmpBitmap(FILE *aWriteFile, const display_t *aDisplay, const uint32_t *aBitmap) {
	int32_t y, x;
	for (y = aDisplay->height - 1; y >= 0; --y)
		for (x = 0; x < aDisplay->width; ++x) {
			uint32_t lPixelRgb888 = aBitmap[x + y * aDisplay->width];
			fwrite(&lPixelRgb888, aDisplay->bpp, 1, aWriteFile);
		}
}

int main(int argc, char *argv[]) {
	if (argc != 3)
		return ErrUsage();

	display_t lScreen;
	lScreen.width = SCR_WIDTH;
	lScreen.height = SCR_HEIGHT;
	lScreen.size = lScreen.height * lScreen.width;
	lScreen.depth = SCR_DEPTH;
	lScreen.bpp = lScreen.depth / 8;
	lScreen.bytes = lScreen.size * lScreen.bpp;

	int32_t fb_fd = open(argv[1], O_RDONLY);
	if (fb_fd == EXIT_FAILURE)
		return ErrFile(argv[1], "read");

	uint8_t *fb_mmap = (uint8_t *) mmap(NULL, lScreen.bytes, PROT_READ, MAP_SHARED, fb_fd, 0);
	if (fb_mmap == MAP_FAILED)
		return ErrFile(argv[1], "mmap");

	uint32_t *lBitmap = CreateBitmapFromFile(fb_mmap, &lScreen);

	munmap(fb_mmap, lScreen.bytes);
	close(fb_fd);

	FILE *lBmpFile = NULL;
	if (!strcmp("stdout", argv[2]))
		lBmpFile = stdout;
	else
		lBmpFile = fopen(argv[2], "wb");
	if (!lBmpFile)
		return ErrFile(argv[2], "write");
	WriteBmpHeader(lBmpFile, &lScreen);
	WriteBmpBitmap(lBmpFile, &lScreen, lBitmap);
	free(lBitmap);
	fclose(lBmpFile);

	return 0;
}
