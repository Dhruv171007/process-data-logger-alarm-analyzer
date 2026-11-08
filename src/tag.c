#include "tag.h"

#include "validation.h"

#include <stddef.h>
#include <string.h>

// Checks description, unit and alarm limits (not the ID).
static Status_t validateTagFields(const Tag_t *tag)
{
    Status_t status;

    status = validateDescription(tag->description);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateUnit(tag->unit);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    return validateAlarmLimits(tag->low_low, tag->low, tag->high, tag->high_high);
}

Status_t findTagById(const Tag_t *tags, int tag_count, const char *id, int *index)
{
    int i;

    if (tags == NULL || id == NULL || index == NULL || tag_count < 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < tag_count; i++)
    {
        if (strcmp(tags[i].id, id) == 0)
        {
            *index = i;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

Status_t addTag(Tag_t *tags, int *tag_count, const Tag_t *new_tag)
{
    Status_t status;
    int existing_index;

    if (tags == NULL || tag_count == NULL || new_tag == NULL ||
        *tag_count < 0 || *tag_count > MAX_TAGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    status = validateTagId(new_tag->id);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateTagFields(new_tag);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (findTagById(tags, *tag_count, new_tag->id, &existing_index) == STATUS_SUCCESS)
    {
        return STATUS_DUPLICATE;
    }

    if (*tag_count >= MAX_TAGS)
    {
        return STATUS_CAPACITY_FULL;
    }

    tags[*tag_count] = *new_tag;
    (*tag_count)++;
    return STATUS_SUCCESS;
}

Status_t editTag(Tag_t *tags, int tag_count, const char *id, const Tag_t *updated)
{
    Status_t status;
    int index;

    if (tags == NULL || id == NULL || updated == NULL || tag_count < 0)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    status = findTagById(tags, tag_count, id, &index);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateTagFields(updated);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    // Copy field by field so the stored ID stays unchanged.
    strcpy(tags[index].description, updated->description);
    strcpy(tags[index].unit, updated->unit);
    tags[index].low_low = updated->low_low;
    tags[index].low = updated->low;
    tags[index].high = updated->high;
    tags[index].high_high = updated->high_high;

    return STATUS_SUCCESS;
}

Status_t deleteTag(Tag_t *tags, int *tag_count, const char *id)
{
    Status_t status;
    int index;

    if (tags == NULL || tag_count == NULL || id == NULL ||
        *tag_count < 0 || *tag_count > MAX_TAGS)
    {
        return STATUS_INVALID_ARGUMENT;
    }

    status = findTagById(tags, *tag_count, id, &index);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    memmove(&tags[index], &tags[index + 1],
            (size_t)(*tag_count - index - 1) * sizeof(Tag_t));
    (*tag_count)--;
    return STATUS_SUCCESS;
}