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

// ─── Climb data ──────────────────────────────────────────────────────────────

typedef struct {
  char name[256];
  CLBoard board;
  CLGrade grade;
} CLClimbData;

typedef struct {
  uint64_t id;
  CLClimbData data;
} CLClimb;

// ─── Session data ────────────────────────────────────────────────────────────

typedef struct {
  int64_t timestamp;  // Unix epoch seconds
  uint32_t attempts;
  double incline;
  bool sent;
} CLSessionData;

typedef struct {
  uint64_t id;
  CLSessionData data;
} CLSession;

// ─── Log lifecycle ───────────────────────────────────────────────────────────

typedef struct CLLog CLLog;

CLLog* cl_log_create(void);
void cl_log_destroy(CLLog* log);

// ─── Climb CRUD ──────────────────────────────────────────────────────────────
// Functions that can fail return 0 on success, -1 on error (e.g. ID not found).

uint64_t cl_log_add_climb(CLLog* log, CLClimbData data);
int cl_log_remove_climb(CLLog* log, uint64_t climb_id);
int cl_log_get_climb(const CLLog* log, uint64_t climb_id, CLClimb* out);
int cl_log_set_climb(CLLog* log, uint64_t climb_id, CLClimbData data);

size_t cl_log_climb_count(const CLLog* log);
size_t cl_log_get_climbs(const CLLog* log, size_t offset, size_t capacity,
                          CLClimb* out_climbs);

// ─── Session CRUD ────────────────────────────────────────────────────────────

uint64_t cl_log_add_session(CLLog* log, uint64_t climb_id,
                             CLSessionData data);
int cl_log_remove_session(CLLog* log, uint64_t climb_id, uint64_t session_id);
int cl_log_get_session(const CLLog* log, uint64_t climb_id,
                        uint64_t session_id, CLSession* out);
int cl_log_set_session(CLLog* log, uint64_t climb_id, uint64_t session_id,
                        CLSessionData data);

size_t cl_log_session_count(const CLLog* log, uint64_t climb_id);
size_t cl_log_get_sessions(const CLLog* log, uint64_t climb_id, size_t offset,
                            size_t capacity, CLSession* out_sessions);

// ─── Serialization ───────────────────────────────────────────────────────────

int cl_log_serialize(const CLLog* log, const char* path);
int cl_log_deserialize(CLLog* log, const char* path);

#ifdef __cplusplus
}
#endif
