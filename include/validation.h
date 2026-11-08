#ifndef VALIDATION_H
#define VALIDATION_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Format checks (uniqueness of a tag ID is checked by the tag module).
    Status_t validateTagId(const char *id);
    Status_t validateTimestamp(const char *timestamp);
    Status_t validateAlarmLimits(double low_low, double low, double high, double high_high);
    Status_t validateDescription(const char *text);
    Status_t validateUnit(const char *text);

    // Text-to-number conversion. *value / *choice are written only on success.
    Status_t parseDouble(const char *text, double *value);
    Status_t validateMenuChoice(const char *text, int min, int max, int *choice);

    // Safe line input: reads one line from stdin without the newline.
    Status_t readLine(char *buffer, int size);

#ifdef __cplusplus
}
#endif

#endif // VALIDATION_H