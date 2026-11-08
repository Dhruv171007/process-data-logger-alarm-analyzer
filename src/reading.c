#include "reading.h"

#include "alarm.h"
#include "tag.h"
#include "validation.h"

#include <stddef.h>
#include <string.h>

// Compares two readings by the chosen field and order.
// Returns a positive number when 'a' must come after 'b'.
static int compareReadings(const Reading_t *a, const Reading_t *b,
                           SortField_t field, SortOrder_t order)
{
    int result;

    if (field == SORT_BY_VALUE)
    {
        result = (a->value > b->value) - (a->value < b->value);
    }
    else
    {
        // The timestamp format sorts correctly as plain text.
        result = strcmp(a->timestamp, b->timestamp);
    }

    return (order == SORT_DESCENDING) ? -result : result;
}

Status_t addReading(Reading_t *readings, int *reading_count,
                    const Tag_t *tags, int tag_count,
                    const char *tag_id, const char *timestamp, double value)
{
    Status_t status;
    int tag_index;

    if (readings == NULL || reading_count == NULL || tags == NULL ||
        tag_id == NULL || timestamp == NULL ||
        *reading_count < 0 || *reading_count > MAX_READINGS || tag_count < 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    status = validateTimestamp(timestamp);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    // Written as "not (in range)" so that NaN is rejected as well.
    if (!(value >= MIN_VALID_VALUE && value <= MAX_VALID_VALUE))
    {
        return STATUS_INVALID_INPUT;
    }

    status = findTagById(tags, tag_count, tag_id, &tag_index);
    if (status != STATUS_SUCCESS)
    {
        return status;                 // STATUS_NOT_FOUND for an unknown tag
    }

    if (*reading_count >= MAX_READINGS)
    {
        return STATUS_CAPACITY_FULL;
    }

    // Both strings are known to fit: the tag ID comes from a stored tag and the
    // timestamp was validated to be exactly TIMESTAMP_LEN characters.
    strcpy(readings[*reading_count].tag_id, tags[tag_index].id);
    strcpy(readings[*reading_count].timestamp, timestamp);
    readings[*reading_count].value = value;
    (*reading_count)++;

    return STATUS_SUCCESS;
}

Status_t deleteReadingsByTag(Reading_t *readings, int *reading_count,
                             const char *tag_id, int *removed)
{
    int read_index;
    int write_index = 0;

    if (readings == NULL || reading_count == NULL || tag_id == NULL ||
        removed == NULL || *reading_count < 0 || *reading_count > MAX_READINGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    for (read_index = 0; read_index < *reading_count; read_index++)
    {
        if (strcmp(readings[read_index].tag_id, tag_id) != 0)
        {
            if (write_index != read_index)
            {
                readings[write_index] = readings[read_index];
            }
            write_index++;
        }
    }

    *removed = *reading_count - write_index;
    *reading_count = write_index;
    return STATUS_SUCCESS;
}

Status_t filterReadings(const Reading_t *readings, int count,
                        const Tag_t *tags, int tag_count,
                        const ReadingFilter_t *filter,
                        Reading_t *out, int *out_count)
{
    int has_start;
    int has_end;
    int matched = 0;
    int tag_index;
    int i;
    AlarmStatus_t alarm_status;

    if (readings == NULL || tags == NULL || filter == NULL ||
        out == NULL || out_count == NULL ||
        count < 0 || count > MAX_READINGS || tag_count < 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    has_start = (filter->start_time[0] != '\0');
    has_end   = (filter->end_time[0] != '\0');

    if (has_start && validateTimestamp(filter->start_time) != STATUS_SUCCESS)
    {
        return STATUS_INVALID_INPUT;
    }
    if (has_end && validateTimestamp(filter->end_time) != STATUS_SUCCESS)
    {
        return STATUS_INVALID_INPUT;
    }
    if (has_start && has_end && strcmp(filter->start_time, filter->end_time) > 0)
    {
        return STATUS_INVALID_INPUT;
    }
    if (filter->filter_by_status &&
        ((int)filter->status < (int)ALARM_NORMAL || (int)filter->status > (int)ALARM_HIGH_HIGH))
    {
        return STATUS_INVALID_INPUT;
    }

    for (i = 0; i < count; i++)
    {
        if (filter->tag_id[0] != '\0' && strcmp(readings[i].tag_id, filter->tag_id) != 0)
        {
            continue;
        }
        if (has_start && strcmp(readings[i].timestamp, filter->start_time) < 0)
        {
            continue;
        }
        if (has_end && strcmp(readings[i].timestamp, filter->end_time) > 0)
        {
            continue;
        }

        if (filter->filter_by_status)
        {
            // A reading whose tag no longer exists cannot be classified: skip it.
            if (findTagById(tags, tag_count, readings[i].tag_id, &tag_index) != STATUS_SUCCESS)
            {
                continue;
            }
            if (classifyReading(readings[i].value, &tags[tag_index], &alarm_status) != STATUS_SUCCESS)
            {
                continue;
            }
            if (alarm_status != filter->status)
            {
                continue;
            }
        }

        out[matched] = readings[i];
        matched++;
    }

    *out_count = matched;
    return STATUS_SUCCESS;
}

Status_t sortReadings(Reading_t *readings, int count,
                      SortField_t field, SortOrder_t order)
{
    Reading_t key;
    int i;
    int j;

    if (readings == NULL || count < 0 ||
        (field != SORT_BY_VALUE && field != SORT_BY_TIMESTAMP) ||
        (order != SORT_ASCENDING && order != SORT_DESCENDING))
    {
        return STATUS_INVALID_ARGUMENT;
    }

    for (i = 1; i < count; i++)
    {
        key = readings[i];
        j = i - 1;

        // Only move items that are strictly greater, which keeps the sort stable.
        while (j >= 0 && compareReadings(&readings[j], &key, field, order) > 0)
        {
            readings[j + 1] = readings[j];
            j--;
        }
        readings[j + 1] = key;
    }

    return STATUS_SUCCESS;
}

Status_t getValuesForTag(const Reading_t *readings, int count,
                         const char *tag_id, double *values, int *value_count)
{
    int found = 0;
    int i;

    if (readings == NULL || tag_id == NULL || values == NULL ||
        value_count == NULL || count < 0 || count > MAX_READINGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < count; i++)
    {
        if (strcmp(readings[i].tag_id, tag_id) == 0)
        {
            values[found] = readings[i].value;
            found++;
        }
    }

    if (found == 0)
    {
        return STATUS_EMPTY;
    }

    *value_count = found;
    return STATUS_SUCCESS;
}