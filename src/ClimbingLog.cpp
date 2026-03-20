#include "ClimbingLog.hpp"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

// ─── Enum helpers ────────────────────────────────────────────────────────────

static std::string boardToString(const Board board) {
  switch (board) {
    case MB2019:
      return "MoonBoard 2019";
    case MB2024:
      return "MoonBoard 2024";
    default:
      return "Unknown";
  }
}

static Board boardFromString(const std::string& s) {
  if (s == "MoonBoard 2019") return MB2019;
  if (s == "MoonBoard 2024") return MB2024;
  throw std::runtime_error("Unknown board: " + s);
}

static std::string gradeToString(const Grade grade) {
  switch (grade) {
    case V3:
      return "6a+/V3";
    case V4:
      return "6b/V4";
    case V5:
      return "6c/V5";
    case V6:
      return "7a/V6";
    case V7:
      return "7a+/V7";
    default:
      return "Ungraded";
  }
}

static Grade gradeFromString(const std::string& s) {
  if (s == "6a+/V3") return V3;
  if (s == "6b/V4") return V4;
  if (s == "6c/V5") return V5;
  if (s == "7a/V6") return V6;
  if (s == "7a+/V7") return V7;
  throw std::runtime_error("Unknown grade: " + s);
}

// ─── JSON conversion ─────────────────────────────────────────────────────────
//
// Timestamps are stored as Unix epoch seconds (int64_t).
// JavaScript reads them directly via new Date(ts * 1000).

static json entryToJson(const Entry& entry) {
  const auto& [timestamp, name, board, grade, attempts, incline, sent] =
      entry.getData();
  const int64_t epochSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                                   timestamp.time_since_epoch())
                                   .count();

  return {
      {"id", entry.getID()},
      {"timestamp", epochSeconds},
      {"name", name},
      {"board", boardToString(board)},
      {"grade", gradeToString(grade)},
      {"attempts", attempts},
      {"incline", incline},
      {"sent", sent},
  };
}

// static
Entry ClimbingLog::entryFromJson(const json& j) {
  const int64_t epochSeconds = j.at("timestamp").get<int64_t>();
  const auto timestamp =
      std::chrono::system_clock::time_point(std::chrono::seconds(epochSeconds));

  EntryData data{
      .timestamp = timestamp,
      .name = j.at("name").get<std::string>(),
      .board = boardFromString(j.at("board").get<std::string>()),
      .grade = gradeFromString(j.at("grade").get<std::string>()),
      .attempts = j.at("attempts").get<uint32_t>(),
      .incline = j.at("incline").get<double>(),
      .sent = j.at("sent").get<bool>(),
  };

  const uint64_t id = j.at("id").get<uint64_t>();
  return {id, std::move(data)};
}

// ─── ClimbingLog ─────────────────────────────────────────────────────────────

ClimbingLog::ClimbingLog() { entries.reserve(512); }

ClimbingLog::~ClimbingLog() = default;

uint64_t ClimbingLog::addEntry(EntryData data) {
  const uint64_t id = nextID++;
  entries.emplace_back(Entry(id, std::move(data)));
  sortByTimestamp();
  return id;
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

void ClimbingLog::setEntry(const uint64_t entryID, EntryData data) {
  const auto it = findByID(entryID);
  if (it == entries.end())
    throw std::out_of_range("setEntry: entryID " + std::to_string(entryID) +
                            " not found");
  it->data = std::move(data);
  sortByTimestamp();
}

std::span<const Entry> ClimbingLog::getEntries(const size_t offset,
                                               const size_t count) const {
  if (offset >= entries.size()) return {};
  const size_t available = entries.size() - offset;
  return std::span(entries).subspan(offset, std::min(count, available));
}

// ─── Serialization ───────────────────────────────────────────────────────────

void ClimbingLog::serializeLog(const std::filesystem::path& path) const {
  json j;
  j["nextID"] = nextID;
  j["entries"] = json::array();
  for (const Entry& entry : entries) j["entries"].push_back(entryToJson(entry));

  std::ofstream file(path);
  if (!file.is_open())
    throw std::runtime_error("serializeLog: could not open " + path.string() +
                             " for writing");
  file << j.dump(2);
}

void ClimbingLog::deserializeLog(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("deserializeLog: could not open " + path.string() +
                             " for reading");

  const json j = json::parse(file);
  nextID = j.at("nextID").get<uint64_t>();

  entries.clear();
  for (const json& je : j.at("entries"))
    entries.emplace_back(entryFromJson(je));

  // Entries are stored sorted, but re-sort defensively on load.
  sortByTimestamp();
}

// ─── Private helpers ─────────────────────────────────────────────────────────

std::vector<Entry>::iterator ClimbingLog::findByID(uint64_t entryID) {
  return std::ranges::find_if(
      entries, [entryID](const Entry& e) { return e.entryID == entryID; });
}

std::vector<Entry>::const_iterator ClimbingLog::findByID(
    uint64_t entryID) const {
  return std::ranges::find_if(
      entries, [entryID](const Entry& e) { return e.entryID == entryID; });
}

void ClimbingLog::sortByTimestamp() {
  std::ranges::sort(entries, [](const Entry& a, const Entry& b) {
    return a.data.timestamp < b.data.timestamp;
  });
}

// ─── Printing ────────────────────────────────────────────────────────────────

std::ostream& operator<<(std::ostream& os, const Board board) {
  switch (board) {
    case MB2019:
      os << "MoonBoard 2019";
      break;
    case MB2024:
      os << "MoonBoard 2024";
      break;
    default:
      os << "N/A";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Grade grade) {
  switch (grade) {
    case V3:
      os << "6a+/V3";
      break;
    case V4:
      os << "6b/V4";
      break;
    case V5:
      os << "6c/V5";
      break;
    case V6:
      os << "7a/V6";
      break;
    case V7:
      os << "7a+/V7";
      break;
    default:
      os << "Ungraded";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Entry& entry) {
  const auto& [timestamp, name, board, grade, attempts, incline, sent] =
      entry.data;
  const auto tt = std::chrono::system_clock::to_time_t(timestamp);
  os << "\t" << std::ctime(&tt);
  os << "\t" << board << " at " << incline << " degrees\n";
  os << "\t" << name << " (" << grade << ")\n";

  if (sent && attempts == 1)
    os << "\tFlashed!\n";
  else if (sent)
    os << "\tSent in " << attempts << " attempts\n";
  else
    os << "\tProject, " << attempts << " attempts so far\n";

  return os;
}

std::ostream& operator<<(std::ostream& os, const ClimbingLog& log) {
  os << "Climbing Log (" << log.entries.size() << " entries)\n";
  for (size_t i = 0; i < log.entries.size(); i++) {
    os << "[" << i << "] (ID " << log.entries[i].getID() << "):\n";
    os << log.entries[i] << "\n";
  }
  return os;
}
