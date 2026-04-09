#pragma once

#include <chrono>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <span>
#include <string>
#include <utility>
#include <vector>

// ─── Enums ───────────────────────────────────────────────────────────────────

enum Board {
  MB2019,
  MB2024,
};

std::ostream& operator<<(std::ostream& os, Board board);

enum Grade {
  V3,
  V4,
  V5,
  V6,
  V7,
};

std::ostream& operator<<(std::ostream& os, Grade grade);

// ─── SessionData ─────────────────────────────────────────────────────────────
//
// Per-day attempt record. Pass this to addSession() to log a new session.

struct SessionData {
  std::chrono::system_clock::time_point timestamp;
  uint32_t attempts;
  double incline;
  bool sent;
};

// ─── Session ─────────────────────────────────────────────────────────────────
//
// Read-only handle returned by the log. IDs are assigned by the owning Climb.

class Session {
 public:
  [[nodiscard]] uint64_t getID() const { return sessionID; }
  [[nodiscard]] const SessionData& getData() const { return data; }

 private:
  friend class Climb;
  friend class ClimbingLog;
  friend Session sessionFromJson(const nlohmann::json& j);

  Session(const uint64_t id, SessionData data)
      : sessionID(id), data(std::move(data)) {}

  uint64_t sessionID;
  SessionData data;
};

// ─── ClimbData ───────────────────────────────────────────────────────────────
//
// Identity of a problem — name + board + grade. Pass this to addClimb().

struct ClimbData {
  std::string name;
  Board board;
  Grade grade;
};

// ─── Climb ───────────────────────────────────────────────────────────────────
//
// A problem with its session history. Construction is private so that IDs
// are always assigned by ClimbingLog.

class Climb {
 public:
  [[nodiscard]] uint64_t getID() const { return climbID; }
  [[nodiscard]] const ClimbData& getData() const { return data; }
  [[nodiscard]] const std::vector<Session>& getSessions() const {
    return sessions;
  }
  [[nodiscard]] size_t sessionCount() const { return sessions.size(); }

  friend std::ostream& operator<<(std::ostream& os, const Climb& climb);

 private:
  friend class ClimbingLog;
  friend nlohmann::json climbToJson(const Climb& climb);

  Climb(const uint64_t id, ClimbData data)
      : climbID(id), data(std::move(data)) {}

  uint64_t addSession(SessionData sessionData);
  void removeSession(uint64_t sessionID);
  Session& getSession(uint64_t sessionID);
  [[nodiscard]] const Session& getSession(uint64_t sessionID) const;
  void setSession(uint64_t sessionID, SessionData sessionData);

  void sortSessionsByTimestamp();
  std::vector<Session>::iterator findSessionByID(uint64_t sessionID);
  [[nodiscard]] std::vector<Session>::const_iterator findSessionByID(
      uint64_t sessionID) const;

  uint64_t climbID;
  ClimbData data;
  uint64_t nextSessionID = 0;
  std::vector<Session> sessions;
};

// ─── ClimbingLog ─────────────────────────────────────────────────────────────

class ClimbingLog {
 public:
  ClimbingLog();
  ~ClimbingLog();

  // ── Climb CRUD ──────────────────────────────────────────────────────────

  // Assigns a new ID and inserts the climb sorted by name.
  uint64_t addClimb(ClimbData data);

  // Throws std::out_of_range if climbID is not found.
  void removeClimb(uint64_t climbID);

  // Throws std::out_of_range if climbID is not found.
  Climb& getClimb(uint64_t climbID);
  [[nodiscard]] const Climb& getClimb(uint64_t climbID) const;

  // Updates the identity fields of a climb. Re-sorts by name.
  // Throws std::out_of_range if climbID is not found.
  void setClimb(uint64_t climbID, ClimbData data);

  [[nodiscard]] std::span<const Climb> getClimbs(size_t offset,
                                                  size_t count) const;

  [[nodiscard]] size_t size() const { return climbs.size(); }
  [[nodiscard]] bool empty() const { return climbs.empty(); }

  // ── Session CRUD (delegated to the owning Climb) ────────────────────────

  // Throws std::out_of_range if climbID is not found.
  uint64_t addSession(uint64_t climbID, SessionData data);
  void removeSession(uint64_t climbID, uint64_t sessionID);
  Session& getSession(uint64_t climbID, uint64_t sessionID);
  [[nodiscard]] const Session& getSession(uint64_t climbID,
                                           uint64_t sessionID) const;
  void setSession(uint64_t climbID, uint64_t sessionID, SessionData data);

  // ── Serialization ───────────────────────────────────────────────────────

  void serializeLog(const std::filesystem::path& path) const;
  void deserializeLog(const std::filesystem::path& path);

  friend std::ostream& operator<<(std::ostream& os, const ClimbingLog& log);

 private:
  std::vector<Climb>::iterator findByID(uint64_t climbID);
  [[nodiscard]] std::vector<Climb>::const_iterator findByID(
      uint64_t climbID) const;

  void sortByName();
  static Climb climbFromJson(const nlohmann::json& j);

  uint64_t nextClimbID = 0;
  std::vector<Climb> climbs;
};
