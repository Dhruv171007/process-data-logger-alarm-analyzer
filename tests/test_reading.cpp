#include <gtest/gtest.h>

#include <cstring>

#include "reading.h"

/* Every test starts with two tags (TT-101, TT-102, limits 10 / 20 / 80 / 90)
   and an empty reading array. */
class ReadingTest : public ::testing::Test
{
protected:
    Tag_t     tags_[2];
    Reading_t readings_[MAX_READINGS];
    Reading_t out_[MAX_READINGS];
    int       count_;

    void SetUp() override
    {
        std::memset(tags_, 0, sizeof(tags_));
        std::memset(readings_, 0, sizeof(readings_));
        std::memset(out_, 0, sizeof(out_));
        count_ = 0;

        setupTag(&tags_[0], "TT-101");
        setupTag(&tags_[1], "TT-102");
    }

    void setupTag(Tag_t *tag, const char *id)
    {
        std::strcpy(tag->id, id);
        std::strcpy(tag->description, "Test tag");
        std::strcpy(tag->unit, "degC");
        tag->low_low   = 10.0;
        tag->low       = 20.0;
        tag->high      = 80.0;
        tag->high_high = 90.0;
    }

    /* Adds a reading and stops the test if the add fails. */
    void add(const char *tag_id, const char *timestamp, double value)
    {
        ASSERT_EQ(STATUS_SUCCESS,
                  addReading(readings_, &count_, tags_, 2, tag_id, timestamp, value));
    }

    ReadingFilter_t emptyFilter()
    {
        ReadingFilter_t filter;
        std::memset(&filter, 0, sizeof(filter));
        return filter;
    }
};

/* UT-28: addReading, positive case */
TEST_F(ReadingTest, AddValidReadingIncreasesCount)
{
    Status_t status = addReading(readings_, &count_, tags_, 2,
                                 "TT-101", "2026-11-07 08:00:00", 72.5);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_EQ(1, count_);
    EXPECT_STREQ("TT-101", readings_[0].tag_id);
    EXPECT_NEAR(72.5, readings_[0].value, 1e-9);
}

/* UT-29: addReading, negative case (unknown tag) */
TEST_F(ReadingTest, AddReadingForUnknownTagReturnsNotFound)
{
    Status_t status = addReading(readings_, &count_, tags_, 2,
                                 "XX-999", "2026-11-07 08:00:00", 50.0);

    EXPECT_EQ(STATUS_NOT_FOUND, status);
    EXPECT_EQ(0, count_);
}

/* UT-30: addReading, negative case (invalid timestamp) */
TEST_F(ReadingTest, AddReadingWithInvalidTimestampIsRejected)
{
    Status_t status = addReading(readings_, &count_, tags_, 2,
                                 "TT-101", "2026-13-07 08:00:00", 50.0);

    EXPECT_EQ(STATUS_INVALID_INPUT, status);
    EXPECT_EQ(0, count_);
}

/* UT-31: addReading, boundary case (array already holds MAX_READINGS) */
TEST_F(ReadingTest, AddReadingToFullArrayReturnsCapacityFull)
{
    for (int i = 0; i < MAX_READINGS; i++)
    {
        add("TT-101", "2026-11-07 08:00:00", 50.0);
    }

    Status_t status = addReading(readings_, &count_, tags_, 2,
                                 "TT-101", "2026-11-07 09:00:00", 50.0);

    EXPECT_EQ(STATUS_CAPACITY_FULL, status);
    EXPECT_EQ(MAX_READINGS, count_);
}

/* UT-32: deleteReadingsByTag, positive case (other tags stay) */
TEST_F(ReadingTest, DeleteReadingsRemovesOnlyThatTag)
{
    int removed = -1;

    add("TT-101", "2026-11-07 08:00:00", 50.0);
    add("TT-102", "2026-11-07 08:00:00", 60.0);
    add("TT-101", "2026-11-07 09:00:00", 55.0);

    Status_t status = deleteReadingsByTag(readings_, &count_, "TT-101", &removed);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_EQ(2, removed);
    EXPECT_EQ(1, count_);
    EXPECT_STREQ("TT-102", readings_[0].tag_id);
}

/* UT-33: filterReadings, positive case (tag and alarm status) */
TEST_F(ReadingTest, FilterByTagAndStatusReturnsMatchingReadings)
{
    int out_count = -1;

    add("TT-101", "2026-11-07 08:00:00", 50.0);   /* normal    */
    add("TT-101", "2026-11-07 09:00:00", 85.0);   /* high      */
    add("TT-101", "2026-11-07 10:00:00", 95.0);   /* high-high */
    add("TT-102", "2026-11-07 09:00:00", 85.0);   /* high, other tag */

    ReadingFilter_t filter = emptyFilter();
    std::strcpy(filter.tag_id, "TT-101");
    filter.filter_by_status = 1;
    filter.status = ALARM_HIGH;

    Status_t status = filterReadings(readings_, count_, tags_, 2, &filter, out_, &out_count);

    EXPECT_EQ(STATUS_SUCCESS, status);
    ASSERT_EQ(1, out_count);
    EXPECT_NEAR(85.0, out_[0].value, 1e-9);
}

