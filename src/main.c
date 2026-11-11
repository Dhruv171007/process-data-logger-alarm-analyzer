/* Process Data Logger and Alarm Analyzer
   Menu-driven application: main.c only handles the menu and the workflow.
   All data handling is done by the other modules. */

#include "common.h"
#include "alarm.h"
#include "file_handler.h"
#include "reading.h"
#include "report.h"
#include "tag.h"
#include "validation.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAIN_MENU_MAX  8
#define TAG_MENU_MAX   5

/* ---------- Input and output helpers ---------- */

static void showStatus(Status_t status)
{
    printf("%s\n", statusToMessage(status));
}

static Status_t promptLine(const char *prompt, char *line, int size)
{
    printf("%s", prompt);
    fflush(stdout);
    return readLine(line, size);
}

static Status_t promptNumber(const char *prompt, double *value)
{
    char line[MAX_LINE_LEN];
    Status_t status;

    status = promptLine(prompt, line, MAX_LINE_LEN);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    return parseDouble(line, value);
}

static Status_t promptChoice(const char *prompt, int min, int max, int *choice)
{
    char line[MAX_LINE_LEN];
    Status_t status;

    status = promptLine(prompt, line, MAX_LINE_LEN);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    return validateMenuChoice(line, min, max, choice);
}

/* Returns 1 only when the user answers y or Y. */
static int confirm(const char *prompt)
{
    char line[MAX_LINE_LEN];

    if (promptLine(prompt, line, MAX_LINE_LEN) != STATUS_SUCCESS)
    {
        return 0;
    }
    return (line[0] == 'y' || line[0] == 'Y') && line[1] == '\0';
}

/* Reads an optional timestamp. An empty answer leaves 'destination' empty. */
static Status_t promptOptionalTimestamp(const char *prompt, char *destination)
{
    char line[MAX_LINE_LEN];
    Status_t status;

    status = promptLine(prompt, line, MAX_LINE_LEN);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    if (line[0] == '\0')
    {
        destination[0] = '\0';
        return STATUS_SUCCESS;
    }

    status = validateTimestamp(line);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    strcpy(destination, line);          /* validated: exactly TIMESTAMP_LEN characters */
    return STATUS_SUCCESS;
}

/* Converts text such as "high-high" to an AlarmStatus_t. Returns 1 on success. */
static int parseAlarmStatus(const char *text, AlarmStatus_t *status)
{
    char upper[16];
    size_t length = strlen(text);
    size_t i;
    int state;

    if (length == 0 || length >= sizeof(upper))
    {
        return 0;
    }

    for (i = 0; i < length; i++)
    {
        upper[i] = (char)toupper((unsigned char)text[i]);
    }
    upper[length] = '\0';

    for (state = ALARM_NORMAL; state <= ALARM_HIGH_HIGH; state++)
    {
        if (strcmp(upper, alarmStatusToText((AlarmStatus_t)state)) == 0)
        {
            *status = (AlarmStatus_t)state;
            return 1;
        }
    }
    return 0;
}

/* ---------- Menus ---------- */

static void printMainMenu(void)
{
    printf("\n===== Process Data Logger and Alarm Analyzer =====\n");
    printf(" 1. Manage tags\n");
    printf(" 2. Add reading\n");
    printf(" 3. Import readings from CSV\n");
    printf(" 4. View, filter and sort readings\n");
    printf(" 5. Show statistics for a tag\n");
    printf(" 6. Show alarm summary\n");
    printf(" 7. Generate shift report\n");
    printf(" 8. Save data\n");
    printf(" 0. Save and exit\n");
}

static void printTagMenu(void)
{
    printf("\n--- Manage tags ---\n");
    printf(" 1. Add tag\n");
    printf(" 2. Edit tag\n");
    printf(" 3. Delete tag\n");
    printf(" 4. Search tag by ID\n");
    printf(" 5. List all tags\n");
    printf(" 0. Back\n");
}

