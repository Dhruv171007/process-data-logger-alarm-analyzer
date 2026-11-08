#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

#include "file_handler.h"

/* The tests write temporary files into the build folder and delete them again.
   They must be run from the project root (make test does this). */
static const char *kTagsPath = "build/ut_tags.tmp";
static const char *kReadingsPath = "build/ut_readings.tmp";
static const char *kMissingPath = "build/ut_does_not_exist.tmp";

class FileHandlerTest : public ::testing::Test
{
protected:
    Tag_t tags_[MAX_TAGS];
    Reading_t readings_[MAX_READINGS];
    int tag_count_;
    int reading_count_;

    void SetUp() override
    {
        std::memset(tags_, 0, sizeof(tags_));
        std::memset(readings_, 0, sizeof(readings_));
        tag_count_ = 0;
        reading_count_ = 0;
        std::remove(kTagsPath);
        std::remove(kReadingsPath);
    }

    void TearDown() override
    {
        std::remove(kTagsPath);
        std::remove(kReadingsPath);
    }

    void writeFile(const char *path, const std::string &content)
    {
        std::ofstream file(path);
        file << content;
    }

    void setupTag(Tag_t *tag, const char *id, const char *description)
    {
        std::memset(tag, 0, sizeof(*tag));
        std::strcpy(tag->id, id);
        std::strcpy(tag->description, description);
        std::strcpy(tag->unit, "degC");
        tag->low_low = 10.0;
        tag->low = 20.0;
        tag->high = 80.0;
        tag->high_high = 90.0;
    }

    /* Two tags used by the reading tests. */
    void setupTwoTags()
    {
        setupTag(&tags_[0], "TT-101", "Boiler outlet");
        setupTag(&tags_[1], "TT-102", "Boiler inlet");
        tag_count_ = 2;
    }
};

/* UT-41: saveTags then loadTags, positive case (round trip) */
TEST_F(FileHandlerTest, SavedTagsCanBeLoadedAgain)
{
    Tag_t loaded[MAX_TAGS];
    int loaded_count = -1;
    int rejected = -1;

    setupTwoTags();

    EXPECT_EQ(STATUS_SUCCESS, saveTags(kTagsPath, tags_, tag_count_));
    EXPECT_EQ(STATUS_SUCCESS, loadTags(kTagsPath, loaded, &loaded_count, &rejected));

    ASSERT_EQ(2, loaded_count);
    EXPECT_EQ(0, rejected);
    EXPECT_STREQ("TT-102", loaded[1].id);
    EXPECT_STREQ("Boiler inlet", loaded[1].description);
    EXPECT_NEAR(90.0, loaded[1].high_high, 1e-9);
}

/* UT-42: loadTags, negative case (file does not exist) */
TEST_F(FileHandlerTest, LoadTagsFromMissingFileReturnsOpenError)
{
    int rejected = -1;

    Status_t status = loadTags(kMissingPath, tags_, &tag_count_, &rejected);

    EXPECT_EQ(STATUS_FILE_OPEN_ERROR, status);
}

/* UT-43: loadTags, negative case (bad lines are skipped and counted) */
TEST_F(FileHandlerTest, LoadTagsSkipsInvalidLinesAndCountsThem)
{
    int rejected = -1;

    writeFile(kTagsPath,
              "TT-101,Boiler outlet,degC,10,20,80,90\n"
              "TT-102,Only six fields,degC,10,20,80\n"
              "\n"
              "TT-103,Limits reversed,degC,90,80,20,10\n"
              "TT-104,Boiler inlet,degC,10,20,80,90\n");

    Status_t status = loadTags(kTagsPath, tags_, &tag_count_, &rejected);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_EQ(2, tag_count_);
    EXPECT_EQ(2, rejected);
    EXPECT_STREQ("TT-104", tags_[1].id);
}

/* UT-44: loadTags, negative case (every line is invalid) */
TEST_F(FileHandlerTest, LoadTagsWithOnlyInvalidLinesReturnsInvalidFileData)
{
    int rejected = -1;

    writeFile(kTagsPath, "this is not a tag\nneither is this\n");

    Status_t status = loadTags(kTagsPath, tags_, &tag_count_, &rejected);

    EXPECT_EQ(STATUS_INVALID_FILE_DATA, status);
    EXPECT_EQ(0, tag_count_);
    EXPECT_EQ(2, rejected);
}

