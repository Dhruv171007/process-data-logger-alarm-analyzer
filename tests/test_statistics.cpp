#include <gtest/gtest.h>

#include "statistics.h"

/* UT-01: calculateMean, positive case */
TEST(StatisticsTest, MeanOfOneTwoThreeIsTwo)
{
    double values[] = {1.0, 2.0, 3.0};
    double result = 0.0;

    Status_t status = calculateMean(values, 3, &result);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_NEAR(2.0, result, 1e-9);
}

/* UT-02: calculateMean, negative case (no values) */
TEST(StatisticsTest, MeanOfZeroValuesReturnsEmpty)
{
    double values[] = {1.0};
    double result = -1.0;

    Status_t status = calculateMean(values, 0, &result);

    EXPECT_EQ(STATUS_EMPTY, status);
    EXPECT_NEAR(-1.0, result, 1e-9);   /* result must stay unchanged */
}

/* UT-03: calculateStdDev, positive case */
TEST(StatisticsTest, StdDevOfKnownDataSetIsTwo)
{
    double values[] = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    double result = 0.0;

    Status_t status = calculateStdDev(values, 8, &result);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_NEAR(2.0, result, 1e-9);
}

/* UT-04: calculateStdDev, boundary case (single value) */
TEST(StatisticsTest, StdDevOfSingleValueIsZero)
{
    double values[] = {42.5};
    double result = -1.0;

    Status_t status = calculateStdDev(values, 1, &result);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_NEAR(0.0, result, 1e-9);
}