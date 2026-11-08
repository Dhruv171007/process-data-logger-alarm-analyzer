#include <gtest/gtest.h>

#include <cstring>

#include "alarm.h"

/* Every test starts with the same tag: limits 10 / 20 / 80 / 90. */
class AlarmTest : public ::testing::Test
{
protected:
    Tag_t tag_;

    void SetUp() override
    {
        std::memset(&tag_, 0, sizeof(tag_));
        std::strcpy(tag_.id, "TT-101");
        std::strcpy(tag_.description, "Boiler outlet temperature");
        std::strcpy(tag_.unit, "degC");
        tag_.low_low = 10.0;
        tag_.low = 20.0;
        tag_.high = 80.0;
        tag_.high_high = 90.0;
    }
};

/* UT-05: classifyReading, positive case (value between low and high) */
TEST_F(AlarmTest, ValueBetweenLimitsIsNormal)
{
    AlarmStatus_t status = ALARM_HIGH;

    Status_t result = classifyReading(50.0, &tag_, &status);

    EXPECT_EQ(STATUS_SUCCESS, result);
    EXPECT_EQ(ALARM_NORMAL, status);
}

/* UT-06: classifyReading, boundary case (value equal to high limit) */
TEST_F(AlarmTest, ValueEqualToHighLimitIsHigh)
{
    AlarmStatus_t status = ALARM_NORMAL;

    Status_t result = classifyReading(80.0, &tag_, &status);

    EXPECT_EQ(STATUS_SUCCESS, result);
    EXPECT_EQ(ALARM_HIGH, status);
}

/* UT-07: classifyReading, boundary case (value equal to low-low limit) */
TEST_F(AlarmTest, ValueEqualToLowLowLimitIsLowLow)
{
    AlarmStatus_t status = ALARM_NORMAL;

    Status_t result = classifyReading(10.0, &tag_, &status);

    EXPECT_EQ(STATUS_SUCCESS, result);
    EXPECT_EQ(ALARM_LOW_LOW, status);
}

/* UT-08: classifyReading, negative case (NULL tag) */
TEST_F(AlarmTest, NullTagReturnsInvalidArgument)
{
    AlarmStatus_t status = ALARM_HIGH;

    Status_t result = classifyReading(50.0, NULL, &status);

    EXPECT_EQ(STATUS_INVALID_ARGUMENT, result);
    EXPECT_EQ(ALARM_HIGH, status); /* status must stay unchanged */
}

/* UT-18: calculateTimeInAlarm, positive case (5 readings, 2 in alarm) */
TEST_F(AlarmTest, TwoAlarmReadingsGiveOneHundredTwentySeconds)
{
    const double values[5] = {50.0, 85.0, 50.0, 95.0, 50.0};
    Reading_t readings[5];
    int seconds = -1;

    for (int i = 0; i < 5; i++)
    {
        std::memset(&readings[i], 0, sizeof(readings[i]));
        std::strcpy(readings[i].tag_id, "TT-101");
        std::strcpy(readings[i].timestamp, "2026-11-07 08:00:00");
        readings[i].value = values[i];
    }

    Status_t result = calculateTimeInAlarm(readings, 5, &tag_, &seconds);

    EXPECT_EQ(STATUS_SUCCESS, result);
    EXPECT_EQ(120, seconds);
}