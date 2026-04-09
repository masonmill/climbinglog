#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <stdexcept>

#include "ClimbingLog.hpp"

// ─── Helpers ─────────────────────────────────────────────────────────────────

static std::chrono::system_clock::time_point makeTimestamp(
    const int64_t epochSeconds) {
  return std::chrono::system_clock::time_point(
      std::chrono::seconds(epochSeconds));
}

static ClimbData makeClimb(std::string name = "Test Problem",
                           Board board = MB2019, Grade grade = V3) {
  return ClimbData{
      .name = std::move(name),
      .board = board,
      .grade = grade,
  };
}

static SessionData makeSession(const int64_t epochSeconds,
                                const uint32_t attempts = 1,
                                const double incline = 40.0,
                                const bool sent = true) {
  return SessionData{
      .timestamp = makeTimestamp(epochSeconds),
      .attempts = attempts,
      .incline = incline,
      .sent = sent,
  };
}

// ─── addClimb ────────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, AddClimbReturnsSequentialIDs) {
  ClimbingLog log;
  const uint64_t id0 = log.addClimb(makeClimb("A"));
  const uint64_t id1 = log.addClimb(makeClimb("B"));
  const uint64_t id2 = log.addClimb(makeClimb("C"));

  EXPECT_EQ(id0, 0);
  EXPECT_EQ(id1, 1);
  EXPECT_EQ(id2, 2);
}

TEST(ClimbingLogTest, AddClimbIncrementsSize) {
  ClimbingLog log;
  EXPECT_EQ(log.size(), 0);
  log.addClimb(makeClimb("A"));
  EXPECT_EQ(log.size(), 1);
  log.addClimb(makeClimb("B"));
  EXPECT_EQ(log.size(), 2);
}

TEST(ClimbingLogTest, AddClimbSortsByName) {
  ClimbingLog log;
  log.addClimb(makeClimb("C"));
  log.addClimb(makeClimb("A"));
  log.addClimb(makeClimb("B"));

  const auto page = log.getClimbs(0, 3);
  EXPECT_EQ(page[0].getData().name, "A");
  EXPECT_EQ(page[1].getData().name, "B");
  EXPECT_EQ(page[2].getData().name, "C");
}

// ─── removeClimb ─────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, RemoveClimbDecreasesSize) {
  ClimbingLog log;
  const uint64_t id = log.addClimb(makeClimb());
  log.removeClimb(id);
  EXPECT_EQ(log.size(), 0);
}

TEST(ClimbingLogTest, RemoveClimbMakesItUnretrievable) {
  ClimbingLog log;
  const uint64_t id = log.addClimb(makeClimb());
  log.removeClimb(id);
  EXPECT_THROW(log.getClimb(id), std::out_of_range);
}

TEST(ClimbingLogTest, RemoveClimbThrowsOnMissingID) {
  ClimbingLog log;
  EXPECT_THROW(log.removeClimb(999), std::out_of_range);
}

TEST(ClimbingLogTest, RemoveClimbPreservesOtherClimbs) {
  ClimbingLog log;
  const uint64_t idA = log.addClimb(makeClimb("A"));
  const uint64_t idB = log.addClimb(makeClimb("B"));
  const uint64_t idC = log.addClimb(makeClimb("C"));

  log.removeClimb(idB);

  EXPECT_EQ(log.size(), 2);
  EXPECT_EQ(log.getClimb(idA).getData().name, "A");
  EXPECT_EQ(log.getClimb(idC).getData().name, "C");
}

// ─── getClimb ────────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, GetClimbReturnsCorrectData) {
  ClimbingLog log;
  const uint64_t id = log.addClimb(makeClimb("The Swarm", MB2019, V4));

  const Climb& climb = log.getClimb(id);
  const ClimbData& d = climb.getData();

  EXPECT_EQ(climb.getID(), id);
  EXPECT_EQ(d.name, "The Swarm");
  EXPECT_EQ(d.board, MB2019);
  EXPECT_EQ(d.grade, V4);
  EXPECT_EQ(climb.sessionCount(), 0);
}

