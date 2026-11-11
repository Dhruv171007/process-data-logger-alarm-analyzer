#include "report.h"

#include "alarm.h"
#include "reading.h"
#include "statistics.h"
#include "tag.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ALARM_STATE_COUNT 5

// Internal helpers (not part of the public interface)

// Prints minimum, maximum, mean and standard deviation of an array of values.
static void writeStatisticsLines(FILE *out, const char *unit,
                                 const double *values, int value_count)
{
    double minimum = 0.0;
    double maximum = 0.0;
    double mean = 0.0;
    double std_dev = 0.0;

    if (calculateMin(values, value_count, &minimum) != STATUS_SUCCESS ||
        calculateMax(values, value_count, &maximum) != STATUS_SUCCESS ||
        calculateMean(values, value_count, &mean) != STATUS_SUCCESS ||
        calculateStdDev(values, value_count, &std_dev) != STATUS_SUCCESS)
    {
        fprintf(out, "  Statistics could not be calculated\n");
        return;
    }

    fprintf(out, "  Minimum  : %.2f %s\n", minimum, unit);
    fprintf(out, "  Maximum  : %.2f %s\n", maximum, unit);
    fprintf(out, "  Mean     : %.2f %s\n", mean, unit);
    fprintf(out, "  Std dev  : %.2f %s\n", std_dev, unit);
}

// One tag's summary. Used by printAlarmSummary (without statistics) and by
// saveShiftReport (with statistics), so the logic is written only once.
static void writeTagSummary(FILE *out, const Tag_t *tag,
                            const Reading_t *readings, int count, int with_statistics)
{
    double values[MAX_READINGS];
    int value_count = 0;
    int alarm_counts[ALARM_STATE_COUNT] = {0, 0, 0, 0, 0};
    int seconds = 0;
    AlarmStatus_t alarm_status;
    int i;

    fprintf(out, "Tag %s - %s (%s)\n", tag->id, tag->description, tag->unit);

    if (getValuesForTag(readings, count, tag->id, values, &value_count) != STATUS_SUCCESS)
    {
        fprintf(out, "  No readings\n\n");
        return;
    }

    for (i = 0; i < count; i++)
    {
        if (strcmp(readings[i].tag_id, tag->id) == 0 &&
            classifyReading(readings[i].value, tag, &alarm_status) == STATUS_SUCCESS)
        {
            alarm_counts[alarm_status]++;
        }
    }

    fprintf(out, "  Readings : %d\n", value_count);

    if (with_statistics)
    {
        writeStatisticsLines(out, tag->unit, values, value_count);
    }

    fprintf(out, "  Normal: %d  Low: %d  Low-Low: %d  High: %d  High-High: %d\n",
            alarm_counts[ALARM_NORMAL], alarm_counts[ALARM_LOW],
            alarm_counts[ALARM_LOW_LOW], alarm_counts[ALARM_HIGH],
            alarm_counts[ALARM_HIGH_HIGH]);

    if (calculateTimeInAlarm(readings, count, tag, &seconds) == STATUS_SUCCESS)
    {
        fprintf(out, "  Time in alarm : %d s\n", seconds);
    }

    fprintf(out, "\n");
}

// Screen output

