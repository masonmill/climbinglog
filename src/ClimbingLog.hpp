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

// ─── EntryData ───────────────────────────────────────────────────────────────
//
// Plain data bag — no identity, no static counters. Pass this to addEntry()
// to create a new entry, or to setEntry() to overwrite an existing one.

struct EntryData {
  std::chrono::system_clock::time_point timestamp;
  std::string name;
  Board board;
  Grade grade;
  uint32_t attempts;
  double incline;
  bool sent;
};

// ─── Entry ───────────────────────────────────────────────────────────────────
//
// Read-only handle returned by the log. Construction is private so that IDs
// are always assigned by ClimbingLog, never by callers.

class Entry {
 public:
  [[nodiscard]] uint64_t getID() const { return entryID; }
  [[nodiscard]] const EntryData& getData() const { return data; }

  friend std::ostream& operator<<(std::ostream& os, const Entry& entry);

 private:
  friend class ClimbingLog;

  Entry(const uint64_t id, EntryData data)
      : entryID(id), data(std::move(data)) {}

  uint64_t entryID;
  EntryData data;
};

// ─── ClimbingLog ─────────────────────────────────────────────────────────────

class ClimbingLog {
 public:
  ClimbingLog();
  ~ClimbingLog();

  // Assigns a new ID and inserts the entry sorted by timestamp.
  // Returns the assigned ID.
  uint64_t addEntry(EntryData data);

  // Throws std::out_of_range if entryID is not found.
  void removeEntry(uint64_t entryID);

  // Throws std::out_of_range if entryID is not found.
  Entry& getEntry(uint64_t entryID);
  [[nodiscard]] const Entry& getEntry(uint64_t entryID) const;

  // Replaces the data of an existing entry without touching its ID.
  // Re-sorts by timestamp since the new data may have a different timestamp.
  // Throws std::out_of_range if entryID is not found.
  void setEntry(uint64_t entryID, EntryData data);

  // Returns a view of `count` entries starting at `offset`.
  // The span is valid until the next mutating operation on the log.
  [[nodiscard]] std::span<const Entry> getEntries(size_t offset,
                                                  size_t count) const;

  [[nodiscard]] size_t size() const { return entries.size(); }
  [[nodiscard]] bool empty() const { return entries.empty(); }

  // Sorts entries by timestamp, then writes the log to a JSON file.
  // Throws std::runtime_error on failure.
  void serializeLog(const std::filesystem::path& path) const;

  // Reads a JSON file and reconstructs the log, restoring nextID.
  // Throws std::runtime_error on failure.
  void deserializeLog(const std::filesystem::path& path);

  friend std::ostream& operator<<(std::ostream& os, const ClimbingLog& log);

 private:
  std::vector<Entry>::iterator findByID(uint64_t entryID);
  [[nodiscard]] std::vector<Entry>::const_iterator findByID(
      uint64_t entryID) const;

  void sortByTimestamp();
  static Entry entryFromJson(const nlohmann::json& j);

  uint64_t nextID = 0;
  std::vector<Entry> entries;
};