/* ---------- Tag handlers ---------- */

/* Reads description, unit and the four limits into 'tag' (the ID is not touched).
   Text is validated before it is copied, so it always fits the tag fields. */
static Status_t promptTagDetails(Tag_t *tag)
{
    char line[MAX_LINE_LEN];
    Status_t status;

    status = promptLine("Description (max 47 characters, no commas): ", line, MAX_LINE_LEN);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    status = validateDescription(line);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    strcpy(tag->description, line);

    status = promptLine("Unit (max 7 characters, no spaces), e.g. degC: ", line, MAX_LINE_LEN);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    status = validateUnit(line);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    strcpy(tag->unit, line);

    status = promptNumber("Low-Low alarm limit : ", &tag->low_low);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    status = promptNumber("Low alarm limit     : ", &tag->low);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    status = promptNumber("High alarm limit    : ", &tag->high);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    return promptNumber("High-High alarm limit: ", &tag->high_high);
}

static void handleAddTag(Tag_t *tags, int *tag_count)
{
    char line[MAX_LINE_LEN];
    Tag_t tag;
    Status_t status;

    memset(&tag, 0, sizeof(tag));

    status = promptLine("Tag ID (3-15 characters, e.g. TT-101): ", line, MAX_LINE_LEN);
    if (status == STATUS_SUCCESS)
    {
        status = validateTagId(line);
    }
    if (status != STATUS_SUCCESS)
    {
        showStatus(status);
        return;
    }
    strcpy(tag.id, line);

    status = promptTagDetails(&tag);
    if (status == STATUS_SUCCESS)
    {
        status = addTag(tags, tag_count, &tag);
    }

    if (status == STATUS_SUCCESS)
    {
        printf("Tag %s added.\n", tag.id);
    }
    else
    {
        showStatus(status);
    }
}

static void handleEditTag(Tag_t *tags, int tag_count)
{
    char line[MAX_LINE_LEN];
    Tag_t updated;
    Status_t status;
    int index;

    status = promptLine("Tag ID to edit: ", line, MAX_LINE_LEN);
    if (status == STATUS_SUCCESS)
    {
        status = findTagById(tags, tag_count, line, &index);
    }
    if (status != STATUS_SUCCESS)
    {
        showStatus(status);
        return;
    }

    printTags(&tags[index], 1);
    printf("Enter the new details (all fields are required).\n");

    memset(&updated, 0, sizeof(updated));
    status = promptTagDetails(&updated);
    if (status == STATUS_SUCCESS)
    {
        status = editTag(tags, tag_count, tags[index].id, &updated);
    }

    if (status == STATUS_SUCCESS)
    {
        printf("Tag %s updated.\n", tags[index].id);
    }
    else
    {
        showStatus(status);
    }
}

static void handleDeleteTag(Tag_t *tags, int *tag_count, Reading_t *readings, int *reading_count)
{
    char line[MAX_LINE_LEN];
    char id[MAX_TAG_ID_LEN + 1];
    Status_t status;
    int index;
    int removed = 0;

    status = promptLine("Tag ID to delete: ", line, MAX_LINE_LEN);
    if (status == STATUS_SUCCESS)
    {
        status = findTagById(tags, *tag_count, line, &index);
    }
    if (status != STATUS_SUCCESS)
    {
        showStatus(status);
        return;
    }

    strcpy(id, tags[index].id);

    if (!confirm("This also deletes all readings of the tag. Continue? (y/n): "))
    {
        printf("Delete cancelled.\n");
        return;
    }

    status = deleteTag(tags, tag_count, id);
    if (status == STATUS_SUCCESS)
    {
        status = deleteReadingsByTag(readings, reading_count, id, &removed);
    }

    if (status == STATUS_SUCCESS)
    {
        printf("Tag %s deleted. %d reading(s) removed.\n", id, removed);
    }
    else
    {
        showStatus(status);
    }
}