TEST(ClimbingLogTest, GetClimbThrowsOnMissingID) {
  ClimbingLog log;
  EXPECT_THROW(log.getClimb(0), std::out_of_range);
}

// ─── setClimb ────────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, SetClimbUpdatesData) {
  ClimbingLog log;
  const uint64_t id = log.addClimb(makeClimb("Original", MB2019, V3));

  log.setClimb(id, makeClimb("Updated", MB2024, V5));

  const ClimbData& d = log.getClimb(id).getData();
  EXPECT_EQ(d.name, "Updated");
  EXPECT_EQ(d.board, MB2024);
  EXPECT_EQ(d.grade, V5);
}

TEST(ClimbingLogTest, SetClimbPreservesID) {
  ClimbingLog log;
  const uint64_t id = log.addClimb(makeClimb());
  log.setClimb(id, makeClimb("Renamed"));
  EXPECT_EQ(log.getClimb(id).getID(), id);
}

TEST(ClimbingLogTest, SetClimbReSortsByName) {
  ClimbingLog log;
  const uint64_t idA = log.addClimb(makeClimb("A"));
  const uint64_t idB = log.addClimb(makeClimb("B"));

  log.setClimb(idA, makeClimb("Z"));

  const auto page = log.getClimbs(0, 2);
  EXPECT_EQ(page[0].getID(), idB);
  EXPECT_EQ(page[1].getID(), idA);
}

TEST(ClimbingLogTest, SetClimbPreservesSessions) {
  ClimbingLog log;
  const uint64_t id = log.addClimb(makeClimb("Original"));
  log.addSession(id, makeSession(1000));

  log.setClimb(id, makeClimb("Renamed"));

  EXPECT_EQ(log.getClimb(id).sessionCount(), 1);
}

TEST(ClimbingLogTest, SetClimbThrowsOnMissingID) {
  ClimbingLog log;
  EXPECT_THROW(log.setClimb(999, makeClimb()), std::out_of_range);
}

// ─── getClimbs ───────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, GetClimbsReturnsRequestedPage) {
  ClimbingLog log;
  log.addClimb(makeClimb("A"));
  log.addClimb(makeClimb("B"));
  log.addClimb(makeClimb("C"));
  log.addClimb(makeClimb("D"));
  log.addClimb(makeClimb("E"));

  const auto page = log.getClimbs(1, 3);
  ASSERT_EQ(page.size(), 3);
  EXPECT_EQ(page[0].getData().name, "B");
  EXPECT_EQ(page[1].getData().name, "C");
  EXPECT_EQ(page[2].getData().name, "D");
}

TEST(ClimbingLogTest, GetClimbsClampsToBounds) {
  ClimbingLog log;
  log.addClimb(makeClimb("A"));
  log.addClimb(makeClimb("B"));

  const auto page = log.getClimbs(0, 100);
  EXPECT_EQ(page.size(), 2);
}

TEST(ClimbingLogTest, GetClimbsReturnsEmptyForOutOfBoundsOffset) {
  ClimbingLog log;
  log.addClimb(makeClimb());

  const auto page = log.getClimbs(999, 10);
  EXPECT_TRUE(page.empty());
}

// ─── addSession ──────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, AddSessionReturnsSequentialIDs) {
  ClimbingLog log;
  const uint64_t climbID = log.addClimb(makeClimb());
  const uint64_t s0 = log.addSession(climbID, makeSession(1000));
  const uint64_t s1 = log.addSession(climbID, makeSession(2000));

  EXPECT_EQ(s0, 0);
  EXPECT_EQ(s1, 1);
}

TEST(ClimbingLogTest, AddSessionIncrementsSessionCount) {
  ClimbingLog log;
  const uint64_t climbID = log.addClimb(makeClimb());
  EXPECT_EQ(log.getClimb(climbID).sessionCount(), 0);
  log.addSession(climbID, makeSession(1000));
  EXPECT_EQ(log.getClimb(climbID).sessionCount(), 1);
}

