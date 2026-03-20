#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <stdexcept>

#include "ClimbingLog.hpp"

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Builds a time_point from a Unix epoch second offset for readable test data.
static std::chrono::system_clock::time_point makeTimestamp(
    const int64_t epochSeconds) {
  return std::chrono::system_clock::time_point(
      std::chrono::seconds(epochSeconds));
}

static EntryData makeEntry(const int64_t epochSeconds,
                           std::string name = "Test Problem",
                           Board board = MB2019, const Grade grade = V3,
                           const uint32_t attempts = 1,
                           const double incline = 40.0,
                           const bool sent = true) {
  return EntryData{
      .timestamp = makeTimestamp(epochSeconds),
      .name = std::move(name),
      .board = board,
      .grade = grade,
      .attempts = attempts,
      .incline = incline,
      .sent = sent,
  };
}

// ─── addEntry ────────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, AddEntryReturnsSequentialIDs) {
  ClimbingLog log;
  const uint64_t id0 = log.addEntry(makeEntry(1000));
  const uint64_t id1 = log.addEntry(makeEntry(2000));
  const uint64_t id2 = log.addEntry(makeEntry(3000));

  EXPECT_EQ(id0, 0);
  EXPECT_EQ(id1, 1);
  EXPECT_EQ(id2, 2);
}

TEST(ClimbingLogTest, AddEntryIncrementsSize) {
  ClimbingLog log;
  EXPECT_EQ(log.size(), 0);
  log.addEntry(makeEntry(1000));
  EXPECT_EQ(log.size(), 1);
  log.addEntry(makeEntry(2000));
  EXPECT_EQ(log.size(), 2);
}

TEST(ClimbingLogTest, AddEntrySortsByTimestamp) {
  ClimbingLog log;
  log.addEntry(makeEntry(3000, "C"));
  log.addEntry(makeEntry(1000, "A"));
  log.addEntry(makeEntry(2000, "B"));

  const auto page = log.getEntries(0, 3);
  EXPECT_EQ(page[0].getData().name, "A");
  EXPECT_EQ(page[1].getData().name, "B");
  EXPECT_EQ(page[2].getData().name, "C");
}

// ─── removeEntry ─────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, RemoveEntryDecreasesSize) {
  ClimbingLog log;
  const uint64_t id = log.addEntry(makeEntry(1000));
  log.removeEntry(id);
  EXPECT_EQ(log.size(), 0);
}

TEST(ClimbingLogTest, RemoveEntryMakesItUnretrievable) {
  ClimbingLog log;
  const uint64_t id = log.addEntry(makeEntry(1000));
  log.removeEntry(id);
  EXPECT_THROW(log.getEntry(id), std::out_of_range);
}

TEST(ClimbingLogTest, RemoveEntryThrowsOnMissingID) {
  ClimbingLog log;
  EXPECT_THROW(log.removeEntry(999), std::out_of_range);
}

TEST(ClimbingLogTest, RemoveEntryPreservesOtherEntries) {
  ClimbingLog log;
  const uint64_t id0 = log.addEntry(makeEntry(1000, "A"));
  const uint64_t id1 = log.addEntry(makeEntry(2000, "B"));
  const uint64_t id2 = log.addEntry(makeEntry(3000, "C"));

  log.removeEntry(id1);

  EXPECT_EQ(log.size(), 2);
  EXPECT_EQ(log.getEntry(id0).getData().name, "A");
  EXPECT_EQ(log.getEntry(id2).getData().name, "C");
}

// ─── getEntry ────────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, GetEntryReturnsCorrectData) {
  ClimbingLog log;
  const uint64_t id =
      log.addEntry(makeEntry(1000, "The Swarm", MB2019, V4, 5, 40.0, true));

  const Entry& entry = log.getEntry(id);
  const EntryData& d = entry.getData();

  EXPECT_EQ(entry.getID(), id);
  EXPECT_EQ(d.name, "The Swarm");
  EXPECT_EQ(d.board, MB2019);
  EXPECT_EQ(d.grade, V4);
  EXPECT_EQ(d.attempts, 5);
  EXPECT_DOUBLE_EQ(d.incline, 40.0);
  EXPECT_TRUE(d.sent);
  EXPECT_EQ(d.timestamp, makeTimestamp(1000));
}

TEST(ClimbingLogTest, GetEntryThrowsOnMissingID) {
  ClimbingLog log;
  EXPECT_THROW(log.getEntry(0), std::out_of_range);
}

// ─── setEntry ────────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, SetEntryUpdatesData) {
  ClimbingLog log;
  const uint64_t id =
      log.addEntry(makeEntry(1000, "Original", MB2019, V3, 1, 40.0, false));

  log.setEntry(id, makeEntry(1000, "Updated", MB2019, V4, 3, 40.0, true));

  const EntryData& d = log.getEntry(id).getData();
  EXPECT_EQ(d.name, "Updated");
  EXPECT_EQ(d.grade, V4);
  EXPECT_EQ(d.attempts, 3);
  EXPECT_TRUE(d.sent);
}

