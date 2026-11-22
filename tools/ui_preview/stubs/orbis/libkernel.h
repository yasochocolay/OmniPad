// Desktop stand-in for <orbis/libkernel.h> (UI preview only).
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void* OrbisKernelEqueue;
typedef struct { uint64_t ident; int16_t filter; uint16_t flags; uint32_t fflags; int64_t data; void* udata; } OrbisKernelEvent;
typedef struct { size_t Size; char VersionString[0x1C]; uint32_t Version; } OrbisKernelSwVersion;
int sceKernelCreateEqueue(OrbisKernelEqueue* eq, const char* name);
int sceKernelWaitEqueue(OrbisKernelEqueue eq, OrbisKernelEvent* ev, int num, int* out, void* timo);
size_t sceKernelGetDirectMemorySize(void);
int sceKernelAllocateDirectMemory(off_t start, off_t end, size_t len, size_t align, int type, off_t* phys);
int sceKernelMapDirectMemory(void** addr, size_t len, int prot, int flags, off_t phys, size_t align);
int sceKernelReleaseDirectMemory(off_t start, size_t len);
int sceKernelOpen(const char* path, int flags, int mode);
ssize_t sceKernelRead(int fd, void* buf, size_t n);
ssize_t sceKernelWrite(int fd, const void* buf, size_t n);
int sceKernelClose(int fd);
off_t sceKernelLseek(int fd, off_t off, int whence);
int sceKernelMkdir(const char* path, int mode);
int sceKernelUnlink(const char* path);
int sceKernelGetSystemSwVersion(OrbisKernelSwVersion* v);
int sceKernelUsleep(unsigned int us);
#ifdef __cplusplus
}
#endif
