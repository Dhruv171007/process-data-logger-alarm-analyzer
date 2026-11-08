#ifndef COMMON_H
#define COMMON_H

//Capacity and size constants
#define MAX_TAGS             50
#define MAX_READINGS         1000
#define MAX_TAG_ID_LEN       15
#define MIN_TAG_ID_LEN       3
#define MAX_DESC_LEN         47
#define MAX_UNIT_LEN         7
#define TIMESTAMP_LEN        19
#define MAX_LINE_LEN         128

//Valid value range and sampling interval
#define MIN_VALID_VALUE      (-100000.0)
#define MAX_VALID_VALUE      100000.0
#define SAMPLE_INTERVAL_SEC  60

//File paths (relative)
#define TAGS_FILE            "data/tags.txt"
#define READINGS_FILE        "data/readings.txt"
#define REPORT_FILE          "data/shift_report.txt"

//Enumerations
typedef enum
{
    STATUS_SUCCESS = 0,
    STATUS_INVALID_ARGUMENT,
    STATUS_INVALID_INPUT,
    STATUS_DUPLICATE,
    STATUS_NOT_FOUND,
    STATUS_CAPACITY_FULL,
    STATUS_EMPTY,
    STATUS_FILE_OPEN_ERROR,
    STATUS_FILE_READ_ERROR,
    STATUS_FILE_WRITE_ERROR,
    STATUS_INVALID_FILE_DATA
} Status_t;

typedef enum
{
    ALARM_NORMAL = 0,
    ALARM_LOW,
    ALARM_LOW_LOW,
    ALARM_HIGH,
    ALARM_HIGH_HIGH
} AlarmStatus_t;

typedef enum
{
    SORT_BY_VALUE = 0,
    SORT_BY_TIMESTAMP
} SortField_t;

typedef enum
{
    SORT_ASCENDING = 0,
    SORT_DESCENDING
} SortOrder_t;

//Data structures
typedef struct
{
    char   id[MAX_TAG_ID_LEN + 1];
    char   description[MAX_DESC_LEN + 1];
    char   unit[MAX_UNIT_LEN + 1];
    double low_low;
    double low;
    double high;
    double high_high;
} Tag_t;

typedef struct
{
    char   tag_id[MAX_TAG_ID_LEN + 1];
    char   timestamp[TIMESTAMP_LEN + 1];
    double value;
} Reading_t;

typedef struct
{
    char          tag_id[MAX_TAG_ID_LEN + 1];
    int           filter_by_status;
    AlarmStatus_t status;
    char          start_time[TIMESTAMP_LEN + 1];
    char          end_time[TIMESTAMP_LEN + 1];
} ReadingFilter_t;

#endif //COMMOM_H