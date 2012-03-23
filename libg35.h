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

#ifndef _LIBG35_H_
#define _LIBG35_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define DEBUG_PRINTF()          fprintf(stderr, "%s:%d\n", __FILE__, __LINE__)

    /* Number of different G35 keys available
     * G1, G2, G3, Volumne Down/Up, Mute (same as microphone arm)
     */
#define G35_MAX_KEYS            6

#define LIBG35_VERSION          0x0110

#define G35_DEV_FOUND           0
#define G35_DEV_NOT_FOUND       1

#define G35_OK                  0
#define G35_OPEN_ERROR          1
#define G35_CLAIM_ERROR         2
#define G35_UINPUT_ERROR        3

#define G35_KEYS_ENDPOINT       3
#define G35_KEYS_READ_LENGTH    5

#define G35_EVENT_KEY           1
#define G35_EVENT_MIC           2

#define KEY_PRESSED             1
#define KEY_RELEASED            0

    typedef struct {
        char *name;
        uint16_t vendor_id;
        uint16_t product_id;
    } G35DeviceRec, *G35DevicePtr;

    enum G35Keys {
        G35_KEY_VOLUP               = 1 << 0x00,
        G35_KEY_VOLDOWN             = 1 << 0x01,

        G35_KEY_G1                  = 1 << 0x02,
        G35_KEY_G2                  = 1 << 0x03,
        G35_KEY_G3                  = 1 << 0x04,
    };

    enum G35Microphone {
        G35_MIC_UNMUTE              = 0x05,
        G35_MIC_MUTE                = 0x15,
    };

    int g35_init();
    void g35_destroy();
    int g35_keypressed(unsigned int *pressed_keys, unsigned int timeout);

    /* uinput */
    int g35_uinput_update_keymap(unsigned int *keymap);
    int g35_uinput_init(const char *udev, unsigned int *keymap);
    int g35_uinput_destroy();
    int g35_uinput_write(unsigned int *keys);

#ifdef __cplusplus
}
#endif

#endif /* _LIBG35_H_ */
