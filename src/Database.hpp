#pragma once

#include <chrono>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

enum Board {
  MB2019,
};

std::ostream& operator<<(std::ostream& os, Board board);

enum Grade {
  V3,
  V4,
};

std::ostream& operator<<(std::ostream& os, Grade grade);

class Entry {
 public:
  Entry(const std::chrono::year_month_day date, std::string name,
        const Board board, const Grade grade, const uint32_t attempts,
        const double incline, const bool sent)
      : entryID(entries++),
        date(date),
        name(std::move(name)),
        board(board),
        grade(grade),
        attempts(attempts),
        incline(incline),
        sent(sent) {}

  [[nodiscard]] uint64_t getID() const { return entryID; }

  friend std::ostream& operator<<(std::ostream& os, const Entry& entry);

 private:
  friend class ClimbingLog;

  static uint64_t entries;

  uint64_t entryID;
  std::chrono::year_month_day date;
  std::string name;
  Board board;
  Grade grade;
  uint32_t attempts;
  double incline;
  bool sent;
};

class ClimbingLog {
 public:
  ClimbingLog();
  ~ClimbingLog();

  uint64_t addEntry(const Entry& entry);

  void removeEntry(uint64_t entryID);

  Entry& getEntry(uint64_t entryID);
  [[nodiscard]] const Entry& getEntry(uint64_t entryID) const;

  void setEntry(uint64_t entryID, Entry entry);

  [[nodiscard]] size_t size() const { return entries.size(); }
  [[nodiscard]] bool empty() const { return entries.empty(); }

  friend std::ostream& operator<<(std::ostream& os, const ClimbingLog& log);

 private:
  std::vector<Entry>::iterator findByID(uint64_t entryID);
  [[nodiscard]] std::vector<Entry>::const_iterator findByID(
      uint64_t entryID) const;

  std::vector<Entry> entries;
};
