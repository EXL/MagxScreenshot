#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <jpeglib.h>

#define RGB666_TO_RGB888(c) ((((c) & (0x3F << 0)) <<  2) | (((c) & (0x3F <<  6)) << 4) | (((c) & (0x3F << 12)) <<  6))

typedef struct {
	int32_t width;
	int32_t height;
	uint32_t size;
	uint32_t depth;
} display_t;

static int32_t ErrUsage(void) {
	fprintf(
		stderr,
		"Usage:\n"
		"\t./jgrab <device> <JPEG image file> <quality 0..100>\n\n"
		"Example:\n"
		"\t./jgrab /dev/fb/0 screenshot.jpeg 85\n"
	);
	return 1;
}

static int32_t ErrFile(const char *aFileName, const char *aMode) {
	fprintf(stderr, "Cannot open '%s' file for %s.\n", aFileName, aMode);
	return 1;
}

static uint8_t *CreateBitmapFromFile(FILE *aReadFile, const display_t *aDisplay) {
	int32_t y, x;
	uint8_t *lBitmapRgb888 = malloc(aDisplay->size * aDisplay->depth);
	for (y = 0; y < aDisplay->height; ++y)
		for (x = 0; x < aDisplay->width; ++x) {
			uint32_t lPixelRgb666 = 0x000000;
			fread(&lPixelRgb666, 3, 1, aReadFile); /* RGB666 in RGB888, 3 bytes, 18-bit or 24-bit for color. */
			lPixelRgb666 = RGB666_TO_RGB888(lPixelRgb666);
			int z = (x + y * aDisplay->width) * 3;
			lBitmapRgb888[z] = (uint8_t) (lPixelRgb666 >> 16) & 0xFF;
			lBitmapRgb888[z + 1] = (uint8_t) (lPixelRgb666 >> 8) & 0xFF;
			lBitmapRgb888[z + 2] = (uint8_t) (lPixelRgb666 >> 0) & 0xFF;
		}
	return lBitmapRgb888;
}

/* https://github.com/Tinker-S/libjpeg-sample/blob/master/jpeg_sample.c */
static void CreateJpegFromBitmap(FILE *aOutPutJpegFile, const display_t *aDisplay, uint8_t *aBitmap, int32_t aQuality) {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPROW row_pointer[1];

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, aOutPutJpegFile);

	cinfo.image_width = aDisplay->width;
	cinfo.image_height = aDisplay->height;
	cinfo.input_components = aDisplay->depth / 8;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, aQuality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &aBitmap[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
}

int main(int argc, char *argv[]) {
	if (argc != 4)
		return ErrUsage();

	display_t lScreen;
	lScreen.width = 240;
	lScreen.height = 320;
	lScreen.size = lScreen.height * lScreen.width;
	lScreen.depth = 24;

	FILE *lSmgFile = fopen(argv[1], "rb");
	if (!lSmgFile)
		return ErrFile(argv[1], "read");
	uint8_t *lBitmap = CreateBitmapFromFile(lSmgFile, &lScreen);
	fclose(lSmgFile);

	FILE *lJpegFile = fopen(argv[2], "wb");
	if (!lJpegFile)
		return ErrFile(argv[2], "write");

	CreateJpegFromBitmap(lJpegFile, &lScreen, lBitmap, atoi(argv[3]));

	free(lBitmap);
	fclose(lJpegFile);

	return 0;
}
