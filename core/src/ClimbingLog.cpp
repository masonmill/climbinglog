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

static json sessionToJson(const Session& session) {
  const auto& d = session.getData();
  const int64_t epochSeconds =
      std::chrono::duration_cast<std::chrono::seconds>(
          d.timestamp.time_since_epoch())
          .count();
  return {
      {"id", session.getID()},
      {"timestamp", epochSeconds},
      {"attempts", d.attempts},
      {"incline", d.incline},
      {"sent", d.sent},
  };
}

Session sessionFromJson(const json& j) {
  const int64_t epochSeconds = j.at("timestamp").get<int64_t>();
  SessionData data{
      .timestamp = std::chrono::system_clock::time_point(
          std::chrono::seconds(epochSeconds)),
      .attempts = j.at("attempts").get<uint32_t>(),
      .incline = j.at("incline").get<double>(),
      .sent = j.at("sent").get<bool>(),
  };
  return {j.at("id").get<uint64_t>(), std::move(data)};
}

json climbToJson(const Climb& climb) {
  const auto& d = climb.getData();
  json sessions = json::array();
  for (const auto& s : climb.getSessions()) sessions.push_back(sessionToJson(s));

  return {
      {"id", climb.getID()},
      {"name", d.name},
      {"board", boardToString(d.board)},
      {"grade", gradeToString(d.grade)},
      {"nextSessionID", climb.nextSessionID},
      {"sessions", sessions},
  };
}

// static
Climb ClimbingLog::climbFromJson(const json& j) {
  ClimbData data{
      .name = j.at("name").get<std::string>(),
      .board = boardFromString(j.at("board").get<std::string>()),
      .grade = gradeFromString(j.at("grade").get<std::string>()),
  };
  Climb climb(j.at("id").get<uint64_t>(), std::move(data));
  climb.nextSessionID = j.at("nextSessionID").get<uint64_t>();
  for (const auto& js : j.at("sessions"))
    climb.sessions.emplace_back(sessionFromJson(js));
  climb.sortSessionsByTimestamp();
  return climb;
}

// ─── Climb ───────────────────────────────────────────────────────────────────

uint64_t Climb::addSession(SessionData sessionData) {
  const uint64_t id = nextSessionID++;
  sessions.emplace_back(Session(id, std::move(sessionData)));
  sortSessionsByTimestamp();
  return id;
}

void Climb::removeSession(const uint64_t sessionID) {
  const auto it = findSessionByID(sessionID);
  if (it == sessions.end())
    throw std::out_of_range("removeSession: sessionID " +
                            std::to_string(sessionID) + " not found");
  sessions.erase(it);
}

Session& Climb::getSession(const uint64_t sessionID) {
  const auto it = findSessionByID(sessionID);
  if (it == sessions.end())
    throw std::out_of_range("getSession: sessionID " +
                            std::to_string(sessionID) + " not found");
  return *it;
}

const Session& Climb::getSession(const uint64_t sessionID) const {
  const auto it = findSessionByID(sessionID);
  if (it == sessions.end())
    throw std::out_of_range("getSession: sessionID " +
                            std::to_string(sessionID) + " not found");
  return *it;
}

void Climb::setSession(const uint64_t sessionID, SessionData sessionData) {
  const auto it = findSessionByID(sessionID);
  if (it == sessions.end())
    throw std::out_of_range("setSession: sessionID " +
                            std::to_string(sessionID) + " not found");
  it->data = std::move(sessionData);
  sortSessionsByTimestamp();
}

void Climb::sortSessionsByTimestamp() {
  std::ranges::sort(sessions, [](const Session& a, const Session& b) {
    return a.data.timestamp < b.data.timestamp;
  });
}

std::vector<Session>::iterator Climb::findSessionByID(
    const uint64_t sessionID) {
  return std::ranges::find_if(sessions, [sessionID](const Session& s) {
    return s.sessionID == sessionID;
  });
}

std::vector<Session>::const_iterator Climb::findSessionByID(
    const uint64_t sessionID) const {
  return std::ranges::find_if(sessions, [sessionID](const Session& s) {
    return s.sessionID == sessionID;
  });
}

// ─── ClimbingLog ─────────────────────────────────────────────────────────────

ClimbingLog::ClimbingLog() { climbs.reserve(128); }

ClimbingLog::~ClimbingLog() = default;

// ── Climb CRUD ──────────────────────────────────────────────────────────────

uint64_t ClimbingLog::addClimb(ClimbData data) {
  const uint64_t id = nextClimbID++;
  climbs.emplace_back(Climb(id, std::move(data)));
  sortByName();
  return id;
}

