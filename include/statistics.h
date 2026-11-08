#ifndef STATISTICS_H
#define STATISTICS_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Each function writes its answer to *result only when it returns STATUS_SUCCESS. */
Status_t calculateMean(const double *values, int count, double *result);
Status_t calculateStdDev(const double *values, int count, double *result);
Status_t calculateMin(const double *values, int count, double *result);
Status_t calculateMax(const double *values, int count, double *result);

#ifdef __cplusplus
}
#endif

#endif /* STATISTICS_H */