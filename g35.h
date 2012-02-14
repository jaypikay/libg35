#ifndef _G35_H_
#define _G35_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define LIBG35_VERSION_MAJOR    0
#define LIBG35_VERSION_MINOR    1
#define LIBG35_VERSION_BUILD    0

typedef struct {
    char *name;
    uint16_t vendor_id;
    uint16_t product_id;
} G35DeviceRec, *G35DevicePtr;

enum G35Keys {
    G35_KEY_G1                  = 1 << 0,
    G35_KEY_G2                  = 1 << 1,
    G35_KEY_G3                  = 1 << 2
};

int g35_init();
void g35_destroy();

#ifdef __cplusplus
}
#endif

#endif /* _G35_H_ */
