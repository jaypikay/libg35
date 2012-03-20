#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
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
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_NEXT) < 0)
        return -1;
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_PLAY) < 0)
        return -1;
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_PREVIOUS) < 0)
        return -1;
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_VOLUMEUP) < 0)
        return -1;
    if (ioctl(uinputfd, UI_SET_KEYBIT, KEY_VOLUMEDOWN) < 0)
        return -1;

    memset(&uidev, 0, sizeof(struct uinput_user_dev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "G35 Keys");
    uidev.id.bustype = BUS_USB;
    uidev.id.product = 0x1;
    uidev.id.vendor = 0x1;
    uidev.id.version = 1;

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
        case G35_KEY_G1:
            fprintf(stderr, "G35_KEY_G1 pressed\n");
            return KEY_NEXT;
        case G35_KEY_G2:
            fprintf(stderr, "G35_KEY_G2 pressed\n");
            return KEY_PLAY;
        case G35_KEY_G3:
            fprintf(stderr, "G35_KEY_G3 pressed\n");
            return KEY_PREVIOUS;
        case G35_KEY_VOLDOWN:
            fprintf(stderr, "G35_KEY_VOLDOWN pressed\n");
            return KEY_VOLUMEDOWN;
        case G35_KEY_VOLUP:
            fprintf(stderr, "G35_KEY_VOLUP pressed\n");
            return KEY_VOLUMEUP;
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

    memset(&ev, 0, sizeof(struct input_event));

    for (i = 0; i < G35_KEYS_READ_LENGTH; ++i) {
        if (keys[i] > 0) {
            ev.type = EV_KEY;
            ev.code = key_dispatcher(keys[i]);
            ev.value = 1;
            ret = write(uinputfd, &ev, sizeof(struct input_event));
            fprintf(stderr, "%d ret=%d\n", uinputfd, ret);
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
    return 0;
}
