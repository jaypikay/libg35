#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#include "g35.h"

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
