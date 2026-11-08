#include <gtest/gtest.h>

#include "validation.h"

/* ---------- validateTagId ---------- */

/* UT-09: positive case */
TEST(ValidationTest, TagIdTT101IsValid)
{
    EXPECT_EQ(STATUS_SUCCESS, validateTagId("TT-101"));
}

/* UT-10: negative case (starts with a digit) */
TEST(ValidationTest, TagIdStartingWithDigitIsRejected)
{
    EXPECT_EQ(STATUS_INVALID_INPUT, validateTagId("1TT"));
}

/* UT-11: boundary case (shortest valid length, 3 characters) */
TEST(ValidationTest, TagIdWithThreeCharactersIsValid)
{
    EXPECT_EQ(STATUS_SUCCESS, validateTagId("TT1"));
}

/* UT-12: boundary case (16 characters, one more than allowed) */
TEST(ValidationTest, TagIdWithSixteenCharactersIsRejected)
{
    EXPECT_EQ(STATUS_INVALID_INPUT, validateTagId("ABCDEFGHIJKLMNOP"));
}

/* ---------- validateTimestamp ---------- */

/* UT-13: positive case */
TEST(ValidationTest, NormalTimestampIsValid)
{
    EXPECT_EQ(STATUS_SUCCESS, validateTimestamp("2026-11-07 08:30:00"));
}

/* UT-14: negative case (month 13) */
TEST(ValidationTest, TimestampWithMonthThirteenIsRejected)
{
    EXPECT_EQ(STATUS_INVALID_INPUT, validateTimestamp("2026-13-07 08:30:00"));
}

/* UT-15: boundary case (29 February in a leap year) */
TEST(ValidationTest, LeapDayInLeapYearIsValid)
{
    EXPECT_EQ(STATUS_SUCCESS, validateTimestamp("2024-02-29 00:00:00"));
}

/* UT-16: boundary case (29 February in a non-leap year) */
TEST(ValidationTest, LeapDayInNonLeapYearIsRejected)
{
    EXPECT_EQ(STATUS_INVALID_INPUT, validateTimestamp("2025-02-29 00:00:00"));
}

/* ---------- validateAlarmLimits ---------- */

/* UT-17: negative case (high limit below the low limit) */
TEST(ValidationTest, HighLimitBelowLowLimitIsRejected)
{
    EXPECT_EQ(STATUS_INVALID_INPUT, validateAlarmLimits(10.0, 50.0, 40.0, 90.0));
}

/* ---------- parseDouble ---------- */

/* UT-19: negative case (text instead of a number) */
TEST(ValidationTest, ParseDoubleRejectsText)
{
    double value = -1.0;

    EXPECT_EQ(STATUS_INVALID_INPUT, parseDouble("abc", &value));
    EXPECT_NEAR(-1.0, value, 1e-9);   /* value must stay unchanged */
}

/* UT-20: boundary case (just above the maximum valid value) */
TEST(ValidationTest, ParseDoubleRejectsValueAboveMaximum)
{
    double value = -1.0;

    EXPECT_EQ(STATUS_INVALID_INPUT, parseDouble("100000.1", &value));
}