# Detailed Design: Process Data Logger and Alarm Analyzer

This document extends `project_proposal.md`. Later parts (module design, function design, validation plan, file handling, error handling, application flow and unit-test plan) are added in separate commits.

---

# 1. Data Design

## 1.1 Constants

| Constant               | Value                      | Purpose                                              |
|------------------------|----------------------------|------------------------------------------------------|
| MAX_TAGS               | 50                         | Maximum number of tags stored                        |
| MAX_READINGS           | 1000                       | Maximum number of readings stored                    |
| MAX_TAG_ID_LEN         | 15                         | Maximum characters in a tag ID (buffer size is +1)   |
| MIN_TAG_ID_LEN         | 3                          | Minimum characters in a tag ID                       |
| MAX_DESC_LEN           | 47                         | Maximum characters in a tag description              |
| MAX_UNIT_LEN           | 7                          | Maximum characters in an engineering unit            |
| TIMESTAMP_LEN          | 19                         | Length of `YYYY-MM-DD HH:MM:SS`                      |
| MAX_LINE_LEN           | 128                        | Buffer size for reading one line of input or a file  |
| MIN_VALID_VALUE        | -100000.0                  | Lowest accepted reading or limit value               |
| MAX_VALID_VALUE        | 100000.0                   | Highest accepted reading or limit value              |
| SAMPLE_INTERVAL_SEC    | 60                         | Assumed time between readings (for time in alarm)    |
| TAGS_FILE              | "data/tags.txt"            | Relative path of the tag storage file                |
| READINGS_FILE          | "data/readings.txt"        | Relative path of the reading storage file            |
| REPORT_FILE            | "data/shift_report.txt"    | Relative path of the generated report                |

All paths are relative, so the project builds and runs on any machine.

## 1.2 Structure Definitions

### Structure: Tag_t

Purpose: Represents one measurement point (for example, a temperature transmitter) and its alarm limits.

```c
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
```

| Member      | C Type                       | Valid Range or Size                   | Description                    |
|-------------|------------------------------|---------------------------------------|--------------------------------|
| id          | char[MAX_TAG_ID_LEN + 1]     | 3 to 15 characters                    | Unique tag identifier          |
| description | char[MAX_DESC_LEN + 1]       | 1 to 47 characters                    | What the tag measures          |
| unit        | char[MAX_UNIT_LEN + 1]       | 1 to 7 characters                     | Engineering unit               |
| low_low     | double                       | MIN_VALID_VALUE to MAX_VALID_VALUE    | Low-Low alarm limit            |
| low         | double                       | MIN_VALID_VALUE to MAX_VALID_VALUE    | Low alarm limit                |
| high        | double                       | MIN_VALID_VALUE to MAX_VALID_VALUE    | High alarm limit               |
| high_high   | double                       | MIN_VALID_VALUE to MAX_VALID_VALUE    | High-High alarm limit          |

### Structure: Reading_t

Purpose: Represents one time-stamped measurement of a tag. The alarm status is calculated when needed and is not stored, so it can never become out of date if a tag's limits are edited.

```c
typedef struct
{
    char   tag_id[MAX_TAG_ID_LEN + 1];
    char   timestamp[TIMESTAMP_LEN + 1];
    double value;
} Reading_t;
```

| Member    | C Type                    | Valid Range or Size                | Description                       |
|-----------|---------------------------|------------------------------------|-----------------------------------|
| tag_id    | char[MAX_TAG_ID_LEN + 1]  | Must match an existing tag         | Tag this reading belongs to       |
| timestamp | char[TIMESTAMP_LEN + 1]   | `YYYY-MM-DD HH:MM:SS`              | When the reading was taken        |
| value     | double                    | MIN_VALID_VALUE to MAX_VALID_VALUE | Measured value in the tag's unit  |

### Relationship Between Records

One tag has many readings (one-to-many). `Reading_t.tag_id` refers to `Tag_t.id`. A reading cannot be added for an unknown tag, and deleting a tag also deletes its readings.

### Storage in Memory

Records are held in fixed-size arrays (`Tag_t tags[MAX_TAGS]`, `Reading_t readings[MAX_READINGS]`) with a count for each. The arrays are declared in `main()` and passed to modules by pointer, so no global variables are needed. Dynamic memory is not used because the fixed limits are enough for this project.

## 1.3 Enumeration Definitions

### Enumeration: AlarmStatus_t

