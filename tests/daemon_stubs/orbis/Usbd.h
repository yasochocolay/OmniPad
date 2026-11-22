/* Desktop stand-in for <orbis/Usbd.h> used by tests/test_switch_bt.c (types only). */
#pragma once
#include <stdint.h>
typedef struct libusb_device libusb_device;
typedef struct libusb_device_handle libusb_device_handle;
struct libusb_device_descriptor { uint8_t bLength, bDescriptorType; uint16_t bcdUSB; uint8_t bDeviceClass, bDeviceSubClass, bDeviceProtocol, bMaxPacketSize0; uint16_t idVendor, idProduct, bcdDevice; uint8_t iManufacturer, iProduct, iSerialNumber, bNumConfigurations; };
struct libusb_config_descriptor;
