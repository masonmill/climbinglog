#pragma once

#include <string>
#include <vector>

enum Board {
  MB2019,
};

enum Grade {
  V3,
};

// TODO: Replace with std::chrono
struct Date {
  uint32_t month;
  uint32_t day;
  uint32_t year;
};

struct Entry {
  uint32_t entryID;
  Date date;
  std::string name;
  Board board;
  Grade grade;
  uint32_t attempts;
  uint32_t incline;
  bool sent;
};

class Database {
public:
  Database();

  ~Database();

  static void addEntry(Entry entry);

  static void removeEntry(uint32_t entryID);


private:
  std::vector<Entry> log;

};