Purpose: Result of comparing a reading with its tag's alarm limits.

```c
typedef enum
{
    ALARM_NORMAL = 0,
    ALARM_LOW,
    ALARM_LOW_LOW,
    ALARM_HIGH,
    ALARM_HIGH_HIGH
} AlarmStatus_t;
```

| Constant         | Meaning                                         |
|------------------|-------------------------------------------------|
| ALARM_NORMAL     | Value is between the Low and High limits        |
| ALARM_LOW        | Value is at or below Low, above Low-Low         |
| ALARM_LOW_LOW    | Value is at or below Low-Low                    |
| ALARM_HIGH       | Value is at or above High, below High-High      |
| ALARM_HIGH_HIGH  | Value is at or above High-High                  |

**Classification rule (limits are inclusive):**

1. value <= low_low gives ALARM_LOW_LOW
2. value <= low gives ALARM_LOW
3. value >= high_high gives ALARM_HIGH_HIGH
4. value >= high gives ALARM_HIGH
5. otherwise ALARM_NORMAL

**Limit ordering rule for every tag:** low_low < low < high < high_high.

### Enumeration: SortField_t and SortOrder_t

```c
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
```

| Constant           | Meaning                          |
|--------------------|----------------------------------|
| SORT_BY_VALUE      | Sort readings by measured value  |
| SORT_BY_TIMESTAMP  | Sort readings by time            |
| SORT_ASCENDING     | Smallest or earliest first       |
| SORT_DESCENDING    | Largest or latest first          |

### Enumeration: Status_t

Purpose: One consistent return type for every function that can fail.

```c
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
```

| Constant                  | Meaning                                                      |
|---------------------------|--------------------------------------------------------------|
| STATUS_SUCCESS            | Operation completed                                          |
| STATUS_INVALID_ARGUMENT   | A pointer argument was NULL or a count was invalid           |
| STATUS_INVALID_INPUT      | A value failed validation (format, range or length)          |
| STATUS_DUPLICATE          | A tag with the same ID already exists                        |
| STATUS_NOT_FOUND          | The requested tag or reading does not exist                  |
| STATUS_CAPACITY_FULL      | MAX_TAGS or MAX_READINGS has been reached                    |
| STATUS_EMPTY              | There is no data to process                                  |
| STATUS_FILE_OPEN_ERROR    | A file could not be opened                                   |
| STATUS_FILE_READ_ERROR    | Reading from a file failed                                   |
| STATUS_FILE_WRITE_ERROR   | Writing to a file failed                                     |
| STATUS_INVALID_FILE_DATA  | A file line was malformed or contained invalid values        |

## 1.4 Data Dictionary

| Data Item     | C Type    | Valid Range or Size                  | Validation Rule                                                                 | Description                       |
|---------------|-----------|--------------------------------------|---------------------------------------------------------------------------------|-----------------------------------|
| Tag ID        | char[16]  | 3 to 15 characters                   | First character is a letter; remaining are letters, digits or hyphen; unique    | Identifies a tag, for example TT-101 |
| Description   | char[48]  | 1 to 47 characters                   | Not empty; no newline characters                                                | Text description of the tag       |
| Unit          | char[8]   | 1 to 7 characters                    | Not empty; no spaces                                                            | Engineering unit, for example degC |
| Alarm limits  | double    | -100000.0 to 100000.0                | Numeric; low_low < low < high < high_high                                       | Four alarm thresholds per tag     |
| Timestamp     | char[20]  | Exactly 19 characters                | Format `YYYY-MM-DD HH:MM:SS`; year 2000 to 2100; valid month, day (with leap years), hour 0-23, minute 0-59, second 0-59 | Time of a reading |
| Reading value | double    | -100000.0 to 100000.0                | Numeric only; no text or empty input                                            | Measured process value            |
| Menu choice   | int       | Valid menu numbers only              | Whole number within the menu range                                              | User's menu selection             |
| Sort field    | SortField_t | SORT_BY_VALUE or SORT_BY_TIMESTAMP | Must be one of the enumeration values                                           | Field used for sorting            |
| Time range    | char[20] x2 | Two valid timestamps               | Both valid; start not later than end                                            | Filter boundaries                 |

---

# 2. Remaining Parts (to be added)

2. Module design (detailed)
3. Function design
4. Input-validation plan
5. File-handling plan
6. Error-handling strategy
7. Application flow
8. Unit-test plan