TEST(ClimbingLogTest, SetEntryPreservesID) {
  ClimbingLog log;
  const uint64_t id = log.addEntry(makeEntry(1000));

  log.setEntry(id, makeEntry(2000, "Renamed"));

  EXPECT_EQ(log.getEntry(id).getID(), id);
}

TEST(ClimbingLogTest, SetEntryReSortsByTimestamp) {
  ClimbingLog log;
  const uint64_t idA = log.addEntry(makeEntry(1000, "A"));
  const uint64_t idB = log.addEntry(makeEntry(2000, "B"));

  // Move A to after B.
  log.setEntry(idA, makeEntry(3000, "A-moved"));

  const auto page = log.getEntries(0, 2);
  EXPECT_EQ(page[0].getID(), idB);
  EXPECT_EQ(page[1].getID(), idA);
}

TEST(ClimbingLogTest, SetEntryThrowsOnMissingID) {
  ClimbingLog log;
  EXPECT_THROW(log.setEntry(999, makeEntry(1000)), std::out_of_range);
}

// ─── getEntries ──────────────────────────────────────────────────────────────

TEST(ClimbingLogTest, GetEntriesReturnsRequestedPage) {
  ClimbingLog log;
  for (int i = 0; i < 5; i++)
    log.addEntry(makeEntry(i * 1000, "Entry " + std::to_string(i)));

  const auto page = log.getEntries(1, 3);

  ASSERT_EQ(page.size(), 3);
  EXPECT_EQ(page[0].getData().name, "Entry 1");
  EXPECT_EQ(page[1].getData().name, "Entry 2");
  EXPECT_EQ(page[2].getData().name, "Entry 3");
}

TEST(ClimbingLogTest, GetEntriesClampsToBounds) {
  ClimbingLog log;
  log.addEntry(makeEntry(1000));
  log.addEntry(makeEntry(2000));

  // Requesting more than available should return only what exists.
  const auto page = log.getEntries(0, 100);
  EXPECT_EQ(page.size(), 2);
}

TEST(ClimbingLogTest, GetEntriesReturnsEmptyForOutOfBoundsOffset) {
  ClimbingLog log;
  log.addEntry(makeEntry(1000));

  const auto page = log.getEntries(999, 10);
  EXPECT_TRUE(page.empty());
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
  log.addEntry(makeEntry(1000, "The Process", MB2019, V3, 3, 40.0, true));
  log.addEntry(makeEntry(2000, "Beltway", MB2019, V4, 1, 45.0, false));
  log.serializeLog(tmpPath);

  ClimbingLog loaded;
  loaded.deserializeLog(tmpPath);

  ASSERT_EQ(loaded.size(), 2);

  const EntryData& d0 = loaded.getEntries(0, 2)[0].getData();
  EXPECT_EQ(d0.name, "The Process");
  EXPECT_EQ(d0.board, MB2019);
  EXPECT_EQ(d0.grade, V3);
  EXPECT_EQ(d0.attempts, 3);
  EXPECT_DOUBLE_EQ(d0.incline, 40.0);
  EXPECT_TRUE(d0.sent);
  EXPECT_EQ(d0.timestamp, makeTimestamp(1000));
}

TEST_F(SerializationTest, RoundTripPreservesNextID) {
  ClimbingLog log;
  log.addEntry(makeEntry(1000));
  log.addEntry(makeEntry(2000));
  log.serializeLog(tmpPath);

  ClimbingLog loaded;
  loaded.deserializeLog(tmpPath);

  // The next entry added to the loaded log should not reuse an existing ID.
  const uint64_t newID = loaded.addEntry(makeEntry(3000));
  EXPECT_EQ(newID, 2);
}

TEST_F(SerializationTest, RoundTripPreservesSortOrder) {
  ClimbingLog log;
  log.addEntry(makeEntry(3000, "C"));
  log.addEntry(makeEntry(1000, "A"));
  log.addEntry(makeEntry(2000, "B"));
  log.serializeLog(tmpPath);

  ClimbingLog loaded;
  loaded.deserializeLog(tmpPath);

  const auto page = loaded.getEntries(0, 3);
  EXPECT_EQ(page[0].getData().name, "A");
  EXPECT_EQ(page[1].getData().name, "B");
  EXPECT_EQ(page[2].getData().name, "C");
}

TEST_F(SerializationTest, DeserializeThrowsOnMissingFile) {
  ClimbingLog log;
  EXPECT_THROW(log.deserializeLog("/nonexistent/path/log.json"),
               std::runtime_error);
}