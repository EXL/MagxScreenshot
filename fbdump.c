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
		"\t./fbdump <device> <dumpfile> <bpp>\n\n"
		"Example:\n"
		"\t./fbdump /dev/fb/0 screenshot.raw 16\n"
		"\t./fbdump /dev/fb/1 screenshot.raw 32\n"
		"\t./fbdump /dev/fb/0 stdout 24 > screenshot.raw\n"
	);
	return 1;
}

static int32_t ErrFile(const char *aFileName, const char *aMode) {
	fprintf(stderr, "Cannot open '%s' file for %s.\n", aFileName, aMode);
	return 1;
}

static uint8_t *CreateDumpFromFile(uint8_t *a_fb_mmap, const display_t *aDisplay) {
	uint8_t *lBitmapRgb888 = malloc(aDisplay->bytes);
	memcpy(lBitmapRgb888, a_fb_mmap, aDisplay->bytes);
	return lBitmapRgb888;
}

/* https://github.com/Tinker-S/libjpeg-sample/blob/master/jpeg_sample.c */
static void CreateDumpFromFb(FILE *aOutPutDumpFile, const display_t *aDisplay, uint8_t *aDump) {
	fwrite(aDump, sizeof(char), aDisplay->bytes, aOutPutDumpFile);
}

int main(int argc, char *argv[]) {
	if (argc != 4)
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

	CreateDumpFromFb(lDumpFile, &lScreen, lDump);

	free(lDump);
	fclose(lDumpFile);

	return 0;
}
