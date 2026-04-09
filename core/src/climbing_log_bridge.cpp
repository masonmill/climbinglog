#include <chrono>
#include <cstring>
#include <stdexcept>

#include "ClimbingLog.hpp"
#include "climbing_log.h"

// ─── Helpers ─────────────────────────────────────────────────────────────────

static CLClimbData toCClimbData(const ClimbData& d) {
  CLClimbData out{};
  std::strncpy(out.name, d.name.c_str(), sizeof(out.name) - 1);
  out.name[sizeof(out.name) - 1] = '\0';
  out.board = static_cast<CLBoard>(d.board);
  out.grade = static_cast<CLGrade>(d.grade);
  return out;
}

static ClimbData fromCClimbData(const CLClimbData& d) {
  return ClimbData{
      .name = d.name,
      .board = static_cast<Board>(d.board),
      .grade = static_cast<Grade>(d.grade),
  };
}

static CLSessionData toCSessionData(const SessionData& d) {
  CLSessionData out{};
  out.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                      d.timestamp.time_since_epoch())
                      .count();
  out.attempts = d.attempts;
  out.incline = d.incline;
  out.sent = d.sent;
  return out;
}

static SessionData fromCSessionData(const CLSessionData& d) {
  using namespace std::chrono;
  return SessionData{
      .timestamp = system_clock::time_point(seconds(d.timestamp)),
      .attempts = d.attempts,
      .incline = d.incline,
      .sent = d.sent,
  };
}

// ─── C API ───────────────────────────────────────────────────────────────────

extern "C" {

CLLog* cl_log_create(void) {
  return reinterpret_cast<CLLog*>(new ClimbingLog());
}

void cl_log_destroy(CLLog* log) { delete reinterpret_cast<ClimbingLog*>(log); }

// ── Climb CRUD ──────────────────────────────────────────────────────────────

uint64_t cl_log_add_climb(CLLog* log, CLClimbData data) {
  return reinterpret_cast<ClimbingLog*>(log)->addClimb(fromCClimbData(data));
}

int cl_log_remove_climb(CLLog* log, uint64_t climb_id) {
  try {
    reinterpret_cast<ClimbingLog*>(log)->removeClimb(climb_id);
    return 0;
  } catch (const std::out_of_range&) {
    return -1;
  }
}

int cl_log_get_climb(const CLLog* log, uint64_t climb_id, CLClimb* out) {
  try {
    const auto& c = reinterpret_cast<const ClimbingLog*>(log)->getClimb(climb_id);
    out->id = c.getID();
    out->data = toCClimbData(c.getData());
    return 0;
  } catch (const std::out_of_range&) {
    return -1;
  }
}

int cl_log_set_climb(CLLog* log, uint64_t climb_id, CLClimbData data) {
  try {
    reinterpret_cast<ClimbingLog*>(log)->setClimb(climb_id,
                                                   fromCClimbData(data));
    return 0;
  } catch (const std::out_of_range&) {
    return -1;
  }
}

size_t cl_log_climb_count(const CLLog* log) {
  return reinterpret_cast<const ClimbingLog*>(log)->size();
}

size_t cl_log_get_climbs(const CLLog* log, size_t offset, size_t capacity,
                          CLClimb* out_climbs) {
  const auto* cpp_log = reinterpret_cast<const ClimbingLog*>(log);
  const auto span = cpp_log->getClimbs(offset, capacity);
  size_t n = 0;
  for (const auto& c : span) {
    out_climbs[n].id = c.getID();
    out_climbs[n].data = toCClimbData(c.getData());
    ++n;
  }
  return n;
}

// ── Session CRUD ────────────────────────────────────────────────────────────

uint64_t cl_log_add_session(CLLog* log, uint64_t climb_id,
                             CLSessionData data) {
  return reinterpret_cast<ClimbingLog*>(log)->addSession(
      climb_id, fromCSessionData(data));
}

int cl_log_remove_session(CLLog* log, uint64_t climb_id, uint64_t session_id) {
  try {
    reinterpret_cast<ClimbingLog*>(log)->removeSession(climb_id, session_id);
    return 0;
  } catch (const std::out_of_range&) {
    return -1;
  }
}

int cl_log_get_session(const CLLog* log, uint64_t climb_id,
                        uint64_t session_id, CLSession* out) {
  try {
    const auto& s =
        reinterpret_cast<const ClimbingLog*>(log)->getSession(climb_id,
                                                               session_id);
    out->id = s.getID();
    out->data = toCSessionData(s.getData());
    return 0;
  } catch (const std::out_of_range&) {
    return -1;
  }
}

int cl_log_set_session(CLLog* log, uint64_t climb_id, uint64_t session_id,
                        CLSessionData data) {
  try {
    reinterpret_cast<ClimbingLog*>(log)->setSession(climb_id, session_id,
                                                     fromCSessionData(data));
    return 0;
  } catch (const std::out_of_range&) {
    return -1;
  }
}

size_t cl_log_session_count(const CLLog* log, uint64_t climb_id) {
  try {
    return reinterpret_cast<const ClimbingLog*>(log)
        ->getClimb(climb_id)
        .sessionCount();
  } catch (const std::out_of_range&) {
    return 0;
  }
}

size_t cl_log_get_sessions(const CLLog* log, uint64_t climb_id, size_t offset,
                            size_t capacity, CLSession* out_sessions) {
  try {
    const auto& climb =
        reinterpret_cast<const ClimbingLog*>(log)->getClimb(climb_id);
    const auto& sessions = climb.getSessions();
    if (offset >= sessions.size()) return 0;
    const size_t available = sessions.size() - offset;
    const size_t count = std::min(capacity, available);
    for (size_t i = 0; i < count; ++i) {
      out_sessions[i].id = sessions[offset + i].getID();
      out_sessions[i].data = toCSessionData(sessions[offset + i].getData());
    }
    return count;
  } catch (const std::out_of_range&) {
    return 0;
  }
}

// ── Serialization ───────────────────────────────────────────────────────────

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
