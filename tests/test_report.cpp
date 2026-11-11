#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "report.h"

static const char *kReportPath = "build/ut_report.tmp";

/* Every test starts with two tags (TT-101, TT-102, limits 10 / 20 / 80 / 90)
   and an empty reading array. */
class ReportTest : public ::testing::Test
{
protected:
    Tag_t tags_[2];
    Reading_t readings_[MAX_READINGS];
    int count_;

    void SetUp() override
    {
        std::memset(tags_, 0, sizeof(tags_));
        std::memset(readings_, 0, sizeof(readings_));
        count_ = 0;
        std::remove(kReportPath);

        setupTag(&tags_[0], "TT-101", "Boiler outlet");
        setupTag(&tags_[1], "TT-102", "Boiler inlet");
    }

    void TearDown() override
    {
        std::remove(kReportPath);
    }

    void setupTag(Tag_t *tag, const char *id, const char *description)
    {
        std::strcpy(tag->id, id);
        std::strcpy(tag->description, description);
        std::strcpy(tag->unit, "degC");
        tag->low_low = 10.0;
        tag->low = 20.0;
        tag->high = 80.0;
        tag->high_high = 90.0;
    }

    void push(const char *tag_id, const char *timestamp, double value)
    {
        std::strcpy(readings_[count_].tag_id, tag_id);
        std::strcpy(readings_[count_].timestamp, timestamp);
        readings_[count_].value = value;
        count_++;
    }

    std::string readReportFile()
    {
        std::ifstream file(kReportPath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

/* UT-51: statusToMessage, negative case (unknown status value) */
TEST_F(ReportTest, UnknownStatusGivesUnknownErrorText)
{
    EXPECT_STREQ("Unknown error.", statusToMessage(static_cast<Status_t>(99)));
    EXPECT_STRNE("", statusToMessage(STATUS_SUCCESS));
}

/* UT-52: alarmStatusToText, positive case */
TEST_F(ReportTest, AlarmStatusTextForHighHigh)
{
    EXPECT_STREQ("HIGH-HIGH", alarmStatusToText(ALARM_HIGH_HIGH));
    EXPECT_STREQ("NORMAL", alarmStatusToText(ALARM_NORMAL));
}

/* UT-53: printTags, negative case (no tags) */
TEST_F(ReportTest, PrintTagsWithoutTagsReturnsEmpty)
{
    EXPECT_EQ(STATUS_EMPTY, printTags(tags_, 0));
}

/* UT-54: printTags, positive case (output lists the tags) */
TEST_F(ReportTest, PrintTagsShowsIdAndDescription)
{
    ::testing::internal::CaptureStdout();
    Status_t status = printTags(tags_, 2);
    std::string output = ::testing::internal::GetCapturedStdout();

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_NE(std::string::npos, output.find("TT-101"));
    EXPECT_NE(std::string::npos, output.find("Boiler inlet"));
}

/* UT-55: printReadings, positive case (alarm status shown per reading) */
TEST_F(ReportTest, PrintReadingsShowsAlarmStatus)
{
    push("TT-101", "2026-11-07 08:00:00", 85.0);

    ::testing::internal::CaptureStdout();
    Status_t status = printReadings(readings_, count_, tags_, 2);
    std::string output = ::testing::internal::GetCapturedStdout();

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_NE(std::string::npos, output.find("85.00"));
    EXPECT_NE(std::string::npos, output.find("HIGH"));
}

/* UT-56: printReadings, negative case (no readings) */
TEST_F(ReportTest, PrintReadingsWithoutReadingsReturnsEmpty)
{
    EXPECT_EQ(STATUS_EMPTY, printReadings(readings_, 0, tags_, 2));
}

/* UT-57: printTagStatistics, positive case (values 10, 20, 30) */
TEST_F(ReportTest, PrintTagStatisticsShowsMeanAndStdDev)
{
    push("TT-101", "2026-11-07 08:00:00", 10.0);
    push("TT-101", "2026-11-07 08:01:00", 20.0);
    push("TT-101", "2026-11-07 08:02:00", 30.0);

    ::testing::internal::CaptureStdout();
    Status_t status = printTagStatistics(&tags_[0], readings_, count_);
    std::string output = ::testing::internal::GetCapturedStdout();

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_NE(std::string::npos, output.find("Mean     : 20.00"));
    EXPECT_NE(std::string::npos, output.find("Std dev  : 8.16"));
}

/* UT-58: printTagStatistics, negative case (tag has no readings) */
TEST_F(ReportTest, PrintTagStatisticsWithoutReadingsReturnsEmpty)
{
    push("TT-102", "2026-11-07 08:00:00", 50.0);

    EXPECT_EQ(STATUS_EMPTY, printTagStatistics(&tags_[0], readings_, count_));
}

/* UT-59: printAlarmSummary, positive case (2 of 5 readings in alarm) */
TEST_F(ReportTest, AlarmSummaryShowsTimeInAlarm)
{
    push("TT-101", "2026-11-07 08:00:00", 50.0);
    push("TT-101", "2026-11-07 08:01:00", 85.0); /* high      */
    push("TT-101", "2026-11-07 08:02:00", 50.0);
    push("TT-101", "2026-11-07 08:03:00", 95.0); /* high-high */
    push("TT-101", "2026-11-07 08:04:00", 50.0);

    ::testing::internal::CaptureStdout();
    Status_t status = printAlarmSummary(tags_, 2, readings_, count_);
    std::string output = ::testing::internal::GetCapturedStdout();

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_NE(std::string::npos, output.find("Time in alarm : 120 s"));
    EXPECT_NE(std::string::npos, output.find("High: 1"));
    EXPECT_NE(std::string::npos, output.find("No readings")); /* TT-102 */
}

/* UT-60: saveShiftReport, positive case (file content) */
TEST_F(ReportTest, ShiftReportFileContainsStatisticsAndNoReadingsNote)
{
    push("TT-101", "2026-11-07 08:00:00", 10.0);
    push("TT-101", "2026-11-07 08:01:00", 20.0);
    push("TT-101", "2026-11-07 08:02:00", 30.0);

    Status_t status = saveShiftReport(kReportPath, tags_, 2, readings_, count_);
    std::string content = readReportFile();

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_NE(std::string::npos, content.find("Period   : 2026-11-07 08:00:00 to 2026-11-07 08:02:00"));
    EXPECT_NE(std::string::npos, content.find("Mean     : 20.00"));
    EXPECT_NE(std::string::npos, content.find("Tag TT-102 - Boiler inlet (degC)\n  No readings"));
}

/* UT-61: saveShiftReport, negative case (folder does not exist) */
TEST_F(ReportTest, ShiftReportToMissingFolderReturnsOpenError)
{
    push("TT-101", "2026-11-07 08:00:00", 10.0);

    Status_t status = saveShiftReport("no_such_folder/report.tmp", tags_, 2, readings_, count_);

    EXPECT_EQ(STATUS_FILE_OPEN_ERROR, status);
}

/* UT-62: saveShiftReport, negative case (no tags) */
TEST_F(ReportTest, ShiftReportWithoutTagsReturnsEmpty)
{
    EXPECT_EQ(STATUS_EMPTY, saveShiftReport(kReportPath, tags_, 0, readings_, 0));
}
