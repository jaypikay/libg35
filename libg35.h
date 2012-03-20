#ifndef _LIBG35_H_
#define _LIBG35_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define DEBUG_PRINTF()          fprintf(stderr, "%s:%d\n", __FILE__, __LINE__)

#define LIBG35_VERSION_MAJOR    0
#define LIBG35_VERSION_MINOR    1
#define LIBG35_VERSION_BUILD    0

#define G35_DEV_FOUND           0
#define G35_DEV_NOT_FOUND       1

#define G35_OK                  0
#define G35_OPEN_ERROR          1
#define G35_CLAIM_ERROR         2

#define G35_KEY_EVENT           1
#define G35_KEYS_ENDPOINT       3
#define G35_KEYS_READ_LENGTH    5

#define G35_MIC_EVENT           2

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
static int uinputfd = 0;

int g35_uinput_init(const char *udev);
int g35_uinput_destroy();
int g35_uinput_write(unsigned int *keys);

#ifdef __cplusplus
}
#endif

#endif /* _LIBG35_H_ */
