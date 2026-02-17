#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

enum Board {
  MB2019,
};

std::ostream& operator<<(std::ostream& os, Board board);

enum Grade {
  V3,
};

std::ostream& operator<<(std::ostream& os, Grade grade);

struct Entry {
  uint32_t entryID;
  std::chrono::year_month_day date;
  std::string name;
  Board board;
  Grade grade;
  uint32_t attempts;
  double incline;
  bool sent;
};

std::ostream& operator<<(std::ostream& os, const Entry& entry);

class Database {
public:
  Database();

  void addEntry(const Entry& entry);

  void removeEntry(uint32_t entryID);

  // TODO: addEntry overload takes Entry params, serializeLog, deserializeLog, readEntry, updateEntry
  // TODO: some sort of iterator friend class?

  ~Database();

  friend std::ostream& operator<<(std::ostream& os, const Database& db);


private:
  // TODO: Does std::vector make sense here?
  std::vector<Entry> log;

};
