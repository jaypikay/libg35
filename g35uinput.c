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


int g35_uinput_init(const char *udev)
{
    struct uinput_user_dev uidev;
    int ret;

    if ((uinputfd = open(udev, O_WRONLY | O_NONBLOCK)) < 0) {
        /* Unable to open uinput device */
        return -1;
    }

    if (ioctl(uinputfd, UI_SET_EVBIT, EV_KEY) < 0)
        return -1;
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_VOLUMEDOWN) < 0)
        return -1;
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_VOLUMEUP) < 0)
        return -1;
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_NEXTSONG) < 0)
        return -1;
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_PLAYPAUSE) < 0)
        return -1;
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_PREVIOUSSONG) < 0)
        return -1;

    memset(&uidev, 0, sizeof(struct uinput_user_dev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "G35 Keys");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 4;

    ret = write(uinputfd, &uidev, sizeof(struct uinput_user_dev));
    if (ret < sizeof(struct uinput_user_dev))
        return -1;

    if (ioctl(uinputfd, UI_DEV_CREATE) < 0)
        return -1;
    
    return 0;
}

static unsigned int key_dispatcher(unsigned int key)
{
    // TODO: dynamicly assigned keys
    switch (key) {
        case G35_KEY_VOLDOWN:
            fprintf(stderr, "G35_KEY_VOLDOWN pressed\n");
            return KEY_VOLUMEDOWN;
        case G35_KEY_VOLUP:
            fprintf(stderr, "G35_KEY_VOLUP pressed\n");
            return KEY_VOLUMEUP;
        case G35_KEY_G1:
            fprintf(stderr, "G35_KEY_G1 pressed\n");
            return KEY_NEXTSONG;
        case G35_KEY_G2:
            fprintf(stderr, "G35_KEY_G2 pressed\n");
            return KEY_PLAYPAUSE;
        case G35_KEY_G3:
            fprintf(stderr, "G35_KEY_G3 pressed\n");
            return KEY_PREVIOUSSONG;
        default:
            return 0;
    }

    return 0;
}

int g35_uinput_write(unsigned int *keys)
{
    struct input_event ev;
    int ret = 0;
    int i;

    for (i = 0; i < G35_KEYS_READ_LENGTH; ++i) {
        if (keys[i] > 0) {
            unsigned int key = key_dispatcher(keys[i]);
            fprintf(stderr, "key_code = %d\n", key);

            memset(&ev, 0, sizeof(struct input_event));
            ev.type = EV_KEY;
            ev.code = key;
            ev.value = 1;
            gettimeofday(&ev.time, NULL);
            ret = write(uinputfd, &ev, sizeof(struct input_event));

            ev.type = EV_SYN;
            ev.code = SYN_REPORT;
            ev.value = 0;
            ret = write(uinputfd, &ev, sizeof(struct input_event));

            memset(&ev, 0, sizeof(struct input_event));
            ev.type = EV_KEY;
            ev.code = key;
            ev.value = 0;
            gettimeofday(&ev.time, NULL);
            ret = write(uinputfd, &ev, sizeof(struct input_event));

            ev.type = EV_SYN;
            ev.code = SYN_REPORT;
            ev.value = 0;
            ret = write(uinputfd, &ev, sizeof(struct input_event));
        }
    }

    return ret;
}

int g35_uinput_destroy()
{
    if (!uinputfd)
        return -1;
    if (ioctl(uinputfd, UI_DEV_DESTROY) < 0)
        return -1;
    close(uinputfd);
    return 0;
}
