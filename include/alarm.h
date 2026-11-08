#ifndef ALARM_H
#define ALARM_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Limits are inclusive: a value equal to a limit triggers that alarm.
    Status_t classifyReading(double value, const Tag_t *tag, AlarmStatus_t *status);

    // Counts the readings of this tag that are not ALARM_NORMAL and multiplies the count by SAMPLE_INTERVAL_SEC. Readings of other tags are ignored.
    Status_t calculateTimeInAlarm(const Reading_t *readings, int count,
                                  const Tag_t *tag, int *seconds);

#ifdef __cplusplus
}
#endif

#endif // ALARM_H