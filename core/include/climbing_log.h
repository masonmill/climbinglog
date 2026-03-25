// climbing_log.h — C API for ClimbingLog
//
// Opaque handle + plain-C structs so the core can be consumed from Swift via
// a bridging header without requiring C++ interop.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ─── Board ───────────────────────────────────────────────────────────────────

typedef enum {
  CL_BOARD_MB2019 = 0,
  CL_BOARD_MB2024 = 1,
} CLBoard;

// ─── Grade ───────────────────────────────────────────────────────────────────

typedef enum {
  CL_GRADE_V3 = 0,
  CL_GRADE_V4 = 1,
  CL_GRADE_V5 = 2,
  CL_GRADE_V6 = 3,
  CL_GRADE_V7 = 4,
} CLGrade;

// ─── Entry data ──────────────────────────────────────────────────────────────

typedef struct {
  int64_t timestamp;  // Unix epoch seconds
  char name[256];     // null-terminated, max 255 chars
  CLBoard board;
  CLGrade grade;
  uint32_t attempts;
  double incline;
  bool sent;
} CLEntryData;

typedef struct {
  uint64_t id;
  CLEntryData data;
} CLEntry;

// ─── Log lifecycle ───────────────────────────────────────────────────────────

typedef struct CLLog CLLog;

CLLog* cl_log_create(void);
void cl_log_destroy(CLLog* log);

// ─── CRUD ────────────────────────────────────────────────────────────────────
// Functions that can fail return 0 on success, -1 on error (e.g. ID not found).

uint64_t cl_log_add_entry(CLLog* log, CLEntryData data);
int cl_log_remove_entry(CLLog* log, uint64_t entry_id);
int cl_log_get_entry(const CLLog* log, uint64_t entry_id, CLEntry* out);
int cl_log_set_entry(CLLog* log, uint64_t entry_id, CLEntryData data);

// ─── Pagination ──────────────────────────────────────────────────────────────

size_t cl_log_size(const CLLog* log);

// Fills `out_entries` with up to `capacity` entries starting at `offset`.
// Returns the number of entries actually written.
size_t cl_log_get_entries(const CLLog* log, size_t offset, size_t capacity,
                          CLEntry* out_entries);

// ─── Serialization ───────────────────────────────────────────────────────────

int cl_log_serialize(const CLLog* log, const char* path);
int cl_log_deserialize(CLLog* log, const char* path);

#ifdef __cplusplus
}
#endif
