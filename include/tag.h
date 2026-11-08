#ifndef TAG_H
#define TAG_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Validates the tag, rejects duplicate IDs and a full array, then appends a copy.
    Status_t addTag(Tag_t *tags, int *tag_count, const Tag_t *new_tag);

    // Searches by ID (case-sensitive). On success *index holds the position.
    Status_t findTagById(const Tag_t *tags, int tag_count, const char *id, int *index);

    // Replaces description, unit and the four limits. The ID cannot be changed.
    Status_t editTag(Tag_t *tags, int tag_count, const char *id, const Tag_t *updated);

    // Removes the tag and shifts the later tags up. Readings are removed separately.
    Status_t deleteTag(Tag_t *tags, int *tag_count, const char *id);

#ifdef __cplusplus
}
#endif

#endif // TAG_H