#include "validation.h"

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- Internal helpers (not part of the public interface) ---------- */

static int isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int daysInMonth(int year, int month)
{
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 2 && isLeapYear(year))
    {
        return 29;
    }
    return days[month - 1];
}

/* Converts 'length' digit characters starting at 'text' into an integer. */
static int readNumber(const char *text, int length)
{
    int value = 0;
    int i;

    for (i = 0; i < length; i++)
    {
        value = value * 10 + (text[i] - '0');
    }
    return value;
}

/* ---------- Tag, timestamp and limit validation ---------- */

Status_t validateTagId(const char *id)
{
    size_t length;
    size_t i;

    if (id == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    length = strlen(id);
    if (length < MIN_TAG_ID_LEN || length > MAX_TAG_ID_LEN)
    {
        return STATUS_INVALID_INPUT;
    }

    if (!isalpha((unsigned char)id[0]))
    {
        return STATUS_INVALID_INPUT;
    }

    for (i = 1; i < length; i++)
    {
        if (!isalnum((unsigned char)id[i]) && id[i] != '-')
        {
            return STATUS_INVALID_INPUT;
        }
    }

    return STATUS_SUCCESS;
}

Status_t validateTimestamp(const char *timestamp)
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int i;

    if (timestamp == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (strlen(timestamp) != TIMESTAMP_LEN)
    {
        return STATUS_INVALID_INPUT;
    }

    /* Format: YYYY-MM-DD HH:MM:SS */
    for (i = 0; i < TIMESTAMP_LEN; i++)
    {
        if (i == 4 || i == 7)
        {
            if (timestamp[i] != '-')
            {
                return STATUS_INVALID_INPUT;
            }
        }
        else if (i == 10)
        {
            if (timestamp[i] != ' ')
            {
                return STATUS_INVALID_INPUT;
            }
        }
        else if (i == 13 || i == 16)
        {
            if (timestamp[i] != ':')
            {
                return STATUS_INVALID_INPUT;
            }
        }
        else if (!isdigit((unsigned char)timestamp[i]))
        {
            return STATUS_INVALID_INPUT;
        }
    }

    year = readNumber(timestamp, 4);
    month = readNumber(timestamp + 5, 2);
    day = readNumber(timestamp + 8, 2);
    hour = readNumber(timestamp + 11, 2);
    minute = readNumber(timestamp + 14, 2);
    second = readNumber(timestamp + 17, 2);

    if (year < 2000 || year > 2100)
    {
        return STATUS_INVALID_INPUT;
    }
    if (month < 1 || month > 12)
    {
        return STATUS_INVALID_INPUT;
    }
    if (day < 1 || day > daysInMonth(year, month))
    {
        return STATUS_INVALID_INPUT;
    }
    if (hour > 23 || minute > 59 || second > 59)
    {
        return STATUS_INVALID_INPUT;
    }

    return STATUS_SUCCESS;
}

Status_t validateAlarmLimits(double low_low, double low, double high, double high_high)
{
    /* Written as "not (in range)" so that NaN is rejected as well. */
    if (!(low_low >= MIN_VALID_VALUE && low_low <= MAX_VALID_VALUE) ||
        !(low >= MIN_VALID_VALUE && low <= MAX_VALID_VALUE) ||
        !(high >= MIN_VALID_VALUE && high <= MAX_VALID_VALUE) ||
        !(high_high >= MIN_VALID_VALUE && high_high <= MAX_VALID_VALUE))
    {
        return STATUS_INVALID_INPUT;
    }

    if (!(low_low < low) || !(low < high) || !(high < high_high))
    {
        return STATUS_INVALID_INPUT;
    }

    return STATUS_SUCCESS;
}

Status_t validateDescription(const char *text)
{
    size_t length;
    size_t i;
    int has_visible_character = 0;

    if (text == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    length = strlen(text);
    if (length < 1 || length > MAX_DESC_LEN)
    {
        return STATUS_INVALID_INPUT;
    }

    for (i = 0; i < length; i++)
    {
        if (text[i] == ',' || text[i] == '\n' || text[i] == '\r')
        {
            return STATUS_INVALID_INPUT;
        }
        if (!isspace((unsigned char)text[i]))
        {
            has_visible_character = 1;
        }
    }

    return has_visible_character ? STATUS_SUCCESS : STATUS_INVALID_INPUT;
}

Status_t validateUnit(const char *text)
{
    size_t length;
    size_t i;

    if (text == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    length = strlen(text);
    if (length < 1 || length > MAX_UNIT_LEN)
    {
        return STATUS_INVALID_INPUT;
    }

    for (i = 0; i < length; i++)
    {
        if (isspace((unsigned char)text[i]) || text[i] == ',')
        {
            return STATUS_INVALID_INPUT;
        }
    }

    return STATUS_SUCCESS;
}

/* ---------- Number parsing ---------- */

Status_t parseDouble(const char *text, double *value)
{
    const char *p;
    char *end = NULL;
    double number;

    if (text == NULL || value == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    /* Only 'e' or 'E' (exponent) may be a letter; this rejects nan, inf and hex. */
    for (p = text; *p != '\0'; p++)
    {
        if (isalpha((unsigned char)*p) && *p != 'e' && *p != 'E')
        {
            return STATUS_INVALID_INPUT;
        }
    }

    errno = 0;
    number = strtod(text, &end);

    if (end == text)
    {
        return STATUS_INVALID_INPUT; /* nothing was converted (also empty text) */
    }

    while (isspace((unsigned char)*end))
    {
        end++;
    }
    if (*end != '\0')
    {
        return STATUS_INVALID_INPUT; /* extra characters after the number */
    }

    if (errno == ERANGE)
    {
        return STATUS_INVALID_INPUT;
    }
    if (!(number >= MIN_VALID_VALUE && number <= MAX_VALID_VALUE))
    {
        return STATUS_INVALID_INPUT;
    }

    *value = number;
    return STATUS_SUCCESS;
}

Status_t validateMenuChoice(const char *text, int min, int max, int *choice)
{
    int value = 0;
    int digits = 0;

    if (text == NULL || choice == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    while (isspace((unsigned char)*text))
    {
        text++;
    }

    while (isdigit((unsigned char)*text))
    {
        if (digits >= 9)
        {
            return STATUS_INVALID_INPUT; /* far too long to be a menu number */
        }
        value = value * 10 + (*text - '0');
        digits++;
        text++;
    }

    if (digits == 0)
    {
        return STATUS_INVALID_INPUT;
    }

    while (isspace((unsigned char)*text))
    {
        text++;
    }
    if (*text != '\0')
    {
        return STATUS_INVALID_INPUT;
    }

    if (value < min || value > max)
    {
        return STATUS_INVALID_INPUT;
    }

    *choice = value;
    return STATUS_SUCCESS;
}

/* ---------- Line input ---------- */

Status_t readLine(char *buffer, int size)
{
    size_t length;
    int c;
    int too_long = 0;

    if (buffer == NULL || size < 2)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    if (fgets(buffer, size, stdin) == NULL)
    {
        buffer[0] = '\0';
        return STATUS_INVALID_INPUT; /* input closed or read error */
    }

    length = strlen(buffer);

    if (length > 0 && buffer[length - 1] == '\n')
    {
        buffer[length - 1] = '\0';
        length--;
    }
    else if (length == (size_t)(size - 1))
    {
        /* Buffer is full: the line is either exactly this long or too long. */
        while ((c = getchar()) != '\n' && c != EOF)
        {
            too_long = 1;
        }
        if (too_long)
        {
            buffer[0] = '\0';
            return STATUS_INVALID_INPUT;
        }
    }

    if (length > 0 && buffer[length - 1] == '\r')
    {
        buffer[length - 1] = '\0';
    }

    return STATUS_SUCCESS;
}