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

/* PNG */
#include <png.h>

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

static int32_t ErrUsage(void) {
	fprintf(
		stderr,
		"Usage:\n"
		"\t./pgrab <device> <PNG image file> <compression 0-7>\n\n"
		"Example:\n"
		"\t./pgrab /dev/fb/0 screenshot1.png 7\n"
		"\t./pgrab /dev/fb/1 screenshot2.png 0\n"
	);
	return 1;
}

static int32_t ErrFile(const char *aFileName, const char *aMode) {
	fprintf(stderr, "Cannot open '%s' file for %s.\n", aFileName, aMode);
	return 1;
}

static uint8_t *CreateBitmapFromFile(uint8_t *a_fb_mmap, const display_t *aDisplay) {
	int32_t y, x;
	uint8_t *lBitmapRgb888 = malloc(aDisplay->bytes);
	for (y = 0; y < aDisplay->height; ++y)
		for (x = 0; x < aDisplay->width; ++x) {
			uint32_t lPixelRgb666 = 0x000000;
			uint8_t r = *a_fb_mmap; ++a_fb_mmap;
			uint8_t g = *a_fb_mmap; ++a_fb_mmap;
			uint8_t b = *a_fb_mmap; ++a_fb_mmap;
			lPixelRgb666 = (b << 16) | (g << 8) | r;
			lPixelRgb666 = RGB666_TO_RGB888(lPixelRgb666);
			int z = (x + y * aDisplay->width) * aDisplay->bpp;
			lBitmapRgb888[z] = (uint8_t) (lPixelRgb666 >> 16) & 0xFF;
			lBitmapRgb888[z + 1] = (uint8_t) (lPixelRgb666 >> 8) & 0xFF;
			lBitmapRgb888[z + 2] = (uint8_t) (lPixelRgb666 >> 0) & 0xFF;
		}
	return lBitmapRgb888;
}

static void CreatePngFromBitmap(FILE *aPngFile, const display_t *aDisplay, uint8_t *aBitmap, int32_t aCompression) {
	int i;
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, aPngFile);

	png_set_compression_level(png_ptr, aCompression);

	png_set_IHDR(
		png_ptr,
		info_ptr,
		aDisplay->width,
		aDisplay->height,
		aDisplay->depth / aDisplay->bpp,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE
	);
	png_write_info(png_ptr, info_ptr);

	uint8_t *rowPointers[SCR_HEIGHT];
	uint32_t bytesPerLine = SCR_WIDTH * aDisplay->bpp;
	for (i = 0; i < SCR_HEIGHT; ++i, aBitmap += bytesPerLine)
		rowPointers[i] = aBitmap;

	png_write_image(png_ptr, rowPointers);

	png_write_end(png_ptr, NULL);
}

int main(int argc, char *argv[]) {
	if (argc != 4)
		return ErrUsage();

	display_t lScreen;
	lScreen.width = SCR_WIDTH;
	lScreen.height = SCR_HEIGHT;
	lScreen.size = lScreen.height * lScreen.width;
	lScreen.depth = SCR_DEPTH;
	lScreen.bpp = SCR_DEPTH / 8;
	lScreen.bytes = lScreen.size * lScreen.bpp;

	int32_t fb_fd = open(argv[1], O_RDONLY);
	if (fb_fd == EXIT_FAILURE)
		return ErrFile(argv[1], "read");

	uint8_t *fb_mmap = (uint8_t *) mmap(NULL, lScreen.bytes, PROT_READ, MAP_SHARED, fb_fd, 0);
	if (fb_mmap == MAP_FAILED)
		return ErrFile(argv[1], "mmap");

	uint8_t *lBitmap = CreateBitmapFromFile(fb_mmap, &lScreen);

	munmap(fb_mmap, lScreen.bytes);
	close(fb_fd);

	FILE *lPngFile = fopen(argv[2], "wb");
	if (!lPngFile)
		return ErrFile(argv[2], "write");

	CreatePngFromBitmap(lPngFile, &lScreen, lBitmap, atoi(argv[3]));

	free(lBitmap);
	fclose(lPngFile);

	return 0;
}
