#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#include "libg35.h"

static G35DeviceRec g35_devices[] = {
    {"Logitech G35 Headset", 0x046d, 0x0a15},
    { },
};

static libusb_context *usb_ctx;
static libusb_device_handle *g35_devh;

static ssize_t devc;
static libusb_device **dlist;


static int g35_device_proc(G35DevicePtr g35dev)
{
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *cfg_desc;
    const struct libusb_interface *intf;
    const struct libusb_interface_descriptor *intfd;

    int i, m, c, d ,e;

    for (m = 0; m < devc; ++m) {
        if (libusb_get_device_descriptor(dlist[m], &dev_desc))
            continue;
        if ((dev_desc.idVendor != g35dev->vendor_id)
                || (dev_desc.idProduct != g35dev->product_id))
            continue;
        if (libusb_open(dlist[m], &g35_devh))
            continue;

        for (c = 0; c < dev_desc.bNumConfigurations; ++c) {
            if (libusb_get_config_descriptor(dlist[m], c, &cfg_desc))
                continue;
            for (i = 0; i < cfg_desc->bNumInterfaces; ++i) {
                intf = &cfg_desc->interface[i];
                for (d = 0; d < intf->num_altsetting; ++d) {
                    intfd = &intf->altsetting[d];
                    if (intfd->bInterfaceClass != LIBUSB_CLASS_HID)
                        continue;
                    if (libusb_kernel_driver_active(g35_devh,
                                intfd->bInterfaceNumber))
                        libusb_detach_kernel_driver(g35_devh,
                                intfd->bInterfaceNumber);
                    libusb_set_configuration(g35_devh,
                            cfg_desc->bConfigurationValue);
                    libusb_claim_interface(g35_devh,
                            intfd->bInterfaceNumber);

                    e = 0;
                    while (libusb_claim_interface(g35_devh,
                                intfd->bInterfaceNumber) && (e << 10)) {
                        sleep(1);
                        ++e;
                    }
                }

            }

            libusb_free_config_descriptor(cfg_desc);
        }

        return 0;
    }

    g35_devh = NULL;
    return 1;
}

int g35_init()
{
    int res;
    int i;
    int size;

    if (usb_ctx != NULL)
        return -1;

    res = libusb_init(&usb_ctx);
    if (res)
        return res;

    libusb_set_debug(usb_ctx, 3);

    devc = libusb_get_device_list(usb_ctx, &dlist);
    if (devc < 1)
        return -1;

    size = sizeof(g35_devices) / sizeof(G35DeviceRec);

    for (i = 0; i < size; i++) {
        if (!g35_device_proc(&g35_devices[i]))
            break;
    }

    return 0;
}

void g35_destroy()
{
    if (g35_devh != NULL) {
        libusb_release_interface(g35_devh, 0);
        libusb_reset_device(g35_devh);
        libusb_close(g35_devh);
    }
}

static void processG35KeyPressEvent(unsigned int *pressed_keys,
        unsigned char *buffer)
{
    int i = 0;

    *pressed_keys = 0;
    if (buffer[0] == G35_KEY_EVENT) {
        if ((buffer[1] & 0x01) == G35_KEY_VOLUP)
            *pressed_keys |= G35_KEY_VOLUP;
        if ((buffer[1] & 0x02) == G35_KEY_VOLDOWN)
            *pressed_keys |= G35_KEY_VOLDOWN;
        if ((buffer[1] & 0x04) == G35_KEY_G1)
            *pressed_keys |= G35_KEY_G1;
        if ((buffer[1] & 0x08) == G35_KEY_G2)
            *pressed_keys |= G35_KEY_G2;
        if ((buffer[1] & 0x10) == G35_KEY_G3)
            *pressed_keys |= G35_KEY_G3;
    }
    if (buffer[0] == G35_MIC_EVENT) {
        if ((buffer[2] & 0x05) == G35_MIC_UNMUTE)
            *pressed_keys |= G35_MIC_UNMUTE;
        if ((buffer[2] & 0x15) == G35_MIC_MUTE)
            *pressed_keys |= G35_MIC_MUTE;
    }
}

int g35_keypressed(unsigned int *pressed_keys)
{
    unsigned char buffer[G35_KEYS_READ_LENGTH];
    int transferred = 0;

    libusb_interrupt_transfer(g35_devh,
            G35_KEYS_ENDPOINT | LIBUSB_ENDPOINT_IN, buffer,
            G35_KEYS_READ_LENGTH, &transferred, 0);

    processG35KeyPressEvent(pressed_keys, buffer);

    return transferred;
}