TEST(ClimbingLogTest, AddSessionSortsByTimestamp) {
  ClimbingLog log;
  const uint64_t climbID = log.addClimb(makeClimb());
  log.addSession(climbID, makeSession(3000, 3));
  log.addSession(climbID, makeSession(1000, 1));
  log.addSession(climbID, makeSession(2000, 2));

  const auto& sessions = log.getClimb(climbID).getSessions();
  EXPECT_EQ(sessions[0].getData().attempts, 1);
  EXPECT_EQ(sessions[1].getData().attempts, 2);
  EXPECT_EQ(sessions[2].getData().attempts, 3);
}

TEST(ClimbingLogTest, AddSessionThrowsOnMissingClimb) {
  ClimbingLog log;
  EXPECT_THROW(log.addSession(999, makeSession(1000)), std::out_of_range);
}

// ─── removeSession ───────────────────────────────────────────────────────────

TEST(ClimbingLogTest, RemoveSessionDecreasesCount) {
  ClimbingLog log;
  const uint64_t cid = log.addClimb(makeClimb());
  const uint64_t sid = log.addSession(cid, makeSession(1000));
  log.removeSession(cid, sid);
  EXPECT_EQ(log.getClimb(cid).sessionCount(), 0);
}

TEST(ClimbingLogTest, RemoveSessionThrowsOnMissingSession) {
  ClimbingLog log;
  const uint64_t cid = log.addClimb(makeClimb());
  EXPECT_THROW(log.removeSession(cid, 999), std::out_of_range);
}

TEST(ClimbingLogTest, RemoveSessionThrowsOnMissingClimb) {
  ClimbingLog log;
  EXPECT_THROW(log.removeSession(999, 0), std::out_of_range);
}

// ─── getSession ──────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, GetSessionReturnsCorrectData) {
  ClimbingLog log;
  const uint64_t cid = log.addClimb(makeClimb());
  const uint64_t sid = log.addSession(cid, makeSession(1000, 5, 45.0, false));

  const Session& s = log.getSession(cid, sid);
  const SessionData& d = s.getData();

  EXPECT_EQ(s.getID(), sid);
  EXPECT_EQ(d.attempts, 5);
  EXPECT_DOUBLE_EQ(d.incline, 45.0);
  EXPECT_FALSE(d.sent);
  EXPECT_EQ(d.timestamp, makeTimestamp(1000));
}

// ─── setSession ──────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, SetSessionUpdatesData) {
  ClimbingLog log;
  const uint64_t cid = log.addClimb(makeClimb());
  const uint64_t sid = log.addSession(cid, makeSession(1000, 1, 40.0, false));

  log.setSession(cid, sid, makeSession(1000, 3, 45.0, true));

  const SessionData& d = log.getSession(cid, sid).getData();
  EXPECT_EQ(d.attempts, 3);
  EXPECT_DOUBLE_EQ(d.incline, 45.0);
  EXPECT_TRUE(d.sent);
}

TEST(ClimbingLogTest, SetSessionReSortsByTimestamp) {
  ClimbingLog log;
  const uint64_t cid = log.addClimb(makeClimb());
  const uint64_t sA = log.addSession(cid, makeSession(1000, 1));
  const uint64_t sB = log.addSession(cid, makeSession(2000, 2));

  // Move A to after B.
  log.setSession(cid, sA, makeSession(3000, 1));

  const auto& sessions = log.getClimb(cid).getSessions();
  EXPECT_EQ(sessions[0].getID(), sB);
  EXPECT_EQ(sessions[1].getID(), sA);
}

TEST(ClimbingLogTest, SetSessionThrowsOnMissingSession) {
  ClimbingLog log;
  const uint64_t cid = log.addClimb(makeClimb());
  EXPECT_THROW(log.setSession(cid, 999, makeSession(1000)), std::out_of_range);
}

