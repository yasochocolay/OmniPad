// Desktop implementations of the few PS4 system calls used by the Companion App UI code.
// Files under /data and /user/data are redirected to $PREVIEW_DATA_DIR; nothing else on disk is touched.
#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/Sysmodule.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include "pkg_updater.h"

static bool map_path(const char* in, std::string& out) {
    const char* root = getenv("PREVIEW_DATA_DIR");
    if (!root || !in) return false;
    const char* rest = NULL;
    if (strncmp(in, "/user/data/", 11) == 0) rest = in + 11;
    else if (strncmp(in, "/data/", 6) == 0) rest = in + 6;
    else if (strncmp(in, "/mnt/usb", 8) == 0) { out = std::string(root) + "/usb" + (in + 8); return true; }
    if (!rest) return false;
    out = std::string(root) + "/data/" + rest;
    return true;
}

extern "C" {
int sceKernelCreateEqueue(OrbisKernelEqueue* eq, const char*) { *eq = NULL; return 0; }
int sceKernelWaitEqueue(OrbisKernelEqueue, OrbisKernelEvent*, int, int* out, void*) { if (out) *out = 1; return 0; }
size_t sceKernelGetDirectMemorySize(void) { return (size_t)1 << 30; }
int sceKernelAllocateDirectMemory(off_t, off_t, size_t, size_t, int, off_t* phys) { *phys = 0; return 0; }
int sceKernelMapDirectMemory(void** addr, size_t len, int, int, off_t, size_t) { *addr = calloc(1, len); return *addr ? 0 : -1; }
int sceKernelReleaseDirectMemory(off_t, size_t) { return 0; }
int sceKernelOpen(const char* path, int flags, int mode) {
    std::string p; if (!map_path(path, p)) return -1;
    int fd = open(p.c_str(), flags, mode); return fd < 0 ? -1 : fd;
}
ssize_t sceKernelRead(int fd, void* buf, size_t n) { return read(fd, buf, n); }
ssize_t sceKernelWrite(int fd, const void* buf, size_t n) { return write(fd, buf, n); }
int sceKernelClose(int fd) { return close(fd); }
off_t sceKernelLseek(int fd, off_t off, int whence) { return lseek(fd, off, whence); }
int sceKernelMkdir(const char* path, int mode) { std::string p; if (!map_path(path, p)) return -1; return mkdir(p.c_str(), mode); }
int sceKernelUnlink(const char* path) { std::string p; if (!map_path(path, p)) return -1; return unlink(p.c_str()); }
int sceKernelGetSystemSwVersion(OrbisKernelSwVersion* v) {
    const char* fw = getenv("PREVIEW_FW"); unsigned bcd = fw ? (unsigned)strtoul(fw, NULL, 16) : 0x1250;
    v->Version = bcd << 16; snprintf(v->VersionString, sizeof(v->VersionString), "%X.%02X", bcd >> 8, bcd & 0xFF); return 0;
}
int sceKernelUsleep(unsigned int us) { return usleep(us); }
int sceVideoOutOpen(int, int, int, const void*) { return 1; }
int sceVideoOutAddFlipEvent(void*, int, void*) { return 0; }
void sceVideoOutSetBufferAttribute(OrbisVideoOutBufferAttribute*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {}
int sceVideoOutRegisterBuffers(int, int, void* const*, int, const OrbisVideoOutBufferAttribute*) { return 0; }
int sceVideoOutSetFlipRate(int, int) { return 0; }
int sceVideoOutSubmitFlip(int, int, int, int64_t) { return 0; }
int sceVideoOutGetFlipStatus(int, OrbisVideoOutFlipStatus* st) { memset(st, 0, sizeof(*st)); return 0; }
int sceSysmoduleLoadModule(int) { return 0; }
}

// The online updater is not exercised by the preview: report "idle".
UpdaterState PkgUpdater::get_state() const { return UPDATER_IDLE; }
UpdaterProgress PkgUpdater::get_progress() const { return UpdaterProgress(); }
std::string PkgUpdater::get_latest_version() const { return ""; }
std::string PkgUpdater::get_status_message() const { return "Preview (offline)"; }
std::string PkgUpdater::get_target_pkg_path() const { return ""; }