static void handleSearchTag(const Tag_t *tags, int tag_count)
{
    char line[MAX_LINE_LEN];
    Status_t status;
    int index;

    status = promptLine("Tag ID to search: ", line, MAX_LINE_LEN);
    if (status == STATUS_SUCCESS)
    {
        status = findTagById(tags, tag_count, line, &index);
    }

    if (status == STATUS_SUCCESS)
    {
        printTags(&tags[index], 1);
    }
    else
    {
        showStatus(status);
    }
}

static void handleManageTags(Tag_t *tags, int *tag_count, Reading_t *readings, int *reading_count)
{
    Status_t status;
    int choice = -1;

    while (choice != 0)
    {
        printTagMenu();
        status = promptChoice("Enter your choice: ", 0, TAG_MENU_MAX, &choice);
        if (status != STATUS_SUCCESS)
        {
            if (feof(stdin))
            {
                return;
            }
            printf("Invalid choice. Enter a number from 0 to %d.\n", TAG_MENU_MAX);
            choice = -1;
            continue;
        }

        switch (choice)
        {
            case 1: handleAddTag(tags, tag_count); break;
            case 2: handleEditTag(tags, *tag_count); break;
            case 3: handleDeleteTag(tags, tag_count, readings, reading_count); break;
            case 4: handleSearchTag(tags, *tag_count); break;
            case 5:
                status = printTags(tags, *tag_count);
                if (status != STATUS_SUCCESS)
                {
                    showStatus(status);
                }
                break;
            default: break;
        }
    }
}

/* ---------- Reading handlers ---------- */

static void handleAddReading(const Tag_t *tags, int tag_count, Reading_t *readings, int *reading_count)
{
    char tag_id[MAX_LINE_LEN];
    char timestamp[MAX_LINE_LEN];
    AlarmStatus_t alarm_status;
    double value = 0.0;
    Status_t status;
    int index;

    status = promptLine("Tag ID: ", tag_id, MAX_LINE_LEN);
    if (status == STATUS_SUCCESS)
    {
        status = promptLine("Timestamp (YYYY-MM-DD HH:MM:SS): ", timestamp, MAX_LINE_LEN);
    }
    if (status == STATUS_SUCCESS)
    {
        status = promptNumber("Value: ", &value);
    }
    if (status == STATUS_SUCCESS)
    {
        status = addReading(readings, reading_count, tags, tag_count, tag_id, timestamp, value);
    }

    if (status != STATUS_SUCCESS)
    {
        showStatus(status);
        return;
    }

    printf("Reading added.");
    if (findTagById(tags, tag_count, tag_id, &index) == STATUS_SUCCESS &&
        classifyReading(value, &tags[index], &alarm_status) == STATUS_SUCCESS)
    {
        printf(" Alarm status: %s", alarmStatusToText(alarm_status));
    }
    printf("\n");
}

static void handleImport(const Tag_t *tags, int tag_count, Reading_t *readings, int *reading_count)
{
    char path[MAX_LINE_LEN];
    Status_t status;
    int imported = 0;
    int rejected = 0;

    status = promptLine("CSV file path: ", path, MAX_LINE_LEN);
    if (status == STATUS_SUCCESS && path[0] == '\0')
    {
        status = STATUS_INVALID_INPUT;
    }
    if (status == STATUS_SUCCESS)
    {
        status = importReadingsCsv(path, readings, reading_count, tags, tag_count,
                                   &imported, &rejected);
    }

    if (status == STATUS_SUCCESS || status == STATUS_CAPACITY_FULL ||
        status == STATUS_INVALID_FILE_DATA)
    {
        printf("Imported %d reading(s), rejected %d line(s).\n", imported, rejected);
    }
    if (status != STATUS_SUCCESS)
    {
        showStatus(status);
    }
}

