// Host-side test harness for the Switch Pro BT additions in bt_driver.c.
// Includes the driver source directly and stubs every platform call.
// Kernel (ring 0) callbacks are never executed: SYS_KEXEC is intercepted.
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "bt_driver.c"

// ---------------- stubs ----------------
DaemonConfig g_daemon_config;
PluginConfig g_config;

static uint64_t t_now = 1000000;
static int t_flag_exists = 0;
static int t_fw_resolved = 1;
static uint32_t t_fw_version = 0x12500000;
static int t_kexec_calls = 0;
static struct PatchRequest t_last_patch;


static int t_slot_opens = 0;
static int t_sends = 0;
static OrbisPadData t_last_sent;
static int t_verbose = 0;

uint64_t daemon_sys_time_us(void) { return t_now; }
int daemon_sys_open(const char *path, int flags, int mode) {
    (void)flags; (void)mode;
    if (strstr(path, "switchpro_bt.enable")) return t_flag_exists ? 42 : -1;
    if (strcmp(path, "/dev/bt") == 0) return 5;
    return -1; // status file etc.
}
int daemon_sys_close(int fd) { (void)fd; return 0; }
int daemon_sys_read(int fd, void *buf, size_t count) { (void)fd; (void)buf; (void)count; return 0; }
int daemon_sys_write(int fd, const void *buf, size_t count) { (void)fd; (void)buf; return (int)count; }
int daemon_sys_mkdir(const char *path, int mode) { (void)path; (void)mode; return 0; }
int daemon_kernel_get_handle(void) { return 1; }

typedef struct { uint64_t size; char text[28]; uint32_t version; } SwInfo;
static int fake_get_sw_version(void *p) { ((SwInfo *)p)->version = t_fw_version; return 0; }
int daemon_sys_dlsym(int handle, const char *symbol, void **out_addr) {
    (void)handle; (void)symbol;
    *out_addr = t_fw_resolved ? (void *)fake_get_sw_version : NULL;
    return 0;
}

