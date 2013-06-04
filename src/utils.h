#ifndef SG_UTILS_H
#define SG_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

bool readUInt32le(FILE *f, uint32_t *v);
bool readUInt16le(FILE *f, uint16_t *v);
bool readUInt8le(FILE *f, uint8_t *v);
bool readInt32le(FILE *f, int32_t *v);
bool readInt16le(FILE *f, int16_t *v);
bool readInt8le(FILE *f, int8_t *v);

#ifdef __cplusplus
}
#endif

#endif
