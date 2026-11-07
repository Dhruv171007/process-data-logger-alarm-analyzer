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

# 2. Function Design (Tested Modules)

This part specifies the functions in the `statistics`, `alarm` and `validation` modules, which will be unit tested. Functions in the `tag`, `reading`, `file_handler` and `report` modules are specified in a later part.

All functions return `Status_t` (or write their result through an output parameter) so that no return value is ignored. Pointer arguments are checked for NULL.

## 2.1 Module: statistics (`statistics.c`, `statistics.h`)

Standard deviation is the **population** standard deviation (divide by n).

### Function: calculateMean

| Item | Specification |
|------|---------------|
| Prototype | `Status_t calculateMean(const double *values, int count, double *result);` |
| Purpose | Calculate the arithmetic mean of an array of values |
| Parameters | `values` (input): array of readings. `count` (input): number of values. `result` (output): the mean |
| Return values | `STATUS_SUCCESS`, `STATUS_INVALID_ARGUMENT` (NULL pointer), `STATUS_EMPTY` (count is 0 or negative) |
| Preconditions | `values` holds at least `count` valid numbers |
| Postconditions | `*result` holds the mean on success; unchanged on error |
| Related requirement | FR-12 |
| Planned tests | UT-01, UT-02 |

### Function: calculateStdDev

| Item | Specification |
|------|---------------|
| Prototype | `Status_t calculateStdDev(const double *values, int count, double *result);` |
| Purpose | Calculate the population standard deviation of an array of values |
| Parameters | `values` (input), `count` (input), `result` (output): the standard deviation |
| Return values | `STATUS_SUCCESS`, `STATUS_INVALID_ARGUMENT` (NULL pointer), `STATUS_EMPTY` (count is 0 or negative) |
| Preconditions | `values` holds at least `count` valid numbers |
| Postconditions | `*result` is 0.0 when `count` is 1; unchanged on error |
| Related requirement | FR-12 |
| Planned tests | UT-03, UT-04 |

### Functions: calculateMin and calculateMax

| Item | Specification |
|------|---------------|
| Prototypes | `Status_t calculateMin(const double *values, int count, double *result);` and `Status_t calculateMax(const double *values, int count, double *result);` |
| Purpose | Find the smallest and largest value |
| Return values | Same as `calculateMean` |
| Related requirement | FR-12 |

## 2.2 Module: alarm (`alarm.c`, `alarm.h`)

### Function: classifyReading

| Item | Specification |
|------|---------------|
| Prototype | `Status_t classifyReading(double value, const Tag_t *tag, AlarmStatus_t *status);` |
| Purpose | Decide the alarm state of a value using the tag's limits |
| Parameters | `value` (input): measured value. `tag` (input): tag with the alarm limits. `status` (output): the alarm state |
| Return values | `STATUS_SUCCESS`, `STATUS_INVALID_ARGUMENT` (NULL `tag` or `status`) |
| Preconditions | The tag's limits satisfy low_low < low < high < high_high |
| Postconditions | `*status` is set using the inclusive rule in section 1.3; unchanged on error |
| Related requirement | FR-09 |
| Planned tests | UT-05, UT-06, UT-07, UT-08 |

### Function: calculateTimeInAlarm

| Item | Specification |
|------|---------------|
| Prototype | `Status_t calculateTimeInAlarm(const Reading_t *readings, int count, const Tag_t *tag, int *seconds);` |
| Purpose | Calculate the total time a tag spent in any alarm state |
| Parameters | `readings` (input): reading array. `count` (input). `tag` (input). `seconds` (output): alarm duration |
| Return values | `STATUS_SUCCESS`, `STATUS_INVALID_ARGUMENT` (NULL pointer or negative count) |
| Preconditions | Readings are logged at the fixed interval `SAMPLE_INTERVAL_SEC` |
| Postconditions | `*seconds` = (number of readings of this tag that are not ALARM_NORMAL) x `SAMPLE_INTERVAL_SEC`; readings of other tags are ignored; 0 when there are none |
| Related requirement | FR-13 |
| Planned tests | UT-18 |

## 2.3 Module: validation (`validation.c`, `validation.h`)

### Function: validateTagId

| Item | Specification |
|------|---------------|
| Prototype | `Status_t validateTagId(const char *id);` |
| Purpose | Check that a tag ID has a legal format (uniqueness is checked by the tag module) |
| Parameters | `id` (input): the text to check |
| Return values | `STATUS_SUCCESS`, `STATUS_INVALID_ARGUMENT` (NULL), `STATUS_INVALID_INPUT` (bad length or characters) |
| Preconditions | `id` is a NUL-terminated string |
| Postconditions | No data is modified |
| Rule | 3 to 15 characters; first character is a letter; the rest are letters, digits or hyphen |
| Related requirement | FR-02 |
| Planned tests | UT-09, UT-10, UT-11, UT-12 |

### Function: validateTimestamp

