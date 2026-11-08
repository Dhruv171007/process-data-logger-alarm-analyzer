#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Rewrites the whole file: one line per tag, 7 comma-separated fields.
    Status_t saveTags(const char *path, const Tag_t *tags, int tag_count);

    // Replaces the contents of 'tags' with the tags read from the file.
    // Invalid, duplicate or over-long lines are skipped and counted in *rejected.
    // *tag_count and *rejected are always set once the file was opened and read.
    Status_t loadTags(const char *path, Tag_t *tags, int *tag_count, int *rejected);

    // Rewrites the whole file: one line per reading, "tag_id,timestamp,value".
    Status_t saveReadings(const char *path, const Reading_t *readings, int reading_count);

    // Appends the readings found in a CSV file ("tag_id,timestamp,value", optional
    // header line) to the array. Used both for the user's CSV import and for loading
    // the saved readings file. *imported and *rejected count the lines of this call.
    // Stops with STATUS_CAPACITY_FULL when the array is full.
    Status_t importReadingsCsv(const char *path, Reading_t *readings, int *reading_count,
                               const Tag_t *tags, int tag_count,
                               int *imported, int *rejected);

#ifdef __cplusplus
}
#endif

#endif // FILE_HANDLER_H