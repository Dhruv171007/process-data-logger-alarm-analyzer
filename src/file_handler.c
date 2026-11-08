#include "file_handler.h"

#include "reading.h"
#include "tag.h"
#include "validation.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define TAG_FIELD_COUNT 7
#define READING_FIELD_COUNT 3

// Internal helpers (not part of the public interface)

// Reads one line without its newline.
// Returns 1 for a line, 0 at end of file, -1 if the line was too long.
static int readFileLine(FILE *fp, char *line, int size)
{
    size_t length;
    int c;
    int too_long = 0;

    if (fgets(line, size, fp) == NULL)
    {
        return 0;
    }

    length = strlen(line);

    if (length > 0 && line[length - 1] == '\n')
    {
        line[length - 1] = '\0';
        length--;
    }
    else if (length == (size_t)(size - 1))
    {
        // Buffer is full: the line is either exactly this long or too long.
        while ((c = getc(fp)) != '\n' && c != EOF)
        {
            too_long = 1;
        }
        if (too_long)
        {
            return -1;
        }
    }

    if (length > 0 && line[length - 1] == '\r')
    {
        line[length - 1] = '\0';
    }

    return 1;
}

static int isBlankLine(const char *line)
{
    while (*line != '\0')
    {
        if (!isspace((unsigned char)*line))
        {
            return 0;
        }
        line++;
    }
    return 1;
}

// Replaces every comma with '\0' and stores a pointer to each field.
// Returns the number of fields, or max_fields + 1 if there are too many.
static int splitFields(char *line, char *fields[], int max_fields)
{
    int count = 1;
    char *p = line;

    fields[0] = line;

    while (*p != '\0')
    {
        if (*p == ',')
        {
            *p = '\0';
            if (count >= max_fields)
            {
                return max_fields + 1;
            }
            fields[count] = p + 1;
            count++;
        }
        p++;
    }

    return count;
}

// Copies text into a fixed-size buffer. Returns 0 if it does not fit.
static int copyField(char *destination, size_t size, const char *source)
{
    if (strlen(source) >= size)
    {
        return 0;
    }
    strcpy(destination, source);
    return 1;
}

// Splits one tags-file line into a Tag_t. Returns 1 on success, 0 otherwise.
// Full validation (format, limits, duplicates) is done later by addTag.
static int parseTagLine(char *line, Tag_t *tag)
{
    char *fields[TAG_FIELD_COUNT];

    memset(tag, 0, sizeof(*tag));

    if (splitFields(line, fields, TAG_FIELD_COUNT) != TAG_FIELD_COUNT)
    {
        return 0;
    }

    if (!copyField(tag->id, sizeof(tag->id), fields[0]) ||
        !copyField(tag->description, sizeof(tag->description), fields[1]) ||
        !copyField(tag->unit, sizeof(tag->unit), fields[2]))
    {
        return 0;
    }

    if (parseDouble(fields[3], &tag->low_low) != STATUS_SUCCESS ||
        parseDouble(fields[4], &tag->low) != STATUS_SUCCESS ||
        parseDouble(fields[5], &tag->high) != STATUS_SUCCESS ||
        parseDouble(fields[6], &tag->high_high) != STATUS_SUCCESS)
    {
        return 0;
    }

    return 1;
}

// Tags