| Item | Specification |
|------|---------------|
| Prototype | `Status_t validateTimestamp(const char *timestamp);` |
| Purpose | Check format and calendar validity of `YYYY-MM-DD HH:MM:SS` |
| Parameters | `timestamp` (input): the text to check |
| Return values | `STATUS_SUCCESS`, `STATUS_INVALID_ARGUMENT` (NULL), `STATUS_INVALID_INPUT` |
| Preconditions | `timestamp` is a NUL-terminated string |
| Postconditions | No data is modified |
| Rule | Exactly 19 characters; separators at the fixed positions; year 2000 to 2100; month 1 to 12; day valid for the month including leap years; hour 0 to 23; minute and second 0 to 59 |
| Related requirements | FR-06, FR-08 |
| Planned tests | UT-13, UT-14, UT-15, UT-16 |

### Function: validateAlarmLimits

| Item | Specification |
|------|---------------|
| Prototype | `Status_t validateAlarmLimits(double low_low, double low, double high, double high_high);` |
| Purpose | Check that the four limits are in range and strictly increasing |
| Return values | `STATUS_SUCCESS`, `STATUS_INVALID_INPUT` |
| Rule | Each limit between `MIN_VALID_VALUE` and `MAX_VALID_VALUE`; low_low < low < high < high_high |
| Related requirements | FR-01, FR-03 |
| Planned tests | UT-17 |

### Function: parseDouble

| Item | Specification |
|------|---------------|
| Prototype | `Status_t parseDouble(const char *text, double *value);` |
| Purpose | Convert text to a number and reject anything that is not a clean number in range |
| Parameters | `text` (input): text typed by the user or read from a file. `value` (output): the number |
| Return values | `STATUS_SUCCESS`, `STATUS_INVALID_ARGUMENT` (NULL), `STATUS_INVALID_INPUT` (empty, extra characters or out of range) |
| Postconditions | `*value` is set only on success |
| Rule | Whole text is one number; between `MIN_VALID_VALUE` and `MAX_VALID_VALUE` |
| Related requirements | FR-06, FR-08 |
| Planned tests | UT-19, UT-20 |

---

# 3. Planned Unit Tests (summary)

| Test ID | Function | Category | Input or Condition | Expected Result |
|---------|----------|----------|--------------------|-----------------|
| UT-01 | calculateMean | Positive | values 1, 2, 3 | 2.0, STATUS_SUCCESS |
| UT-02 | calculateMean | Negative | count 0 | STATUS_EMPTY |
| UT-03 | calculateStdDev | Positive | values 2, 4, 4, 4, 5, 5, 7, 9 | 2.0, STATUS_SUCCESS |
| UT-04 | calculateStdDev | Boundary | one value | 0.0, STATUS_SUCCESS |
| UT-05 | classifyReading | Positive | value between low and high | ALARM_NORMAL |
| UT-06 | classifyReading | Boundary | value equal to high | ALARM_HIGH |
| UT-07 | classifyReading | Boundary | value equal to low_low | ALARM_LOW_LOW |
| UT-08 | classifyReading | Negative | NULL tag | STATUS_INVALID_ARGUMENT |
| UT-09 | validateTagId | Positive | "TT-101" | STATUS_SUCCESS |
| UT-10 | validateTagId | Negative | "1TT" (starts with digit) | STATUS_INVALID_INPUT |
| UT-11 | validateTagId | Boundary | 3-character ID "TT1" | STATUS_SUCCESS |
| UT-12 | validateTagId | Boundary | 16-character ID | STATUS_INVALID_INPUT |
| UT-13 | validateTimestamp | Positive | "2026-11-07 08:30:00" | STATUS_SUCCESS |
| UT-14 | validateTimestamp | Negative | month 13 | STATUS_INVALID_INPUT |
| UT-15 | validateTimestamp | Boundary | "2024-02-29 00:00:00" (leap year) | STATUS_SUCCESS |
| UT-16 | validateTimestamp | Boundary | "2025-02-29 00:00:00" | STATUS_INVALID_INPUT |
| UT-17 | validateAlarmLimits | Negative | high not above low | STATUS_INVALID_INPUT |
| UT-18 | calculateTimeInAlarm | Positive | 5 readings, 2 in alarm | 120 seconds |
| UT-19 | parseDouble | Negative | "abc" | STATUS_INVALID_INPUT |
| UT-20 | parseDouble | Boundary | "100000.1" | STATUS_INVALID_INPUT |

These cover 5 functions across 3 test files (`test_statistics.cpp`, `test_alarm.cpp`, `test_validation.cpp`), with 20 tests including positive, negative and boundary cases. The full test plan (section 14 of the template) will be completed later.

---

# 4. Remaining Parts (to be added)

- Detailed module descriptions
- Function design for `tag`, `reading`, `file_handler` and `report`
- Input-validation plan
- File-handling plan
- Error-handling strategy
- Application flow
- Complete unit-test plan (setup and cleanup)