static void handleViewReadings(const Tag_t *tags, int tag_count, const Reading_t *readings,
                               int reading_count, Reading_t *view_buffer)
{
    char line[MAX_LINE_LEN];
    ReadingFilter_t filter;
    Status_t status;
    int out_count = 0;
    int sort_field = 0;
    int sort_order = 1;

    memset(&filter, 0, sizeof(filter));

    status = promptLine("Tag ID (empty = all tags): ", line, MAX_LINE_LEN);
    if (status == STATUS_SUCCESS && line[0] != '\0')
    {
        status = validateTagId(line);
        if (status == STATUS_SUCCESS)
        {
            strcpy(filter.tag_id, line);
        }
    }
    if (status == STATUS_SUCCESS)
    {
        status = promptLine("Alarm status (NORMAL, LOW, LOW-LOW, HIGH, HIGH-HIGH, empty = all): ",
                            line, MAX_LINE_LEN);
    }
    if (status == STATUS_SUCCESS && line[0] != '\0')
    {
        if (parseAlarmStatus(line, &filter.status))
        {
            filter.filter_by_status = 1;
        }
        else
        {
            status = STATUS_INVALID_INPUT;
        }
    }
    if (status == STATUS_SUCCESS)
    {
        status = promptOptionalTimestamp("Start time (YYYY-MM-DD HH:MM:SS, empty = none): ",
                                         filter.start_time);
    }
    if (status == STATUS_SUCCESS)
    {
        status = promptOptionalTimestamp("End time (YYYY-MM-DD HH:MM:SS, empty = none): ",
                                         filter.end_time);
    }
    if (status == STATUS_SUCCESS)
    {
        status = promptChoice("Sort by (0 = no sorting, 1 = value, 2 = timestamp): ",
                              0, 2, &sort_field);
    }
    if (status == STATUS_SUCCESS && sort_field != 0)
    {
        status = promptChoice("Order (1 = ascending, 2 = descending): ", 1, 2, &sort_order);
    }
    if (status == STATUS_SUCCESS)
    {
        status = filterReadings(readings, reading_count, tags, tag_count, &filter,
                                view_buffer, &out_count);
    }
    if (status == STATUS_SUCCESS && sort_field != 0)
    {
        status = sortReadings(view_buffer, out_count,
                              (sort_field == 1) ? SORT_BY_VALUE : SORT_BY_TIMESTAMP,
                              (sort_order == 1) ? SORT_ASCENDING : SORT_DESCENDING);
    }
    if (status == STATUS_SUCCESS)
    {
        status = printReadings(view_buffer, out_count, tags, tag_count);
    }

    if (status != STATUS_SUCCESS)
    {
        showStatus(status);
    }
}

static void handleStatistics(const Tag_t *tags, int tag_count, const Reading_t *readings,
                             int reading_count)
{
    char line[MAX_LINE_LEN];
    Status_t status;
    int index;

    status = promptLine("Tag ID: ", line, MAX_LINE_LEN);
    if (status == STATUS_SUCCESS)
    {
        status = findTagById(tags, tag_count, line, &index);
    }
    if (status == STATUS_SUCCESS)
    {
        status = printTagStatistics(&tags[index], readings, reading_count);
    }

    if (status == STATUS_EMPTY)
    {
        printf("This tag has no readings yet.\n");
    }
    else if (status != STATUS_SUCCESS)
    {
        showStatus(status);
    }
}

/* ---------- Save, load and report ---------- */

static Status_t saveAllData(const Tag_t *tags, int tag_count,
                            const Reading_t *readings, int reading_count)
{
    Status_t status;

    status = saveTags(TAGS_FILE, tags, tag_count);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }
    return saveReadings(READINGS_FILE, readings, reading_count);
}