// ─── Serialization ───────────────────────────────────────────────────────────

class SerializationTest : public ::testing::Test {
 protected:
  std::filesystem::path tmpPath =
      std::filesystem::temp_directory_path() / "climbinglog_test.json";

  void TearDown() override { std::filesystem::remove(tmpPath); }
};

TEST_F(SerializationTest, RoundTripPreservesAllFields) {
  ClimbingLog log;
  const uint64_t cid = log.addClimb(makeClimb("The Process", MB2019, V3));
  log.addSession(cid, makeSession(1000, 3, 40.0, true));
  log.addSession(cid, makeSession(2000, 1, 45.0, false));

  const uint64_t cid2 = log.addClimb(makeClimb("Beltway", MB2024, V4));
  log.addSession(cid2, makeSession(3000, 2, 40.0, true));
  log.serializeLog(tmpPath);

  ClimbingLog loaded;
  loaded.deserializeLog(tmpPath);

  ASSERT_EQ(loaded.size(), 2);

  // Climbs are sorted by name: Beltway, The Process
  const auto climbs = loaded.getClimbs(0, 2);

  const ClimbData& d0 = climbs[0].getData();
  EXPECT_EQ(d0.name, "Beltway");
  EXPECT_EQ(d0.board, MB2024);
  EXPECT_EQ(d0.grade, V4);
  EXPECT_EQ(climbs[0].sessionCount(), 1);

  const ClimbData& d1 = climbs[1].getData();
  EXPECT_EQ(d1.name, "The Process");
  EXPECT_EQ(d1.board, MB2019);
  EXPECT_EQ(d1.grade, V3);
  ASSERT_EQ(climbs[1].sessionCount(), 2);

  const SessionData& s0 = climbs[1].getSessions()[0].getData();
  EXPECT_EQ(s0.attempts, 3);
  EXPECT_DOUBLE_EQ(s0.incline, 40.0);
  EXPECT_TRUE(s0.sent);
  EXPECT_EQ(s0.timestamp, makeTimestamp(1000));
}

TEST_F(SerializationTest, RoundTripPreservesNextClimbID) {
  ClimbingLog log;
  log.addClimb(makeClimb("A"));
  log.addClimb(makeClimb("B"));
  log.serializeLog(tmpPath);

  ClimbingLog loaded;
  loaded.deserializeLog(tmpPath);

  const uint64_t newID = loaded.addClimb(makeClimb("C"));
  EXPECT_EQ(newID, 2);
}

TEST_F(SerializationTest, RoundTripPreservesNextSessionID) {
  ClimbingLog log;
  const uint64_t cid = log.addClimb(makeClimb());
  log.addSession(cid, makeSession(1000));
  log.addSession(cid, makeSession(2000));
  log.serializeLog(tmpPath);

  ClimbingLog loaded;
  loaded.deserializeLog(tmpPath);

  // Find the climb (it'll have the same ID)
  const uint64_t newSID = loaded.addSession(cid, makeSession(3000));
  EXPECT_EQ(newSID, 2);
}

TEST_F(SerializationTest, RoundTripPreservesSortOrder) {
  ClimbingLog log;
  log.addClimb(makeClimb("C"));
  log.addClimb(makeClimb("A"));
  log.addClimb(makeClimb("B"));
  log.serializeLog(tmpPath);

  ClimbingLog loaded;
  loaded.deserializeLog(tmpPath);

  const auto page = loaded.getClimbs(0, 3);
  EXPECT_EQ(page[0].getData().name, "A");
  EXPECT_EQ(page[1].getData().name, "B");
  EXPECT_EQ(page[2].getData().name, "C");
}

TEST_F(SerializationTest, DeserializeThrowsOnMissingFile) {
  ClimbingLog log;
  EXPECT_THROW(log.deserializeLog("/nonexistent/path/log.json"),
               std::runtime_error);
}
