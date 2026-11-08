#ifndef READING_H
#define READING_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Checks timestamp, value, tag and capacity, then appends the reading.
    Status_t addReading(Reading_t *readings, int *reading_count,
                        const Tag_t *tags, int tag_count,
                        const char *tag_id, const char *timestamp, double value);

    // Removes every reading of the tag. *removed receives how many were deleted.
    Status_t deleteReadingsByTag(Reading_t *readings, int *reading_count,
                                 const char *tag_id, int *removed);

    // Copies the readings that match the filter into 'out' (room for MAX_READINGS). An empty tag_id / start_time / end_time means "no restriction".
    Status_t filterReadings(const Reading_t *readings, int count,
                            const Tag_t *tags, int tag_count,
                            const ReadingFilter_t *filter,
                            Reading_t *out, int *out_count);

    // Stable insertion sort, in place.
    Status_t sortReadings(Reading_t *readings, int count,
                          SortField_t field, SortOrder_t order);

    // Copies the values of one tag into a plain array (room for MAX_READINGS).Returns STATUS_EMPTY when the tag has no readings.
    Status_t getValuesForTag(const Reading_t *readings, int count,
                             const char *tag_id, double *values, int *value_count);

#ifdef __cplusplus
}
#endif

#endif // READING_H