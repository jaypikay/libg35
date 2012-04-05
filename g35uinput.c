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

/** \file g35uinput.c
 * UINPUT library functions for key event injection.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include "libg35.h"

static int uinputfd = 0;

static unsigned int g35_key_g1 = 0;
static unsigned int g35_key_g2 = 0;
static unsigned int g35_key_g3 = 0;
static unsigned int g35_key_voldn = 0;
static unsigned int g35_key_volup = 0;


/** \fn int g35_uinput_update_keymap(unsigned int *keymap)
 * \brief Re-assign button events with uinput key events.
 *
 * The G35 buttons are mapped to a uinput key event.
 *
 * \param       keymap      keymap will store the new assigned key event codes
 *
 * \return G35_OK on success
 * \return G35_UINPUT_ERROR on error
 */
int g35_uinput_update_keymap(unsigned int *keymap)
{
    struct uinput_user_dev uidev;
    int ret = 0, i;

    if (!uinputfd || !keymap)
        return G35_UINPUT_ERROR;

    g35_key_g1 = keymap[0];
    g35_key_g2 = keymap[1];
    g35_key_g3 = keymap[2];
    g35_key_voldn = keymap[3];
    g35_key_volup = keymap[4];

    if (ioctl(uinputfd, UI_SET_EVBIT, EV_KEY) < 0)
        return G35_UINPUT_ERROR;

    /* Enable all keys */
    for (i = 0; i < 0xff; ++i) {
        if (ioctl(uinputfd, UI_SET_KEYBIT, i) < 0)
            return G35_UINPUT_ERROR;
    }
    /* Set the actual keymap */
    for (i = 0; i < G35_MAX_KEYS; ++i) {
        if (ioctl(uinputfd, UI_SET_KEYBIT, keymap[i]) < 0)
            return G35_UINPUT_ERROR;
    }

    memset(&uidev, 0, sizeof(struct uinput_user_dev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Logitech G35 Headset Buttons");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 4;

    ret = write(uinputfd, &uidev, sizeof(struct uinput_user_dev));
    if (ret < sizeof(struct uinput_user_dev))
        return G35_UINPUT_ERROR;

    return G35_OK;
}

int g35_uinput_init(const char *udev, unsigned int *keymap)
{
    if ((uinputfd = open(udev, O_WRONLY | O_NONBLOCK)) < 0)
        return G35_UINPUT_ERROR;

    if (g35_uinput_update_keymap(keymap) != G35_OK)
        return G35_UINPUT_ERROR;

    if (ioctl(uinputfd, UI_DEV_CREATE) < 0)
        return G35_UINPUT_ERROR;
    
    return G35_OK;
}

static unsigned int key_dispatcher(unsigned int key)
{
    switch (key) {
        case G35_KEY_VOLDOWN:
            return g35_key_voldn;
        case G35_KEY_VOLUP:
            return g35_key_volup;
        case G35_KEY_G1:
            return g35_key_g1;
        case G35_KEY_G2:
            return g35_key_g2;
        case G35_KEY_G3:
            return g35_key_g3;
    }

    return G35_OK;
}

static int write_keypress(unsigned int key, unsigned char key_state)
{
    struct input_event ev;
    int ret;

    memset(&ev, 0, sizeof(struct input_event));
    ev.type = EV_KEY;
    ev.code = key;
    ev.value = key_state;
    gettimeofday(&ev.time, NULL);
    ret = write(uinputfd, &ev, sizeof(struct input_event));

    if (ret) {
        ev.type = EV_SYN;
        ev.code = SYN_REPORT;
        ev.value = 0;
        write(uinputfd, &ev, sizeof(struct input_event));
    } else {
        return G35_UINPUT_ERROR;
    }

    return G35_OK;
}

/** \fn int g35_uinput_write(unsigned int *keys)
 * \brief write key press event to uinput device.
 *
 * The read button events from the USB device are translated into uinput key
 * events using the keymap stored in keys, defined by
 * g35_uinput_update_keymap() and will write them to the UINPUT device.
 *
 * \return G35_OK on success
 */
int g35_uinput_write(unsigned int *keys)
{
    int i;

    for (i = 0; i < G35_KEYS_READ_LENGTH; ++i) {
        if (keys[i] > 0) {
            unsigned int key = key_dispatcher(keys[i]);
            write_keypress(key, KEY_PRESSED);
            write_keypress(key, KEY_RELEASED);
        }
    }

    return G35_OK;
}

/** \fn int g35_uinput_destroy()
 * \brief Close the uinput device.
 *
 * Closes the open uinput device handle and resets interal resources.
 *
 * \return G35_OK on success
 * \return G35_UINPUT_ERROR on error
 */
int g35_uinput_destroy()
{
    if (!uinputfd)
        return G35_UINPUT_ERROR;
    if (ioctl(uinputfd, UI_DEV_DESTROY) < 0)
        return G35_UINPUT_ERROR;
    close(uinputfd);
    uinputfd = 0;

    return G35_OK;
}
