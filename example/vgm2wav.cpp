#include "../sccvgm.hpp"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#define _open open
#define _write write
#define _lseek lseek
#define _close close
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#ifndef O_BINARY
#define O_BINARY 0
#endif

typedef struct {
    char riff[4];
    unsigned int fsize;
    char wave[4];
    char fmt[4];
    unsigned int bnum;
    unsigned short fid;
    unsigned short ch;
    unsigned int sample;
    unsigned int bps;
    unsigned short bsize;
    unsigned short bits;
    char data[4];
    unsigned int dsize;
} WavHeader;

int main(int argc, char* argv[])
{
    if (argc < 3) {
        puts("usage: vgm2wav /path/to/input/file.vgm /path/to/output/file.wav");
        return -1;
    }

    // Read VGM file to the memory
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        puts("VGM file not found.");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    if (size < 1) {
        puts("VGM file is empty.");
        fclose(fp);
        return -1;
    }
    fseek(fp, 0, SEEK_SET);
    uint8_t* vgm = (uint8_t*)malloc(size);
    if (!vgm) {
        puts("No memory.");
        fclose(fp);
        return -1;
    }
    if (size != fread(vgm, 1, size, fp)) {
        puts("File read error");
        free(vgm);
        fclose(fp);
        return -1;
    }
    fclose(fp);

    // Load to the driver
    scc::VgmDriver scc;
    if (!scc.load(vgm, size)) {
        puts("scc.load failed! (invalid data, version or chipset)");
        free(vgm);
        return -1;
    }

    // Open wav file
    fp = fopen(argv[2], "wb");
    if (!fp) {
        puts("Can not open wav file.");
        free(vgm);
        return -1;
    }

    // initialize wave header
    WavHeader wh;
    memset(&wh, 0, sizeof(wh));
    strncpy(wh.riff, "RIFF", 4);
    strncpy(wh.wave, "WAVE", 4);
    strncpy(wh.fmt, "fmt ", 4);
    strncpy(wh.data, "data", 4);
    wh.bnum = 16;
    wh.fid = 1;
    wh.ch = 1;
    wh.sample = 44100;
    wh.bps = 88200;
    wh.bsize = 2;
    wh.bits = 16;
    wh.dsize = 0;
    fwrite(&wh, 1, sizeof(wh), fp);

    // render pcm
    int16_t buf[4410];
    while (scc.getLoopCount() < 1 && scc.isPlaying()) {
        scc.render(buf, (int)sizeof(buf) / 2);
        fwrite(buf, 1, sizeof(buf), fp);
        wh.dsize += (int)sizeof(buf);
    }

    // render fadeout in 3.2 sec
    for (int i = 0; i < 32; i++) {
        scc.render(buf, (int)sizeof(buf) / 2);
        for (int n = 0; n < (int)(sizeof(buf) / 2); n++) {
            int wav = buf[n];
            wav *= 32 - i;
            wav /= 32;
            buf[n] = (int16_t)wav;
        }
        fwrite(buf, 1, sizeof(buf), fp);
        wh.dsize += (int)sizeof(buf);
    }

    // update wave header
    wh.fsize = wh.dsize + sizeof(wh) - 8;
    int fd = _open(argv[2], O_RDWR | O_BINARY);
    if (-1 != fd) {
        _lseek(fd, 0, 0);
        _write(fd, &wh, sizeof(wh));
        _close(fd);
    }
    return 0;
}