Status_t printTags(const Tag_t *tags, int tag_count)
{
    int i;

    if (tags == NULL || tag_count < 0 || tag_count > MAX_TAGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (tag_count == 0)
    {
        return STATUS_EMPTY;
    }

    printf("\n%-10s %-30s %-8s %9s %9s %9s %9s\n",
           "Tag ID", "Description", "Unit", "Low-Low", "Low", "High", "High-High");
    printf("------------------------------------------------------------------------------------\n");

    for (i = 0; i < tag_count; i++)
    {
        printf("%-10s %-30.30s %-8s %9.2f %9.2f %9.2f %9.2f\n",
               tags[i].id, tags[i].description, tags[i].unit,
               tags[i].low_low, tags[i].low, tags[i].high, tags[i].high_high);
    }

    printf("\n%d tag(s)\n", tag_count);
    return STATUS_SUCCESS;
}

Status_t printReadings(const Reading_t *readings, int count,
                       const Tag_t *tags, int tag_count)
{
    AlarmStatus_t alarm_status;
    const char *status_text;
    int tag_index;
    int i;

    if (readings == NULL || tags == NULL || count < 0 || count > MAX_READINGS || tag_count < 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (count == 0)
    {
        return STATUS_EMPTY;
    }

    printf("\n%-10s %-19s %12s  %s\n", "Tag ID", "Timestamp", "Value", "Alarm status");
    printf("---------------------------------------------------------\n");

    for (i = 0; i < count; i++)
    {
        status_text = "UNKNOWN TAG";

        if (findTagById(tags, tag_count, readings[i].tag_id, &tag_index) == STATUS_SUCCESS &&
            classifyReading(readings[i].value, &tags[tag_index], &alarm_status) == STATUS_SUCCESS)
        {
            status_text = alarmStatusToText(alarm_status);
        }

        printf("%-10s %-19s %12.2f  %s\n",
               readings[i].tag_id, readings[i].timestamp, readings[i].value, status_text);
    }

    printf("\n%d reading(s)\n", count);
    return STATUS_SUCCESS;
}

Status_t printTagStatistics(const Tag_t *tag, const Reading_t *readings, int count)
{
    double values[MAX_READINGS];
    int value_count = 0;
    Status_t status;

    if (tag == NULL || readings == NULL || count < 0 || count > MAX_READINGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    status = getValuesForTag(readings, count, tag->id, values, &value_count);
    if (status != STATUS_SUCCESS)
    {
        return status; // STATUS_EMPTY when the tag has no readings
    }

    printf("\nStatistics for %s (%s)\n", tag->id, tag->description);
    printf("  Readings : %d\n", value_count);
    writeStatisticsLines(stdout, tag->unit, values, value_count);

    return STATUS_SUCCESS;
}

Status_t printAlarmSummary(const Tag_t *tags, int tag_count,
                           const Reading_t *readings, int count)
{
    int i;

    if (tags == NULL || readings == NULL || tag_count < 0 || tag_count > MAX_TAGS ||
        count < 0 || count > MAX_READINGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (tag_count == 0)
    {
        return STATUS_EMPTY;
    }

    printf("\nAlarm summary\n");
    printf("-------------\n");

    for (i = 0; i < tag_count; i++)
    {
        writeTagSummary(stdout, &tags[i], readings, count, 0);
    }

    return STATUS_SUCCESS;
}

// Shift report file

Status_t saveShiftReport(const char *path, const Tag_t *tags, int tag_count,
                         const Reading_t *readings, int count)
{
    FILE *fp;
    const char *first_time = NULL;
    const char *last_time = NULL;
    int write_failed;
    int i;

    if (path == NULL || tags == NULL || readings == NULL ||
        tag_count < 0 || tag_count > MAX_TAGS || count < 0 || count > MAX_READINGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (tag_count == 0)
    {
        return STATUS_EMPTY;
    }

    fp = fopen(path, "w");
    if (fp == NULL)
    {
        return STATUS_FILE_OPEN_ERROR;
    }

    // Earliest and latest timestamp (the format sorts correctly as text).
    for (i = 0; i < count; i++)
    {
        if (first_time == NULL || strcmp(readings[i].timestamp, first_time) < 0)
        {
            first_time = readings[i].timestamp;
        }
        if (last_time == NULL || strcmp(readings[i].timestamp, last_time) > 0)
        {
            last_time = readings[i].timestamp;
        }
    }

    fprintf(fp, "SHIFT REPORT - Process Data Logger and Alarm Analyzer\n");
    fprintf(fp, "======================================================\n");
    fprintf(fp, "Tags     : %d\n", tag_count);
    fprintf(fp, "Readings : %d\n", count);
    if (first_time != NULL)
    {
        fprintf(fp, "Period   : %s to %s\n", first_time, last_time);
    }
    fprintf(fp, "\n");

    for (i = 0; i < tag_count; i++)
    {
        writeTagSummary(fp, &tags[i], readings, count, 1);
    }

    write_failed = ferror(fp);
    if (fclose(fp) != 0 || write_failed)
    {
        return STATUS_FILE_WRITE_ERROR;
    }

    return STATUS_SUCCESS;
}

// Texts

const char *statusToMessage(Status_t status)
{
    switch (status)
    {
    case STATUS_SUCCESS:
        return "Operation completed successfully.";
    case STATUS_INVALID_ARGUMENT:
        return "Internal error: invalid argument.";
    case STATUS_INVALID_INPUT:
        return "Invalid input. Please check the value and try again.";
    case STATUS_DUPLICATE:
        return "A tag with this ID already exists.";
    case STATUS_NOT_FOUND:
        return "Tag or record not found.";
    case STATUS_CAPACITY_FULL:
        return "Storage is full.";
    case STATUS_EMPTY:
        return "No data available.";
    case STATUS_FILE_OPEN_ERROR:
        return "The file could not be opened.";
    case STATUS_FILE_READ_ERROR:
        return "Error while reading the file.";
    case STATUS_FILE_WRITE_ERROR:
        return "Error while writing the file.";
    case STATUS_INVALID_FILE_DATA:
        return "The file contains no valid data.";
    default:
        return "Unknown error.";
    }
}

const char *alarmStatusToText(AlarmStatus_t status)
{
    switch (status)
    {
    case ALARM_NORMAL:
        return "NORMAL";
    case ALARM_LOW:
        return "LOW";
    case ALARM_LOW_LOW:
        return "LOW-LOW";
    case ALARM_HIGH:
        return "HIGH";
    case ALARM_HIGH_HIGH:
        return "HIGH-HIGH";
    default:
        return "UNKNOWN";
    }
}
