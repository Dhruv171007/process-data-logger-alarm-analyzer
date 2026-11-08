#include "statistics.h"

#include <math.h>
#include <stddef.h>

Status_t calculateMean(const double *values, int count, double *result)
{
    double sum = 0.0;
    int i;

    if (values == NULL || result == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (count <= 0)
    {
        return STATUS_EMPTY;
    }

    for (i = 0; i < count; i++)
    {
        sum += values[i];
    }

    *result = sum / count;
    return STATUS_SUCCESS;
}

Status_t calculateStdDev(const double *values, int count, double *result)
{
    double mean = 0.0;
    double sum_squares = 0.0;
    double difference;
    Status_t status;
    int i;

    if (result == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    status = calculateMean(values, count, &mean);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    for (i = 0; i < count; i++)
    {
        difference = values[i] - mean;
        sum_squares += difference * difference;
    }

    /* Population standard deviation: divide by n */
    *result = sqrt(sum_squares / count);
    return STATUS_SUCCESS;
}

Status_t calculateMin(const double *values, int count, double *result)
{
    double min_value;
    int i;

    if (values == NULL || result == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (count <= 0)
    {
        return STATUS_EMPTY;
    }

    min_value = values[0];
    for (i = 1; i < count; i++)
    {
        if (values[i] < min_value)
        {
            min_value = values[i];
        }
    }

    *result = min_value;
    return STATUS_SUCCESS;
}

Status_t calculateMax(const double *values, int count, double *result)
{
    double max_value;
    int i;

    if (values == NULL || result == NULL)
    {
        return STATUS_INVALID_ARGUMENT;
    }
    if (count <= 0)
    {
        return STATUS_EMPTY;
    }

    max_value = values[0];
    for (i = 1; i < count; i++)
    {
        if (values[i] > max_value)
        {
            max_value = values[i];
        }
    }

    *result = max_value;
    return STATUS_SUCCESS;
}