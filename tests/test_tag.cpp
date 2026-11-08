#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>

#include "tag.h"

/* Every test starts with an empty tag array. */
class TagTest : public ::testing::Test
{
protected:
    Tag_t tags_[MAX_TAGS];
    int   count_;

    void SetUp() override
    {
        std::memset(tags_, 0, sizeof(tags_));
        count_ = 0;
    }

    /* Builds a valid tag with limits 10 / 20 / 80 / 90. */
    Tag_t makeTag(const char *id)
    {
        Tag_t tag;

        std::memset(&tag, 0, sizeof(tag));
        std::strcpy(tag.id, id);
        std::strcpy(tag.description, "Boiler outlet temperature");
        std::strcpy(tag.unit, "degC");
        tag.low_low   = 10.0;
        tag.low       = 20.0;
        tag.high      = 80.0;
        tag.high_high = 90.0;
        return tag;
    }
};

/* UT-21: addTag, positive case */
TEST_F(TagTest, AddValidTagIncreasesCount)
{
    Tag_t tag = makeTag("TT-101");

    Status_t status = addTag(tags_, &count_, &tag);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_EQ(1, count_);
    EXPECT_STREQ("TT-101", tags_[0].id);
}

/* UT-22: addTag, negative case (duplicate ID) */
TEST_F(TagTest, AddDuplicateTagIdIsRejected)
{
    Tag_t tag = makeTag("TT-101");

    addTag(tags_, &count_, &tag);
    Status_t status = addTag(tags_, &count_, &tag);

    EXPECT_EQ(STATUS_DUPLICATE, status);
    EXPECT_EQ(1, count_);
}

/* UT-23: addTag, boundary case (array already holds MAX_TAGS tags) */
TEST_F(TagTest, AddTagToFullArrayReturnsCapacityFull)
{
    char id[MAX_TAG_ID_LEN + 1];

    for (int i = 0; i < MAX_TAGS; i++)
    {
        std::snprintf(id, sizeof(id), "TT-%03d", i);
        Tag_t tag = makeTag(id);
        ASSERT_EQ(STATUS_SUCCESS, addTag(tags_, &count_, &tag));
    }

    Tag_t extra = makeTag("XX-999");
    Status_t status = addTag(tags_, &count_, &extra);

    EXPECT_EQ(STATUS_CAPACITY_FULL, status);
    EXPECT_EQ(MAX_TAGS, count_);
}

/* UT-24: addTag, negative case (limits not in increasing order) */
TEST_F(TagTest, AddTagWithInvalidLimitsIsRejected)
{
    Tag_t tag = makeTag("TT-101");
    tag.high = 15.0;   /* high below low */

    Status_t status = addTag(tags_, &count_, &tag);

    EXPECT_EQ(STATUS_INVALID_INPUT, status);
    EXPECT_EQ(0, count_);
}

/* UT-25: findTagById, negative case (tag does not exist) */
TEST_F(TagTest, FindMissingTagReturnsNotFound)
{
    Tag_t tag = makeTag("TT-101");
    int index = -1;

    addTag(tags_, &count_, &tag);
    Status_t status = findTagById(tags_, count_, "NO-TAG", &index);

    EXPECT_EQ(STATUS_NOT_FOUND, status);
    EXPECT_EQ(-1, index);   /* index must stay unchanged */
}

/* UT-26: editTag, positive case (limits and description change, ID stays) */
TEST_F(TagTest, EditTagChangesLimitsButNotId)
{
    Tag_t tag = makeTag("TT-101");
    addTag(tags_, &count_, &tag);

    Tag_t updated = makeTag("IGNORED");
    std::strcpy(updated.description, "Updated description");
    updated.high = 70.0;

    Status_t status = editTag(tags_, count_, "TT-101", &updated);

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_STREQ("TT-101", tags_[0].id);
    EXPECT_STREQ("Updated description", tags_[0].description);
    EXPECT_NEAR(70.0, tags_[0].high, 1e-9);
}

/* UT-27: deleteTag, positive case (later tags shift up) */
TEST_F(TagTest, DeleteTagShiftsRemainingTagsUp)
{
    Tag_t first  = makeTag("TT-101");
    Tag_t second = makeTag("TT-102");
    Tag_t third  = makeTag("TT-103");

    addTag(tags_, &count_, &first);
    addTag(tags_, &count_, &second);
    addTag(tags_, &count_, &third);

    Status_t status = deleteTag(tags_, &count_, "TT-102");

    EXPECT_EQ(STATUS_SUCCESS, status);
    EXPECT_EQ(2, count_);
    EXPECT_STREQ("TT-101", tags_[0].id);
    EXPECT_STREQ("TT-103", tags_[1].id);
}