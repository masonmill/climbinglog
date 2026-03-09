#include "Database.hpp"

#include <algorithm>
#include <stdexcept>

uint64_t Entry::entries = 0;

ClimbingLog::ClimbingLog() { entries.reserve(512); }

ClimbingLog::~ClimbingLog() = default;

uint64_t ClimbingLog::addEntry(const Entry& entry) {
  entries.emplace_back(entry);
  return entries.back().entryID;
}

void ClimbingLog::removeEntry(const uint64_t entryID) {
  const auto it = findByID(entryID);
  if (it == entries.end())
    throw std::out_of_range("removeEntry: entryID " + std::to_string(entryID) +
                            " not found");
  entries.erase(it);
}

Entry& ClimbingLog::getEntry(const uint64_t entryID) {
  const auto it = findByID(entryID);
  if (it == entries.end())
    throw std::out_of_range("getEntry: entryID " + std::to_string(entryID) +
                            " not found");
  return *it;
}

const Entry& ClimbingLog::getEntry(const uint64_t entryID) const {
  const auto it = findByID(entryID);
  if (it == entries.end())
    throw std::out_of_range("getEntry: entryID " + std::to_string(entryID) +
                            " not found");
  return *it;
}

void ClimbingLog::setEntry(const uint64_t entryID, Entry entry) {
  const auto it = findByID(entryID);
  if (it == entries.end())
    throw std::out_of_range("setEntry: entryID " + std::to_string(entryID) +
                            " not found");
  entry.entryID = entryID;
  *it = std::move(entry);
}

std::vector<Entry>::iterator ClimbingLog::findByID(uint64_t entryID) {
  return std::ranges::find_if(
      entries, [entryID](const Entry& e) { return e.entryID == entryID; });
}

std::vector<Entry>::const_iterator ClimbingLog::findByID(
    uint64_t entryID) const {
  return std::ranges::find_if(
      entries, [entryID](const Entry& e) { return e.entryID == entryID; });
}

std::ostream& operator<<(std::ostream& os, const Board board) {
  switch (board) {
    case Board::MB2019:
      os << "MoonBoard 2019";
      break;
    default:
      os << "N/A";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Grade grade) {
  switch (grade) {
    case Grade::V3:
      os << "6a+/V3";
      break;
    case Grade::V4:
      os << "6b/V4";
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

  if (entry.sent && entry.attempts == 1)
    os << "\tFlashed!\n";
  else if (entry.sent)
    os << "\tSent in " << entry.attempts << " attempts\n";
  else
    os << "\tProject, " << entry.attempts << " attempts so far\n";

  return os;
}

std::ostream& operator<<(std::ostream& os, const ClimbingLog& log) {
  os << "Climbing Log (" << log.entries.size() << " entries)\n";
  for (size_t i = 0; i < log.entries.size(); i++) {
    os << "[" << i << "] (ID " << log.entries[i] << "):\n";
    os << log.entries[i] << "\n";
  }
  return os;
}