/* UT-34: filterReadings, boundary case (time range includes both end times) */
TEST_F(ReadingTest, FilterByTimeRangeIncludesBothBoundaries)
{
    int out_count = -1;

    add("TT-101", "2026-11-07 08:00:00", 50.0);
    add("TT-101", "2026-11-07 09:00:00", 51.0);
    add("TT-101", "2026-11-07 10:00:00", 52.0);
    add("TT-101", "2026-11-07 11:00:00", 53.0);

    ReadingFilter_t filter = emptyFilter();
    std::strcpy(filter.start_time, "2026-11-07 09:00:00");
    std::strcpy(filter.end_time, "2026-11-07 10:00:00");

    Status_t status = filterReadings(readings_, count_, tags_, 2, &filter, out_, &out_count);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_EQ(2, out_count);
}

/* UT-35: filterReadings, negative case (start time after end time) */
TEST_F(ReadingTest, FilterWithStartAfterEndIsRejected)
{
    int out_count = -1;

    add("TT-101", "2026-11-07 08:00:00", 50.0);

    ReadingFilter_t filter = emptyFilter();
    std::strcpy(filter.start_time, "2026-11-07 10:00:00");
    std::strcpy(filter.end_time, "2026-11-07 09:00:00");

    Status_t status = filterReadings(readings_, count_, tags_, 2, &filter, out_, &out_count);

    EXPECT_EQ(STATUS_INVALID_INPUT, status);
    EXPECT_EQ(-1, out_count);   /* must stay unchanged */
}

/* UT-36: sortReadings, positive case (by value, ascending) */
TEST_F(ReadingTest, SortByValueAscendingPutsSmallestFirst)
{
    add("TT-101", "2026-11-07 08:00:00", 70.0);
    add("TT-101", "2026-11-07 09:00:00", 30.0);
    add("TT-101", "2026-11-07 10:00:00", 50.0);

    Status_t status = sortReadings(readings_, count_, SORT_BY_VALUE, SORT_ASCENDING);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_NEAR(30.0, readings_[0].value, 1e-9);
    EXPECT_NEAR(50.0, readings_[1].value, 1e-9);
    EXPECT_NEAR(70.0, readings_[2].value, 1e-9);
}

/* UT-37: sortReadings, positive case (by timestamp, descending) */
TEST_F(ReadingTest, SortByTimestampDescendingPutsLatestFirst)
{
    add("TT-101", "2026-11-07 08:00:00", 70.0);
    add("TT-101", "2026-11-07 10:00:00", 30.0);
    add("TT-101", "2026-11-07 09:00:00", 50.0);

    Status_t status = sortReadings(readings_, count_, SORT_BY_TIMESTAMP, SORT_DESCENDING);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_STREQ("2026-11-07 10:00:00", readings_[0].timestamp);
    EXPECT_STREQ("2026-11-07 09:00:00", readings_[1].timestamp);
    EXPECT_STREQ("2026-11-07 08:00:00", readings_[2].timestamp);
}

/* UT-38: sortReadings, negative case (invalid sort field) */
TEST_F(ReadingTest, SortWithInvalidFieldReturnsInvalidArgument)
{
    add("TT-101", "2026-11-07 08:00:00", 70.0);

    Status_t status = sortReadings(readings_, count_,
                                   static_cast<SortField_t>(99), SORT_ASCENDING);

    EXPECT_EQ(STATUS_INVALID_ARGUMENT, status);
}

/* UT-39: getValuesForTag, positive case (only the requested tag, in order) */
TEST_F(ReadingTest, GetValuesReturnsOnlyValuesOfThatTag)
{
    double values[MAX_READINGS];
    int value_count = -1;

    add("TT-101", "2026-11-07 08:00:00", 11.0);
    add("TT-102", "2026-11-07 08:00:00", 99.0);
    add("TT-101", "2026-11-07 09:00:00", 22.0);

    Status_t status = getValuesForTag(readings_, count_, "TT-101", values, &value_count);

    EXPECT_EQ(STATUS_SUCCESS, status);
    ASSERT_EQ(2, value_count);
    EXPECT_NEAR(11.0, values[0], 1e-9);
    EXPECT_NEAR(22.0, values[1], 1e-9);
}

/* UT-40: getValuesForTag, negative case (tag has no readings) */
TEST_F(ReadingTest, GetValuesForTagWithoutReadingsReturnsEmpty)
{
    double values[MAX_READINGS];
    int value_count = -1;

    add("TT-101", "2026-11-07 08:00:00", 11.0);

    Status_t status = getValuesForTag(readings_, count_, "TT-102", values, &value_count);

    EXPECT_EQ(STATUS_EMPTY, status);
    EXPECT_EQ(-1, value_count);   /* must stay unchanged */
}