void ClimbingLog::removeClimb(const uint64_t climbID) {
  const auto it = findByID(climbID);
  if (it == climbs.end())
    throw std::out_of_range("removeClimb: climbID " +
                            std::to_string(climbID) + " not found");
  climbs.erase(it);
}

Climb& ClimbingLog::getClimb(const uint64_t climbID) {
  const auto it = findByID(climbID);
  if (it == climbs.end())
    throw std::out_of_range("getClimb: climbID " + std::to_string(climbID) +
                            " not found");
  return *it;
}

const Climb& ClimbingLog::getClimb(const uint64_t climbID) const {
  const auto it = findByID(climbID);
  if (it == climbs.end())
    throw std::out_of_range("getClimb: climbID " + std::to_string(climbID) +
                            " not found");
  return *it;
}

void ClimbingLog::setClimb(const uint64_t climbID, ClimbData data) {
  const auto it = findByID(climbID);
  if (it == climbs.end())
    throw std::out_of_range("setClimb: climbID " + std::to_string(climbID) +
                            " not found");
  it->data = std::move(data);
  sortByName();
}

std::span<const Climb> ClimbingLog::getClimbs(const size_t offset,
                                               const size_t count) const {
  if (offset >= climbs.size()) return {};
  const size_t available = climbs.size() - offset;
  return std::span(climbs).subspan(offset, std::min(count, available));
}

// ── Session CRUD ────────────────────────────────────────────────────────────

uint64_t ClimbingLog::addSession(const uint64_t climbID, SessionData data) {
  return getClimb(climbID).addSession(std::move(data));
}

void ClimbingLog::removeSession(const uint64_t climbID,
                                 const uint64_t sessionID) {
  getClimb(climbID).removeSession(sessionID);
}

Session& ClimbingLog::getSession(const uint64_t climbID,
                                  const uint64_t sessionID) {
  return getClimb(climbID).getSession(sessionID);
}

const Session& ClimbingLog::getSession(const uint64_t climbID,
                                        const uint64_t sessionID) const {
  return getClimb(climbID).getSession(sessionID);
}

void ClimbingLog::setSession(const uint64_t climbID, const uint64_t sessionID,
                              SessionData data) {
  getClimb(climbID).setSession(sessionID, std::move(data));
}

// ─── Serialization ───────────────────────────────────────────────────────────

void ClimbingLog::serializeLog(const std::filesystem::path& path) const {
  json j;
  j["nextClimbID"] = nextClimbID;
  j["climbs"] = json::array();
  for (const auto& climb : climbs) j["climbs"].push_back(climbToJson(climb));

  std::ofstream file(path);
  if (!file.is_open())
    throw std::runtime_error("serializeLog: could not open " + path.string() +
                             " for writing");
  file << j.dump(2);
}

void ClimbingLog::deserializeLog(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("deserializeLog: could not open " +
                             path.string() + " for reading");

  const json j = json::parse(file);
  nextClimbID = j.at("nextClimbID").get<uint64_t>();

  climbs.clear();
  for (const auto& jc : j.at("climbs"))
    climbs.emplace_back(climbFromJson(jc));

  sortByName();
}

// ─── Private helpers ─────────────────────────────────────────────────────────

std::vector<Climb>::iterator ClimbingLog::findByID(const uint64_t climbID) {
  return std::ranges::find_if(
      climbs, [climbID](const Climb& c) { return c.climbID == climbID; });
}

std::vector<Climb>::const_iterator ClimbingLog::findByID(
    const uint64_t climbID) const {
  return std::ranges::find_if(
      climbs, [climbID](const Climb& c) { return c.climbID == climbID; });
}

void ClimbingLog::sortByName() {
  std::ranges::sort(climbs, [](const Climb& a, const Climb& b) {
    return a.data.name < b.data.name;
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

std::ostream& operator<<(std::ostream& os, const Climb& climb) {
  const auto& d = climb.data;
  os << d.name << " (" << d.grade << ") — " << d.board << "\n";
  os << climb.sessions.size() << " session(s):\n";
  for (const auto& s : climb.sessions) {
    const auto& sd = s.getData();
    const auto tt = std::chrono::system_clock::to_time_t(sd.timestamp);
    os << "\t" << std::ctime(&tt);
    os << "\t" << sd.incline << " degrees, " << sd.attempts << " attempt(s)";
    os << (sd.sent ? " — Sent" : " — Project") << "\n";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const ClimbingLog& log) {
  os << "Climbing Log (" << log.climbs.size() << " climbs)\n";
  for (size_t i = 0; i < log.climbs.size(); i++) {
    os << "[" << i << "] (ID " << log.climbs[i].getID() << "):\n";
    os << log.climbs[i] << "\n";
  }
  return os;
}
