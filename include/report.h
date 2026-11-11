#ifndef REPORT_H
#define REPORT_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Screen output. Each function returns STATUS_EMPTY when there is nothing to show.
    Status_t printTags(const Tag_t *tags, int tag_count);
    Status_t printReadings(const Reading_t *readings, int count,
                           const Tag_t *tags, int tag_count);
    Status_t printTagStatistics(const Tag_t *tag, const Reading_t *readings, int count);
    Status_t printAlarmSummary(const Tag_t *tags, int tag_count,
                               const Reading_t *readings, int count);

    // Writes the full shift report (statistics, alarm counts, time in alarm per tag).
    Status_t saveShiftReport(const char *path, const Tag_t *tags, int tag_count,
                             const Reading_t *readings, int count);

    // Fixed texts for the user. Never return NULL.
    const char *statusToMessage(Status_t status);
    const char *alarmStatusToText(AlarmStatus_t status);

#ifdef __cplusplus
}
#endif

#endif // REPORT_H
