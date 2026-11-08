#include "alarm.h"

#include <stddef.h>
#include <string.h>

Status_t classifyReading(double value, const Tag_t *tag, AlarmStatus_t *status)
{
    if (tag == NULL || status == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (value <= tag->low_low)
    {
        *status = ALARM_LOW_LOW;
    }
    else if (value <= tag->low)
    {
        *status = ALARM_LOW;
    }
    else if (value >= tag->high_high)
    {
        *status = ALARM_HIGH_HIGH;
    }
    else if (value >= tag->high)
    {
        *status = ALARM_HIGH;
    }
    else
    {
        *status = ALARM_NORMAL;
    }

    return STATUS_SUCCESS;
}

Status_t calculateTimeInAlarm(const Reading_t *readings, int count,
                              const Tag_t *tag, int *seconds)
{
    AlarmStatus_t alarm_status;
    int alarm_count = 0;
    int i;

    if (readings == NULL || tag == NULL || seconds == NULL || count < 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < count; i++)
    {
        if (strcmp(readings[i].tag_id, tag->id) != 0)
        {
            continue;
        }

        if (classifyReading(readings[i].value, tag, &alarm_status) != STATUS_SUCCESS)
        {
            return STATUS_INVALID_ARGUMENT;
        }

        if (alarm_status != ALARM_NORMAL)
        {
            alarm_count++;
        }
    }

    *seconds = alarm_count * SAMPLE_INTERVAL_SEC;
    return STATUS_SUCCESS;
}