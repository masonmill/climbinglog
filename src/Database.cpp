#include "Database.hpp"

std::ostream& operator<<(std::ostream& os, Board board) {
  switch (board) {
    case Board::MB2019:
      os << "MoonBoard 2019";
      break;

    default:
      os << "N/A";
  }

  return os;
}

std::ostream& operator<<(std::ostream& os, Grade grade) {
  switch (grade) {
    case Grade::V3:
      os << "6a+/V3";
      break;

    default:
      os << "Ungraded";
  }

  return os;
}

std::ostream& operator<<(std::ostream& os, const Entry& entry) {
  os << "\t" << entry.date << "\n";
  os << "\t" << entry.board << " at " << entry.incline << " degrees\n";
  os << "\t" << entry.name << " (" << entry.grade << ")\n";
  if (entry.sent && entry.attempts == 1) os << "\tFlashed!\n";
  if (entry.sent)
    os << "\tSent, " << entry.attempts << " attempts\n";
  else
    os << "\tProject, " << entry.attempts << " attempts\n";

  return os;
}

Database::Database() { log.reserve(16); }

Database::~Database() = default;

void Database::addEntry(const Entry& entry) { log.emplace_back(entry); }

void Database::removeEntry(const uint32_t entryID) {
  // Combination of remove_if and erase?
}

std::ostream& operator<<(std::ostream& os, const Database& db) {
  const std::vector<Entry>& log = db.log;
  os << "Climbing Log\n";

  for (size_t i = 0; i < log.size(); i++) {
    const Entry& entry = log[i];

    os << "[" << i << "]:\n";
    os << entry << "\n";
  }

  return os;
}