Status_t saveTags(const char *path, const Tag_t *tags, int tag_count)
{
    FILE *fp;
    int i;
    int write_failed = 0;

    if (path == NULL || tags == NULL || tag_count < 0 || tag_count > MAX_TAGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    fp = fopen(path, "w");
    if (fp == NULL)
    {
        return STATUS_FILE_OPEN_ERROR;
    }

    for (i = 0; i < tag_count && !write_failed; i++)
    {
        if (fprintf(fp, "%s,%s,%s,%.4f,%.4f,%.4f,%.4f\n",
                    tags[i].id, tags[i].description, tags[i].unit,
                    tags[i].low_low, tags[i].low, tags[i].high, tags[i].high_high) < 0)
        {
            write_failed = 1;
        }
    }

    if (fclose(fp) != 0 || write_failed)
    {
        return STATUS_FILE_WRITE_ERROR;
    }

    return STATUS_SUCCESS;
}

Status_t loadTags(const char *path, Tag_t *tags, int *tag_count, int *rejected)
{
    FILE *fp;
    char line[MAX_LINE_LEN];
    Tag_t tag;
    int line_result;
    int first_line = 1;
    int accepted = 0;
    int rejected_lines = 0;

    if (path == NULL || tags == NULL || tag_count == NULL || rejected == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    fp = fopen(path, "r");
    if (fp == NULL)
    {
        return STATUS_FILE_OPEN_ERROR;
    }

    while ((line_result = readFileLine(fp, line, MAX_LINE_LEN)) != 0)
    {
        if (line_result < 0)
        {
            rejected_lines++; // line was too long
            first_line = 0;
            continue;
        }

        if (isBlankLine(line))
        {
            continue;
        }

        if (first_line)
        {
            first_line = 0;
            if (strncmp(line, "id,", 3) == 0)
            {
                continue; // optional header line
            }
        }

        if (!parseTagLine(line, &tag) || addTag(tags, &accepted, &tag) != STATUS_SUCCESS)
        {
            rejected_lines++;
        }
    }

    if (ferror(fp))
    {
        fclose(fp);
        return STATUS_FILE_READ_ERROR;
    }
    fclose(fp);

    *tag_count = accepted;
    *rejected = rejected_lines;

    if (accepted == 0 && rejected_lines > 0)
    {
        return STATUS_INVALID_FILE_DATA;
    }

    return STATUS_SUCCESS;
}

// Readings

Status_t saveReadings(const char *path, const Reading_t *readings, int reading_count)
{
    FILE *fp;
    int i;
    int write_failed = 0;

    if (path == NULL || readings == NULL || reading_count < 0 || reading_count > MAX_READINGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    fp = fopen(path, "w");
    if (fp == NULL)
    {
        return STATUS_FILE_OPEN_ERROR;
    }

    for (i = 0; i < reading_count && !write_failed; i++)
    {
        if (fprintf(fp, "%s,%s,%.4f\n",
                    readings[i].tag_id, readings[i].timestamp, readings[i].value) < 0)
        {
            write_failed = 1;
        }
    }

    if (fclose(fp) != 0 || write_failed)
    {
        return STATUS_FILE_WRITE_ERROR;
    }

    return STATUS_SUCCESS;
}

Status_t importReadingsCsv(const char *path, Reading_t *readings, int *reading_count,
                           const Tag_t *tags, int tag_count,
                           int *imported, int *rejected)
{
    FILE *fp;
    char line[MAX_LINE_LEN];
    char *fields[READING_FIELD_COUNT];
    double value;
    Status_t status;
    int line_result;
    int first_line = 1;
    int imported_lines = 0;
    int rejected_lines = 0;
    int array_full = 0;

    if (path == NULL || readings == NULL || reading_count == NULL || tags == NULL ||
        imported == NULL || rejected == NULL ||
        *reading_count < 0 || *reading_count > MAX_READINGS || tag_count < 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    fp = fopen(path, "r");
    if (fp == NULL)
    {
        return STATUS_FILE_OPEN_ERROR;
    }

    while (!array_full && (line_result = readFileLine(fp, line, MAX_LINE_LEN)) != 0)
    {
        if (line_result < 0)
        {
            rejected_lines++; // line was too long
            first_line = 0;
            continue;
        }

        if (isBlankLine(line))
        {
            continue;
        }

        if (first_line)
        {
            first_line = 0;
            if (strncmp(line, "tag_id", 6) == 0)
            {
                continue; // optional header line
            }
        }

        if (splitFields(line, fields, READING_FIELD_COUNT) != READING_FIELD_COUNT ||
            parseDouble(fields[2], &value) != STATUS_SUCCESS)
        {
            rejected_lines++;
            continue;
        }

        status = addReading(readings, reading_count, tags, tag_count,
                            fields[0], fields[1], value);

        if (status == STATUS_SUCCESS)
        {
            imported_lines++;
        }
        else if (status == STATUS_CAPACITY_FULL)
        {
            array_full = 1;
        }
        else
        {
            rejected_lines++;
        }
    }

    if (ferror(fp))
    {
        fclose(fp);
        return STATUS_FILE_READ_ERROR;
    }
    fclose(fp);

    *imported = imported_lines;
    *rejected = rejected_lines;

    if (array_full)
    {
        return STATUS_CAPACITY_FULL;
    }
    if (imported_lines == 0 && rejected_lines > 0)
    {
        return STATUS_INVALID_FILE_DATA;
    }

    return STATUS_SUCCESS;
}