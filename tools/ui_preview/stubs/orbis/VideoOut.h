// Desktop stand-in for <orbis/VideoOut.h> (UI preview only).
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define ORBIS_VIDEO_USER_MAIN 0xFF
#define ORBIS_VIDEO_OUT_BUS_MAIN 0
#define ORBIS_VIDEO_OUT_FLIP_VSYNC 1
typedef struct { uint32_t pixelFormat, tilingMode, aspectRatio, width, height, pitchInPixel, option; } OrbisVideoOutBufferAttribute;
typedef struct { uint64_t count, processTime, tsc; int64_t flipArg; uint64_t submitTsc, reserved0; int32_t gcQueueNum, flipPendingNum, currentBuffer; uint32_t reserved1; } OrbisVideoOutFlipStatus;
int sceVideoOutOpen(int user, int bus, int index, const void* param);
int sceVideoOutAddFlipEvent(void* eq, int handle, void* udata);
void sceVideoOutSetBufferAttribute(OrbisVideoOutBufferAttribute* a, uint32_t fmt, uint32_t tiling, uint32_t aspect, uint32_t w, uint32_t h, uint32_t pitch);
int sceVideoOutRegisterBuffers(int handle, int start, void* const* addrs, int num, const OrbisVideoOutBufferAttribute* a);
int sceVideoOutSetFlipRate(int handle, int rate);
int sceVideoOutSubmitFlip(int handle, int idx, int mode, int64_t arg);
int sceVideoOutGetFlipStatus(int handle, OrbisVideoOutFlipStatus* st);
#ifdef __cplusplus
}
#endif
