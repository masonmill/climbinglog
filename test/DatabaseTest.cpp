#include <iostream>
#include <memory>

#include "Database.hpp"

// TODO: unit testing macros?
int main() {
  Database db;

  const Entry testEntry{
      .entryID = 0,
      .date = std::chrono::year_month_day(
          std::chrono::year(2026), std::chrono::month(2), std::chrono::day(17)),
      .name = "Entry",
      .board = MB2019,
      .grade = V3,
      .attempts = 4,
      .incline = 40.0,
      .sent = true,
  };

  db.addEntry(testEntry);

  std::cout << db << std::endl;

  return 0;
}