static void loadStartupData(Tag_t *tags, int *tag_count, Reading_t *readings, int *reading_count)
{
    Status_t status;
    int rejected = 0;
    int imported = 0;

    status = loadTags(TAGS_FILE, tags, tag_count, &rejected);
    if (status == STATUS_FILE_OPEN_ERROR)
    {
        printf("No saved tags found. Starting with empty data.\n");
        *tag_count = 0;
    }
    else if (status != STATUS_SUCCESS)
    {
        printf("Tags file: %s\n", statusToMessage(status));
        *tag_count = 0;
    }
    else if (rejected > 0)
    {
        printf("Tags file: %d invalid line(s) were skipped.\n", rejected);
    }

    rejected = 0;
    status = importReadingsCsv(READINGS_FILE, readings, reading_count, tags, *tag_count,
                               &imported, &rejected);
    if (status == STATUS_FILE_OPEN_ERROR)
    {
        printf("No saved readings found.\n");
    }
    else if (status != STATUS_SUCCESS)
    {
        printf("Readings file: %s\n", statusToMessage(status));
    }
    if (rejected > 0)
    {
        printf("Readings file: %d invalid line(s) were skipped.\n", rejected);
    }

    printf("Loaded %d tag(s) and %d reading(s).\n", *tag_count, *reading_count);
}

/* Returns 1 when the program should exit. */
static int handleSaveAndExit(const Tag_t *tags, int tag_count,
                             const Reading_t *readings, int reading_count)
{
    Status_t status = saveAllData(tags, tag_count, readings, reading_count);

    if (status == STATUS_SUCCESS)
    {
        printf("Data saved. Goodbye.\n");
        return 1;
    }

    showStatus(status);
    if (feof(stdin))
    {
        return 1;                       /* nobody can answer any more questions */
    }
    return confirm("Saving failed. Exit without saving? (y/n): ");
}

/* ---------- Main program ---------- */

int main(void)
{
    Tag_t tags[MAX_TAGS];
    Reading_t readings[MAX_READINGS];
    Reading_t view_buffer[MAX_READINGS];
    int tag_count = 0;
    int reading_count = 0;
    int choice = 0;
    int running = 1;
    Status_t status;

    memset(tags, 0, sizeof(tags));
    memset(readings, 0, sizeof(readings));
    memset(view_buffer, 0, sizeof(view_buffer));

    loadStartupData(tags, &tag_count, readings, &reading_count);

    while (running)
    {
        printMainMenu();
        status = promptChoice("Enter your choice: ", 0, MAIN_MENU_MAX, &choice);

        if (status != STATUS_SUCCESS)
        {
            if (feof(stdin))
            {
                printf("\nInput closed. Saving and exiting.\n");
                handleSaveAndExit(tags, tag_count, readings, reading_count);
                break;
            }
            printf("Invalid choice. Enter a number from 0 to %d.\n", MAIN_MENU_MAX);
            continue;
        }

        switch (choice)
        {
            case 1:
                handleManageTags(tags, &tag_count, readings, &reading_count);
                break;
            case 2:
                handleAddReading(tags, tag_count, readings, &reading_count);
                break;
            case 3:
                handleImport(tags, tag_count, readings, &reading_count);
                break;
            case 4:
                handleViewReadings(tags, tag_count, readings, reading_count, view_buffer);
                break;
            case 5:
                handleStatistics(tags, tag_count, readings, reading_count);
                break;
            case 6:
                status = printAlarmSummary(tags, tag_count, readings, reading_count);
                if (status != STATUS_SUCCESS)
                {
                    showStatus(status);
                }
                break;
            case 7:
                status = saveShiftReport(REPORT_FILE, tags, tag_count, readings, reading_count);
                if (status == STATUS_SUCCESS)
                {
                    printf("Shift report saved to %s\n", REPORT_FILE);
                }
                else
                {
                    showStatus(status);
                }
                break;
            case 8:
                status = saveAllData(tags, tag_count, readings, reading_count);
                if (status == STATUS_SUCCESS)
                {
                    printf("Data saved.\n");
                }
                else
                {
                    showStatus(status);
                }
                break;
            case 0:
                if (handleSaveAndExit(tags, tag_count, readings, reading_count))
                {
                    running = 0;
                }
                break;
            default:
                break;
        }
    }

    return 0;
}
