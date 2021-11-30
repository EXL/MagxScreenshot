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
#define BI_BITFIELDS        (0x03)

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
		"\t./fbdump <device> <dumpfile> <bpp>\n\n"
		"Example:\n"
		"\t./fbdump /dev/fb/0 screenshot.bmp 16 -bmp16\n"
		"\t./fbdump /dev/fb/0 screenshot.bmp 16 -bmp24\n\n"
		"\t./fbdump /dev/fb/0 screenshot.raw 16\n"
		"\t./fbdump /dev/fb/1 screenshot.raw 24\n"
		"\t./fbdump /dev/fb/0 stdout 24 > screenshot.raw\n"
	);
	return 1;
}

static int32_t ErrFile(const char *aFileName, const char *aMode) {
	fprintf(stderr, "Cannot open '%s' file for %s.\n", aFileName, aMode);
	return 1;
}

/* See https://github.com/iven/e680_fb2bmp/blob/master/src/main.c */
static void WriteBmpHeader16(FILE *aWriteFile, const display_t *aDisplay) {
	bmp_header_t lBmpHeader;
	memset(&lBmpHeader, 0, sizeof(bmp_header_t));
	lBmpHeader.file_magic = 0x4D42;
	lBmpHeader.file_size = aDisplay->bytes + 14 + 40 + sizeof(uint32_t) * 3;
	lBmpHeader.bitmap_start = 0x00000036;
	lBmpHeader.dib_header_size = 0x00000028;
	lBmpHeader.bitmap_width = aDisplay->width;
	lBmpHeader.bitmap_height = aDisplay->height;
	lBmpHeader.color_planes = 0x0001;
	lBmpHeader.bitmap_bpp = aDisplay->depth;
	lBmpHeader.compression_method = BI_BITFIELDS;
	lBmpHeader.bitmap_size = aDisplay->bytes;
	fwrite(&lBmpHeader, sizeof(bmp_header_t), 1, aWriteFile);
}

static void WriteBmpBitmap16(FILE *aWriteFile, const display_t *aDisplay, const uint8_t *aDump) {
	if (aDisplay->depth == 16) {
		uint32_t mask_565[3] = { 0xF800, 0x07E0, 0x001F };
		fwrite(mask_565, sizeof(uint32_t), 3, aWriteFile);
		int32_t y, x;
		uint16_t *lPixelPtr = (uint16_t *) aDump;
		for (y = aDisplay->height - 1; y >= 0; --y)
			for (x = 0; x < aDisplay->width; ++x) {
				uint16_t lPixel = lPixelPtr[x + y * aDisplay->width];
				fwrite(&lPixel, aDisplay->bpp, 1, aWriteFile);
			}
	} else
		fprintf(stderr, "Error: BMP support for %d-bit depth not yet implemented!\n", aDisplay->depth);
}

static void WriteBmpHeader(FILE *aWriteFile, const display_t *aDisplay) {
	bmp_header_t lBmpHeader;
	memset(&lBmpHeader, 0, sizeof(bmp_header_t));
	lBmpHeader.file_magic = 0x4D42;
	lBmpHeader.file_size = aDisplay->size * 3 + 14 + 40;
	lBmpHeader.bitmap_start = 0x00000036;
	lBmpHeader.dib_header_size = 0x00000028;
	lBmpHeader.bitmap_width = aDisplay->width;
	lBmpHeader.bitmap_height = aDisplay->height;
	lBmpHeader.color_planes = 0x0001;
	lBmpHeader.bitmap_bpp = 24;
	lBmpHeader.bitmap_size = aDisplay->size * 3;
	fwrite(&lBmpHeader, sizeof(bmp_header_t), 1, aWriteFile);
}

static void WriteBmpBitmap(FILE *aWriteFile, const display_t *aDisplay, const uint8_t *aDump) {
	if (aDisplay->depth == 16) {
		int32_t y, x;
		uint16_t *lPixelPtr = (uint16_t *) aDump;
		for (y = aDisplay->height - 1; y >= 0; --y)
			for (x = 0; x < aDisplay->width; ++x) {
				uint16_t lPixel = lPixelPtr[x + y * aDisplay->width];
				uint8_t r = ((lPixel & 0xF800) >> 11) << 3;
				uint8_t g = ((lPixel & 0x7E0) >> 5) << 2;
				uint8_t b = (lPixel & 0x1F) << 3;
				uint32_t lBmpPixel = r << 16 | g << 8 | b;
				fwrite(&lBmpPixel, 3, 1, aWriteFile);
			}
	} else
		fprintf(stderr, "Error: BMP support for %d-bit depth not yet implemented!\n", aDisplay->depth);
}

static uint8_t *CreateDumpFromFile(uint8_t *a_fb_mmap, const display_t *aDisplay) {
	uint8_t *lBitmap = malloc(aDisplay->bytes);
	memcpy(lBitmap, a_fb_mmap, aDisplay->bytes);
	return lBitmap;
}

static void CreateDumpFromFb(FILE *aOutPutDumpFile, const display_t *aDisplay, uint8_t *aDump) {
	fwrite(aDump, sizeof(char), aDisplay->bytes, aOutPutDumpFile);
}

int main(int argc, char *argv[]) {
	if (argc < 4 || argc > 5)
		return ErrUsage();

	display_t lScreen;
	lScreen.width = SCR_WIDTH;
	lScreen.height = SCR_HEIGHT;
	lScreen.size = lScreen.height * lScreen.width;
	lScreen.depth = atoi(argv[3]);
	lScreen.bpp = lScreen.depth / 8;
	lScreen.bytes = lScreen.size * lScreen.bpp;

	int32_t fb_fd = open(argv[1], O_RDONLY);
	if (fb_fd == EXIT_FAILURE)
		return ErrFile(argv[1], "read");

	uint8_t *fb_mmap = (uint8_t *) mmap(NULL, lScreen.bytes, PROT_READ, MAP_SHARED, fb_fd, 0);
	if (fb_mmap == MAP_FAILED)
		return ErrFile(argv[1], "mmap");

	uint8_t *lDump = CreateDumpFromFile(fb_mmap, &lScreen);

	munmap(fb_mmap, lScreen.bytes);
	close(fb_fd);

	FILE *lDumpFile = NULL;
	if (!strcmp("stdout", argv[2]))
		lDumpFile = stdout;
	else
		lDumpFile = fopen(argv[2], "wb");
	if (!lDumpFile)
		return ErrFile(argv[2], "write");

	if (argc == 5 && !strcmp("-bmp24", argv[4])) {
		WriteBmpHeader(lDumpFile, &lScreen);
		WriteBmpBitmap(lDumpFile, &lScreen, lDump);
	} else if (argc == 5 && !strcmp("-bmp16", argv[4])) {
		WriteBmpHeader16(lDumpFile, &lScreen);
		WriteBmpBitmap16(lDumpFile, &lScreen, lDump);
	} else
		CreateDumpFromFb(lDumpFile, &lScreen, lDump);
	free(lDump);
	fclose(lDumpFile);

	return 0;
}
