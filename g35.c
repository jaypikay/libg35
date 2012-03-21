/*
 *  G35 Library for G35 Daemon
 *  Copyright (C) 2012  Julian Knauer <jpk-at-goatpr0n.de>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <usb.h>

#include "libg35.h"

static G35DeviceRec g35_devices[] = {
    {"Logitech G35 Headset", 0x046d, 0x0a15},
    { },
};

static usb_dev_handle *g35_devh;


static usb_dev_handle *g35_find_device(G35DeviceRec g35dev)
{
    int c, i, a;
    struct usb_bus *bus = 0;
    struct usb_device *dev = 0;
    char name_buffer[65535];
    int ret;

    for (bus = usb_busses; bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {

            if (dev->descriptor.idVendor != g35dev.vendor_id
                    || dev->descriptor.idProduct != g35dev.product_id)
                continue;

            usb_dev_handle *devh = usb_open(dev);
            if (!devh) {
                // TODO notify permission problem
                return NULL;
            }

            for (c = 0; c < dev->descriptor.bNumConfigurations; ++c) {
                struct usb_config_descriptor *cfg = &dev->config[c];

                for (i = 0; i < cfg->bNumInterfaces; ++i) {
                    struct usb_interface *intf = &cfg->interface[i];

                    for (a = 0; a < intf->num_altsetting; ++a) {
                        struct usb_interface_descriptor *idsc = &intf->altsetting[a];

                        if (idsc->bInterfaceClass != USB_CLASS_HID)
                            continue;

                        ret = usb_get_driver_np(devh, i, name_buffer, 65535);
                        if (!ret && name_buffer[0]) {
                            ret = usb_detach_kernel_driver_np(devh, i);
                            if (ret)
                                return NULL;
                        }

                        int retries = 10;
                        while ((ret = usb_claim_interface(devh, i)) != 0
                                && retries-- > 0)
                            usleep(50 * 1000);
                        if (ret != 0)
                            return NULL;

                        return devh;
                    }
                }
            }
        }
    }

    return NULL;
}

static usb_dev_handle *g35_open_device()
{
    int i;

    for (i = 0; g35_devices[i].name != NULL; i++) {
        g35_devh = g35_find_device(g35_devices[i]);

        if (g35_devh)
            break;
    }

    return g35_devh;
}

int g35_init_usb()
{
    usb_init();

    if (!usb_find_busses())
        return G35_OPEN_ERROR;
    if (!usb_find_devices())
        return G35_OPEN_ERROR;

    g35_devh = g35_open_device();
    if (!g35_devh)
        return G35_OPEN_ERROR;

    return G35_OK;
}

int g35_init()
{
    int ret = g35_init_usb();

    return ret;
}

void g35_destroy()
{
    if (g35_devh != NULL) {
        usb_reset(g35_devh);
        usleep(10000);
        usb_close(g35_devh);
        g35_devh = 0;
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

int g35_keypressed(unsigned int *pressed_keys, unsigned int timeout)
{
    unsigned char buffer[G35_KEYS_READ_LENGTH];
    int tx = 0;

    tx = usb_interrupt_read(g35_devh, G35_KEYS_ENDPOINT | USB_ENDPOINT_IN,
            (char *)buffer, G35_KEYS_READ_LENGTH, timeout);
    processG35KeyPressEvent(pressed_keys, buffer);

    return tx;
}
