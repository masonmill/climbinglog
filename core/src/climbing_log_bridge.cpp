#include "climbing_log.h"
#include "ClimbingLog.hpp"

#include <chrono>
#include <cstring>
#include <stdexcept>

// ─── Helpers ─────────────────────────────────────────────────────────────────

static CLEntryData toCEntryData(const EntryData& d) {
  CLEntryData out{};
  out.timestamp =
      std::chrono::duration_cast<std::chrono::seconds>(
          d.timestamp.time_since_epoch())
          .count();
  std::strncpy(out.name, d.name.c_str(), sizeof(out.name) - 1);
  out.name[sizeof(out.name) - 1] = '\0';
  out.board    = static_cast<CLBoard>(d.board);
  out.grade    = static_cast<CLGrade>(d.grade);
  out.attempts = d.attempts;
  out.incline  = d.incline;
  out.sent     = d.sent;
  return out;
}

static EntryData fromCEntryData(const CLEntryData& d) {
  using namespace std::chrono;
  return EntryData{
      .timestamp = system_clock::time_point(seconds(d.timestamp)),
      .name      = d.name,
      .board     = static_cast<Board>(d.board),
      .grade     = static_cast<Grade>(d.grade),
      .attempts  = d.attempts,
      .incline   = d.incline,
      .sent      = d.sent,
  };
}

// ─── C API ───────────────────────────────────────────────────────────────────

extern "C" {

CLLog* cl_log_create(void) {
  return reinterpret_cast<CLLog*>(new ClimbingLog());
}

void cl_log_destroy(CLLog* log) {
  delete reinterpret_cast<ClimbingLog*>(log);
}

uint64_t cl_log_add_entry(CLLog* log, CLEntryData data) {
  return reinterpret_cast<ClimbingLog*>(log)->addEntry(fromCEntryData(data));
}

int cl_log_remove_entry(CLLog* log, uint64_t entry_id) {
  try {
    reinterpret_cast<ClimbingLog*>(log)->removeEntry(entry_id);
    return 0;
  } catch (const std::out_of_range&) {
    return -1;
  }
}

int cl_log_get_entry(const CLLog* log, uint64_t entry_id, CLEntry* out) {
  try {
    const Entry& e =
        reinterpret_cast<const ClimbingLog*>(log)->getEntry(entry_id);
    out->id   = e.getID();
    out->data = toCEntryData(e.getData());
    return 0;
  } catch (const std::out_of_range&) {
    return -1;
  }
}

int cl_log_set_entry(CLLog* log, uint64_t entry_id, CLEntryData data) {
  try {
    reinterpret_cast<ClimbingLog*>(log)->setEntry(entry_id,
                                                   fromCEntryData(data));
    return 0;
  } catch (const std::out_of_range&) {
    return -1;
  }
}

size_t cl_log_size(const CLLog* log) {
  return reinterpret_cast<const ClimbingLog*>(log)->size();
}

size_t cl_log_get_entries(const CLLog* log, size_t offset, size_t capacity,
                          CLEntry* out_entries) {
  const auto* cpp_log = reinterpret_cast<const ClimbingLog*>(log);
  const auto  span    = cpp_log->getEntries(offset, capacity);
  size_t      n       = 0;
  for (const Entry& e : span) {
    out_entries[n].id   = e.getID();
    out_entries[n].data = toCEntryData(e.getData());
    ++n;
  }
  return n;
}

int cl_log_serialize(const CLLog* log, const char* path) {
  try {
    reinterpret_cast<const ClimbingLog*>(log)->serializeLog(path);
    return 0;
  } catch (...) {
    return -1;
  }
}

int cl_log_deserialize(CLLog* log, const char* path) {
  try {
    reinterpret_cast<ClimbingLog*>(log)->deserializeLog(path);
    return 0;
  } catch (...) {
    return -1;
  }
}

}  // extern "C"