/* UT-45: loadTags, boundary case (a line longer than MAX_LINE_LEN) */
TEST_F(FileHandlerTest, LoadTagsRejectsOverlongLineAndContinues)
{
    int rejected = -1;
    std::string long_line(300, 'A');

    writeFile(kTagsPath,
              long_line + "\n"
                          "TT-101,Boiler outlet,degC,10,20,80,90\n");

    Status_t status = loadTags(kTagsPath, tags_, &tag_count_, &rejected);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_EQ(1, tag_count_);
    EXPECT_EQ(1, rejected);
}

/* UT-46: saveReadings then importReadingsCsv, positive case (round trip) */
TEST_F(FileHandlerTest, SavedReadingsCanBeImportedAgain)
{
    int imported = -1;
    int rejected = -1;

    setupTwoTags();

    std::strcpy(readings_[0].tag_id, "TT-101");
    std::strcpy(readings_[0].timestamp, "2026-11-07 08:00:00");
    readings_[0].value = 72.5;
    std::strcpy(readings_[1].tag_id, "TT-102");
    std::strcpy(readings_[1].timestamp, "2026-11-07 09:00:00");
    readings_[1].value = 15.25;

    ASSERT_EQ(STATUS_SUCCESS, saveReadings(kReadingsPath, readings_, 2));

    Reading_t loaded[MAX_READINGS];
    int loaded_count = 0;
    Status_t status = importReadingsCsv(kReadingsPath, loaded, &loaded_count,
                                        tags_, tag_count_, &imported, &rejected);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_EQ(2, imported);
    EXPECT_EQ(0, rejected);
    ASSERT_EQ(2, loaded_count);
    EXPECT_STREQ("2026-11-07 09:00:00", loaded[1].timestamp);
    EXPECT_NEAR(15.25, loaded[1].value, 1e-9);
}

/* UT-47: importReadingsCsv, negative case (header skipped, bad lines counted) */
TEST_F(FileHandlerTest, ImportSkipsHeaderAndCountsRejectedLines)
{
    int imported = -1;
    int rejected = -1;

    setupTwoTags();

    writeFile(kReadingsPath,
              "tag_id,timestamp,value\n"
              "TT-101,2026-11-07 08:00:00,50.0\n"
              "TT-102,2026-11-07 08:01:00,60.0\n"
              "XX-999,2026-11-07 08:02:00,50.0\n"  /* unknown tag        */
              "TT-101,2026-13-07 08:03:00,50.0\n"  /* invalid timestamp  */
              "TT-101,2026-11-07 08:04:00,abc\n"); /* value not a number */

    Status_t status = importReadingsCsv(kReadingsPath, readings_, &reading_count_,
                                        tags_, tag_count_, &imported, &rejected);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_EQ(2, imported);
    EXPECT_EQ(3, rejected);
    EXPECT_EQ(2, reading_count_);
}

/* UT-48: importReadingsCsv, boundary case (array becomes full) */
TEST_F(FileHandlerTest, ImportStopsWhenArrayIsFull)
{
    int imported = -1;
    int rejected = -1;

    setupTwoTags();
    reading_count_ = MAX_READINGS - 1; /* room for exactly one more reading */

    writeFile(kReadingsPath,
              "TT-101,2026-11-07 08:00:00,50.0\n"
              "TT-101,2026-11-07 08:01:00,51.0\n"
              "TT-101,2026-11-07 08:02:00,52.0\n");

    Status_t status = importReadingsCsv(kReadingsPath, readings_, &reading_count_,
                                        tags_, tag_count_, &imported, &rejected);

    EXPECT_EQ(STATUS_CAPACITY_FULL, status);
    EXPECT_EQ(1, imported);
    EXPECT_EQ(MAX_READINGS, reading_count_);
}

/* UT-49: importReadingsCsv, negative case (file does not exist) */
TEST_F(FileHandlerTest, ImportFromMissingFileReturnsOpenError)
{
    int imported = -1;
    int rejected = -1;

    setupTwoTags();

    Status_t status = importReadingsCsv(kMissingPath, readings_, &reading_count_,
                                        tags_, tag_count_, &imported, &rejected);
        
    EXPECT_EQ(STATUS_FILE_OPEN_ERROR, status);
    EXPECT_EQ(0, reading_count_);
}

/* UT-50: saveTags, negative case (folder does not exist) */
TEST_F(FileHandlerTest, SaveTagsToMissingFolderReturnsOpenError)
{
    setupTwoTags();

    Status_t status = saveTags("no_such_folder/tags.tmp", tags_, tag_count_);

    EXPECT_EQ(STATUS_FILE_OPEN_ERROR, status);
}