long daemon_syscall(long num, ...) {
    va_list ap;
    va_start(ap, num);
    long a = va_arg(ap, long);
    long b = va_arg(ap, long);
    va_end(ap);
    if (num == SYS_KEXEC) {
        t_kexec_calls++;
        if ((void *)a == (void *)patch_allowlist) {
            t_last_patch = *(struct PatchRequest *)b;
        }
        return 0; // pretend ring-0 call succeeded, never execute it
    }
    if (num == SYS_KQUEUE) return 6;
    if (num == SYS_IOCTL) {
        if ((unsigned long)b == 0x8004622BUL) {
            struct { uint16_t v, p; } *d = (void *)a;
            (void)d;
        }
        unsigned long req = (unsigned long)b;
        (void)req;
        return 0;
    }
    if (num == 591) return 0;
    return 0;
}
// bt_register_pair passes (fd, request, ptr): re-route through a dedicated hook
long daemon_sys_call6(long number, long a, long b, long c, long d, long e, long f) {
    (void)number; (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return 0; // kevent: no events
}

void daemon_log(int level, const char *fmt, ...) {
    (void)level;
    if (!t_verbose) return;
    va_list ap; va_start(ap, fmt); printf("  [log] "); vprintf(fmt, ap); printf("\n"); va_end(ap);
}
void daemon_notify(const char *fmt, ...) {
    if (!t_verbose) return;
    va_list ap; va_start(ap, fmt); printf("  [notify] "); vprintf(fmt, ap); printf("\n"); va_end(ap);
}

int vpad_slot_open_dev(VPadSource source, uint64_t key, int *out_slot) { (void)source; (void)key; t_slot_opens++; *out_slot = 0; return 0; }
void vpad_slot_close(int slot) { (void)slot; }
int vpad_send_data_slot(int slot, const OrbisPadData *data) { (void)slot; t_sends++; t_last_sent = *data; return 0; }
void vpad_reactivate_slot(int slot) { (void)slot; }
int vpad_get_assigned_slot_num(int slot) { return slot + 1; }
void usb_driver_load_remap_for_bt(void) {}
void usb_driver_apply_bt_remap(OrbisPadData *ds4) { (void)ds4; }
uint8_t translator_apply_deadzone(uint8_t v, uint8_t dz) { (void)dz; return v; }

// ---------------- helpers ----------------
static int failures = 0;
#define CHECK(cond, msg) do { if (cond) { printf("ok   %s\n", msg); } else { printf("FAIL %s  (line %d)\n", msg, __LINE__); failures++; } } while (0)

// Build a /dev/bt style packet: [len][hdr][0x3F b0 b1 hat lx ly rx ry]
static long make_pkt(uint8_t *out, uint8_t b0, uint8_t b1, uint8_t hat,
                     uint16_t lx, uint16_t ly, uint16_t rx, uint16_t ry) {
    uint8_t r[12] = { 0x3F, b0, b1, hat,
        (uint8_t)lx, (uint8_t)(lx >> 8), (uint8_t)ly, (uint8_t)(ly >> 8),
        (uint8_t)rx, (uint8_t)(rx >> 8), (uint8_t)ry, (uint8_t)(ry >> 8) };
    out[0] = 12; out[1] = 0xA1;
    memcpy(out + 2, r, 12);
    return 14;
}

static BtDevice *fresh_switch_dev(void) {
    bt_devices_reset();
    s_switch_mode = true;
    BtDevice *dev = bt_dev_acquire(0xAABBCCDDEEFFULL);
    return dev;
}

int main(void) {
    uint8_t pkt[80];
    memset(&g_daemon_config, 0, sizeof(g_daemon_config));
    g_daemon_config.switch_positional = 1;

    // ---- decoding ----
    BtDevice *dev = fresh_switch_dev();
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_slot_opens == 1 && dev->connected, "first 0x3F report opens a virtual pad slot");
    CHECK(t_last_sent.buttons == 0, "neutral report -> no buttons");
    CHECK(t_last_sent.leftStick.x == 128 && t_last_sent.leftStick.y == 128 &&
          t_last_sent.rightStick.x == 128 && t_last_sent.rightStick.y == 128, "center 0x8000 -> 128");

    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0x01, 0, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_CROSS, "B (bottom) -> Cross (positional)");
    CHECK(t_slot_opens == 1, "no second slot for same pad");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0x02, 0, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_CIRCLE, "A (right) -> Circle");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0x04, 0, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_SQUARE, "Y (left) -> Square");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0x08, 0, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_TRIANGLE, "X (top) -> Triangle");

    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0xF0, 0, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == (DS4_BUTTON_L1 | DS4_BUTTON_R1 | DS4_BUTTON_L2 | DS4_BUTTON_R2) &&
          t_last_sent.analogButtons.l2 == 255 && t_last_sent.analogButtons.r2 == 255,
          "L/R/ZL/ZR -> L1/R1/L2/R2 with full analog triggers");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0x80, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_R2 && t_last_sent.analogButtons.r2 == 255, "ZR alt bit (byte2 0x80) -> R2");

    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0x01, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_SHARE, "Minus -> Share");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0x02, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_OPTIONS, "Plus -> Options");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0x0C, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == (DS4_BUTTON_L3 | DS4_BUTTON_R3), "stick clicks -> L3/R3");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0x10, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_PS, "Home -> PS");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0x20, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_TOUCHPAD, "Capture -> Touchpad");

    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0, 0, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_DPAD_UP, "hat 0 -> Up");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0, 3, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == (DS4_BUTTON_DPAD_DOWN | DS4_BUTTON_DPAD_RIGHT), "hat 3 -> Down+Right");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0, 0x0F, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == 0, "out-of-range hat -> centered");

    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0, 8, 0xC000, 0x4000, 0x8000, 0x8000));
    CHECK(t_last_sent.leftStick.x == 255 && t_last_sent.leftStick.y == 0, "half throw saturates before extents grow (like SDL)");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0, 8, 0xFFFF, 0x0000, 0x8000, 0x8000));
    CHECK(t_last_sent.leftStick.x == 255 && t_last_sent.leftStick.y == 0, "full throw -> 255 / 0");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0, 8, 0xC000, 0x4000, 0x8000, 0x8000));
    CHECK(t_last_sent.leftStick.x >= 189 && t_last_sent.leftStick.x <= 192 &&
          t_last_sent.leftStick.y >= 63 && t_last_sent.leftStick.y <= 65, "after extents grow, half throw -> ~half");
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0, 0, 8, 0x8000, 0xFFFF, 0x8000, 0x8000));
    CHECK(t_last_sent.leftStick.y == 255, "Y raw high -> DS4 down (255)");

    g_daemon_config.switch_positional = 0;
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0x02, 0, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    CHECK(t_last_sent.buttons == DS4_BUTTON_CROSS, "by-letter layout: A -> Cross");
    g_daemon_config.switch_positional = 1;

    // ---- framing / rejection ----
    int sends_before = t_sends;
    uint32_t drops_before = dev->decode_drops;
    uint8_t full[51] = { 49, 0xA1, 0x30 };
    bt_switch_handle_raw(dev, full, 51);
    CHECK(t_sends == sends_before && dev->decode_drops == drops_before + 1, "report 0x30 is ignored, not decoded");
    uint8_t alt[13] = { 12, 0x3F, 0x01, 0, 8, 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80 };
    bt_switch_handle_raw(dev, alt, 13);
    CHECK(t_sends == sends_before + 1 && t_last_sent.buttons == DS4_BUTTON_CROSS, "alternative framing [len][0x3F..] decoded");
    uint8_t shortp[6] = { 4, 0xA1, 0x3F, 0, 0, 8 };
    sends_before = t_sends;
    bt_switch_handle_raw(dev, shortp, 6);
    CHECK(t_sends == sends_before, "truncated packet ignored");

    // ---- held button survives silence (event-driven reports) ----
    s_bt_running = true; s_bt_fd = 5; s_bt_queue = 6;
    bt_switch_handle_raw(dev, pkt, make_pkt(pkt, 0x80, 0, 8, 0x8000, 0x8000, 0x8000, 0x8000));
    t_now += 2000000; // 2 s without any report
    bt_driver_poll_step();
    CHECK((t_last_sent.buttons & DS4_BUTTON_R2) && t_last_sent.analogButtons.r2 == 255,
          "Switch mode: held ZR still pressed after 2 s of silence");
    t_now += 31000000; // > 30 s silence -> away -> neutral
    bt_driver_poll_step();
    bt_driver_poll_step();
    CHECK(t_last_sent.buttons == 0, "after 30 s silence the pad is neutralised (away)");
    CHECK(dev->vpad_slot == 0 && !dev->connected && dev->away_since != 0, "slot held during grace period");
    t_now += 700000000ULL; // 700 s > DualSense grace (600 s) but < Switch grace (3600 s)
    bt_driver_poll_step();
    CHECK(dev->address != 0 && dev->vpad_slot == 0, "Switch grace keeps slot after 700 s");

    // DualSense regression: 500 ms neutralisation still active when not in Switch mode
    bt_devices_reset();
    s_switch_mode = false;
    BtDevice *ds = bt_dev_acquire(0x112233445566ULL);
    ds->vpad_slot = 1; ds->connected = true; ds->last_input_time = t_now;
    ds->last_pad_data.buttons = DS4_BUTTON_CROSS;
    t_now += 600000;
    bt_driver_poll_step();
    CHECK(t_last_sent.buttons == 0, "DualSense mode unchanged: neutralised after 500 ms silence");

    // ---- init: identity chosen for the kernel patch ----
    struct { const char *name; int flag; int resolved; uint32_t fw; int expect_switch; } cases[] = {
        { "no flag, FW 12.50 -> DualSense",             0, 1, 0x12500000, 0 },
        { "flag, FW 12.50 -> Switch Pro",               1, 1, 0x12500000, 1 },
        { "flag, FW 13.52 -> Switch Pro",               1, 1, 0x13520000, 1 },
        { "flag, FW 9.00 -> refused, DualSense",        1, 1, 0x09000000, 0 },
        { "flag, FW 12.00 -> refused, DualSense",       1, 1, 0x12000000, 0 },
        { "flag, FW unknown -> refused, DualSense",     1, 0, 0x12500000, 0 },
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        t_flag_exists = cases[i].flag;
        t_fw_resolved = cases[i].resolved;
        t_fw_version = cases[i].fw;
        s_current_fw_bcd = 0x1250;
        memset(&t_last_patch, 0, sizeof(t_last_patch));
        s_bt_running = false; s_bt_registered = false;
        int rc = bt_driver_init();
        int is_sw = s_switch_mode;
        uint32_t want_vid = cases[i].expect_switch ? 0x057E : 0x054C;
        uint32_t want_pid = cases[i].expect_switch ? 0x2009 : 0x0CE6;
        char msg[160];
        snprintf(msg, sizeof(msg), "init %s (rc=%d, patch %04X:%04X, regs sw=%d ds=%d)",
                 cases[i].name, rc, t_last_patch.new_vid, t_last_patch.new_pid,
                 s_bt_switch_reg_count, s_bt_reg_count);
        CHECK(rc == 0 && is_sw == cases[i].expect_switch &&
              t_last_patch.new_vid == want_vid && t_last_patch.new_pid == want_pid &&
              s_bt_switch_reg_count == (cases[i].expect_switch ? 1 : 0) &&
              s_bt_reg_count == (cases[i].expect_switch ? 0 : 1), msg);
        bt_driver_shutdown();
        CHECK(t_last_patch.restore == 1 && t_last_patch.new_vid == want_vid, "  shutdown restores with the same identity");
    }

    printf("\n%s (%d failure%s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
