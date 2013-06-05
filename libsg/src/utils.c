#include <sg/utils.h>

bool readUInt32le(FILE *f, uint32_t *v) {
    uint8_t bytes[4];
    int n = fread(bytes, 4, 1, f);
    if (n == 0)
        return false;
    *v = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
    return true;
}

bool readUInt16le(FILE *f, uint16_t *v) {
    uint8_t bytes[2];
    int n = fread(bytes, 2, 1, f);
    if (n == 0)
        return false;
    *v = bytes[0] | (bytes[1] << 8);
    return true;
};

bool readUInt8le(FILE *f, uint8_t *v) {
    int n = fread(v, 1, 1, f);
    return n != 0;
}

bool readInt32le(FILE *f, int32_t *v) {
    return readUInt32le(f, (uint32_t*)(v));
}

bool readInt16le(FILE *f, int16_t *v) {
    return readUInt16le(f, (uint16_t*)(v));
}
bool readInt8le(FILE *f, int8_t *v) {
    return readUInt8le(f, (uint8_t*)(v));
}
