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

/** \file g35.c
 * \brief G35 support library to communicate with the device.
 *
 * The libg35 provides methods to handle the G35 USB interface. In order to use
 * the G35 headset. The device itself has to be initialised and the USB
 * endpoint needs to be claimed. This is done be calling g35_init(). All events
 * by the G35 HID are handled by g35_keypressed(). To free the resources and
 * close the connection to the G35 the function g35_destroy() is called.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <usb.h>

#include "libg35.h"

/** Known and supported devices by libg35 */
static G35DeviceRec g35_devices[] = {
    {"Logitech G35 Headset", 0x046d, 0x0a15},
    { },
};

static usb_dev_handle *g35_devh; //! G35 USB device handle


/** \fn usb_dev_handle *g35_find_device(G35DeviceRec g35dev)
 * \brief Locate and claim G35 USB HID.
 *
 * This function will walk through all connected USB devices to identify the
 * first supported device listed in g35_devices.
 * \sa g35_devices
 *
 * \param[in]   g35dev      G35 device record of supported device
 *
 * \return usb_dev_handle on success
 * \return NULL on error
 */
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

                        /// Ignore the device if it is not of class HID.
                        if (idsc->bInterfaceClass != USB_CLASS_HID)
                            continue;

                        /// If the device is attached to the kernel, detach it.
                        ret = usb_get_driver_np(devh, i, name_buffer, 65535);
                        if (!ret && name_buffer[0]) {
                            ret = usb_detach_kernel_driver_np(devh, i);
                            if (ret)
                                return NULL;
                        }

                        /// Try to claim the device 10 times, otherwise abort.
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

/** \fn usb_dev_handle *g35_open_device()
 * \brief open usb device
 *
 * Will try to locate and open the USB device and return the result of
 * g35_find_device().
 *
 * \return usb_dev_handle on success
 * \return NULL on error
 */
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

/** \fn int g35_init_usb()
 * \brief Helper function, called by g35_init().
 *
 * Initialize USB library and claim device.
 *
 * \return G35_OK on success
 * \return G35_OPEN_ERROR on error
 */
static int g35_init_usb()
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

/** \fn int g35_init()
 * \brief Initializes the first supported USB device.
 *
 * This function takes no parameters and will initialise the G35 support
 * library. The first supported device is opened and claimed by the library. To
 * free and close the claimed device g35_destroy() is called.
 * 
 * \sa g35_destroy()
 *
 * \return G35_OK on success
 * \return G35_OPEN_ERROR on error
 */
int g35_init()
{
    int ret = g35_init_usb();

    return ret;
}

/** \fn void g35_destroy()
 * \brief Free and close the claimed USB device.
 *
 * To clean up the library resources and make the device end point accassible
 * for other programs again this function will close all open handles and
 * resets the device.
 */
void g35_destroy()
{
    if (g35_devh != NULL) {
        usb_close(g35_devh);
        g35_devh = 0;
    }
}

/** \fn void processG35KeyPressEvent(unsigned int *pressed_keys, unsigned char *buffer)
 * \brief Convert the raw input buffer into a bit map of button events.
 *
 * Reads the input buffer and extracts the information about all pressed
 * buttons. The information is stored as bitmap in pressed_keys.
 *
 * \param[out]  pressed_keys    store the pressed buttons
 * \param[in]   buffer          raw button event buffer read from USB device
 */
static void processG35KeyPressEvent(unsigned int *pressed_keys,
        unsigned char *buffer)
{
    *pressed_keys = 0;
    if (buffer[0] == G35_EVENT_KEY) {
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
    if (buffer[0] == G35_EVENT_MIC) {
        if ((buffer[2] & 0x05) == G35_MIC_UNMUTE)
            *pressed_keys |= G35_MIC_UNMUTE;
        if ((buffer[2] & 0x15) == G35_MIC_MUTE)
            *pressed_keys |= G35_MIC_MUTE;
    }
}

/** \fn int g35_keypressed(unsigned int *pressed_keys, unsigned int timeout)
 * \brief Read raw button events and convert them into a key press event map.
 *
 * This function will read the raw key press events send by the G35 buttons and
 * converts them into keypress events libg35 can handle. The read buffer can
 * hold up to G35_MAX_KEYS events.
 *
 * \param[out]  pressed_keys    store the pressed buttons
 * \param[in]   timeout         milliseconds until read will timeout
 *
 * \return tx number of bytes read from the USB interface
 */
int g35_keypressed(unsigned int *pressed_keys, unsigned int timeout)
{
    unsigned char buffer[G35_MAX_KEYS];
    int tx = 0;

    tx = usb_interrupt_read(g35_devh, G35_KEYS_ENDPOINT | USB_ENDPOINT_IN,
            (char *)buffer, G35_KEYS_READ_LENGTH, timeout);
    processG35KeyPressEvent(pressed_keys, buffer);

    return tx;
}
