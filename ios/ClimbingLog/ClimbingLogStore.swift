//
//  ClimbingLogStore.swift
//  ClimbingLog
//

import Foundation
import Observation

// ─── SyncState ────────────────────────────────────────────────────────────────

enum SyncState: Equatable {
    case idle
    case syncing
    case synced(Date)
    case failed(String)
}

// ─── LogHandle ────────────────────────────────────────────────────────────────

/// Wraps the C opaque pointer so its deinit can call cl_log_destroy without
/// needing to escape @MainActor isolation. The store creates it once and never
/// replaces it, so all access through .ptr is effectively single-threaded.
private final class LogHandle {
    let ptr: OpaquePointer
    init() { ptr = cl_log_create() }
    deinit { cl_log_destroy(ptr) }
}

// ─── Store ────────────────────────────────────────────────────────────────────

@Observable
@MainActor
class ClimbingLogStore {
    private(set) var climbs: [ClimbViewModel] = []
    /// Sessions flattened across all climbs, sorted newest-first and grouped by calendar day.
    private(set) var sessionsByDate: [(dateString: String, rows: [SessionRow])] = []
    private(set) var isDirty = false
    private(set) var syncState: SyncState = .idle
    private let handle = LogHandle()

    static var logFileURL: URL {
        FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("climbing_log.json")
    }

    init() {
        let path = Self.logFileURL.path(percentEncoded: false)
        if FileManager.default.fileExists(atPath: path) {
            cl_log_deserialize(handle.ptr, path)
        }
        refreshClimbs()
    }

    func serialize() {
        cl_log_serialize(handle.ptr, Self.logFileURL.path(percentEncoded: false))
    }

    // ─── Climb CRUD ───────────────────────────────────────────────────────

    @discardableResult
    func addClimb(_ data: CLClimbData) -> UInt64 {
        let id = cl_log_add_climb(handle.ptr, data)
        refreshClimbs()
        serialize()
        isDirty = true
        return id
    }

    func updateClimb(id: UInt64, data: CLClimbData) {
        cl_log_set_climb(handle.ptr, id, data)
        refreshClimbs()
        serialize()
        isDirty = true
    }

    func removeClimb(id: UInt64) {
        cl_log_remove_climb(handle.ptr, id)
        refreshClimbs()
        serialize()
        isDirty = true
    }

    // ─── Session CRUD ─────────────────────────────────────────────────────

    @discardableResult
    func addSession(climbID: UInt64, data: CLSessionData) -> UInt64 {
        let id = cl_log_add_session(handle.ptr, climbID, data)
        refreshClimbs()
        serialize()
        isDirty = true
        return id
    }

    func updateSession(climbID: UInt64, sessionID: UInt64, data: CLSessionData) {
        cl_log_set_session(handle.ptr, climbID, sessionID, data)
        refreshClimbs()
        serialize()
        isDirty = true
    }

    func removeSession(climbID: UInt64, sessionID: UInt64) {
        cl_log_remove_session(handle.ptr, climbID, sessionID)
        refreshClimbs()
        serialize()
        isDirty = true
    }

    func sessions(for climbID: UInt64) -> [SessionViewModel] {
        let count = cl_log_session_count(handle.ptr, climbID)
        guard count > 0 else { return [] }
        let buffer = UnsafeMutablePointer<CLSession>.allocate(capacity: count)
        defer { buffer.deallocate() }
        let fetched = cl_log_get_sessions(handle.ptr, climbID, 0, count, buffer)
        // Reverse so newest sessions appear first.
        return (0..<fetched).map { SessionViewModel(from: buffer[$0], climbID: climbID) }.reversed()
    }

    /// All existing climb names, useful for autocomplete.
    var climbNames: [String] {
        climbs.map(\.name)
    }

    /// Finds an existing climb matching the given name and board.
    func existingClimb(name: String, board: Board) -> ClimbViewModel? {
        climbs.first { $0.name == name && $0.board == board }
    }

    /// Returns the climb view model for the given id, if it exists.
    func climb(id: UInt64) -> ClimbViewModel? {
        climbs.first { $0.id == id }
    }

    // ─── Sync ─────────────────────────────────────────────────────────────

    /// Downloads the log from GitHub and replaces local state.
    /// `force: true` skips the dirty-guard (for explicit user-initiated pulls where
    /// overwriting unpushed local edits is the intent).
    func pullFromRemote(force: Bool = false) async {
        guard GitHubConfig.isConfigured, let token = GitHubConfig.pat else { return }
        // Refuse to silently stomp on local changes that haven't been pushed yet.
        if isDirty && !force { return }
        syncState = .syncing
        do {
            let data = try await GitHubSyncService.shared.pull(
                owner: GitHubConfig.owner, repo: GitHubConfig.repo, token: token
            )
            try data.write(to: Self.logFileURL)
            cl_log_deserialize(handle.ptr, Self.logFileURL.path(percentEncoded: false))
            refreshClimbs()
            isDirty = false
            syncState = .synced(Date())
        } catch {
            syncState = .failed(error.localizedDescription)
        }
    }

    /// Syncs to GitHub only when there are unsaved changes.
    func syncIfNeeded() async {
        guard isDirty else { return }
        await sync()
    }

    /// Unconditionally syncs the current log to GitHub.
    func sync() async {
        guard GitHubConfig.isConfigured, let token = GitHubConfig.pat else { return }
        let owner = GitHubConfig.owner
        let repo  = GitHubConfig.repo
        syncState = .syncing
        do {
            try await GitHubSyncService.shared.sync(
                fileURL: Self.logFileURL,
                owner: owner,
                repo: repo,
                token: token
            )
            isDirty = false
            syncState = .synced(Date())
        } catch {
            syncState = .failed(error.localizedDescription)
        }
    }

    // ─── Private ──────────────────────────────────────────────────────────

    private func refreshClimbs() {
        let count = cl_log_climb_count(handle.ptr)
        guard count > 0 else {
            climbs = []
            sessionsByDate = []
            return
        }
        let buffer = UnsafeMutablePointer<CLClimb>.allocate(capacity: count)
        defer { buffer.deallocate() }
        let fetched = cl_log_get_climbs(handle.ptr, 0, count, buffer)

        var newClimbs: [ClimbViewModel] = []
        var allRows: [SessionRow] = []

        for i in 0..<fetched {
            let raw = buffer[i]
            let climbID = raw.id
            let board = Board(raw.data.board)
            let grade = Grade(raw.data.grade)
            let name = withUnsafeBytes(of: raw.data.name) { bytes in
                String(cString: bytes.baseAddress!.assumingMemoryBound(to: CChar.self))
            }

            let sCount = cl_log_session_count(handle.ptr, climbID)
            var everSent = false

            if sCount > 0 {
                let sBuf = UnsafeMutablePointer<CLSession>.allocate(capacity: sCount)
                defer { sBuf.deallocate() }
                let sFetched = cl_log_get_sessions(handle.ptr, climbID, 0, sCount, sBuf)

                // Sort chronologically (oldest first) so send labels are correct.
                let chronological = (0..<sFetched)
                    .map { j in SessionViewModel(from: sBuf[j], climbID: climbID) }
                    .sorted { $0.date < $1.date }

                everSent = chronological.contains(where: \.sent)

                for (idx, session) in chronological.enumerated() {
                    let label = computeSessionLabel(
                        sent: session.sent,
                        attempts: session.attempts,
                        sessionIndex: idx,
                        allChronologicalSessions: chronological
                    )
                    allRows.append(SessionRow(
                        climbID: climbID,
                        sessionID: session.id,
                        climbName: name,
                        board: board,
                        grade: grade,
                        date: session.date,
                        attempts: session.attempts,
                        incline: session.incline,
                        sent: session.sent,
                        label: label,
                        sessionCountForClimb: sFetched
                    ))
                }
            }

            newClimbs.append(ClimbViewModel(from: raw, sessionCount: sCount, everSent: everSent))
        }

        climbs = newClimbs

        // Sort all rows newest-first, then bucket by calendar day.
        allRows.sort { $0.date > $1.date }
        sessionsByDate = groupByDate(allRows)
    }

    private func groupByDate(_ rows: [SessionRow]) -> [(dateString: String, rows: [SessionRow])] {
        var groups: [(dateString: String, rows: [SessionRow])] = []
        var bucket: [SessionRow] = []
        var bucketDay: Date? = nil
        let cal = Calendar.current

        for row in rows {
            let day = cal.startOfDay(for: row.date)
            if let current = bucketDay, cal.isDate(day, inSameDayAs: current) {
                bucket.append(row)
            } else {
                if !bucket.isEmpty, let current = bucketDay {
                    let label = current.formatted(date: .abbreviated, time: .omitted)
                    groups.append((dateString: label, rows: bucket))
                }
                bucketDay = day
                bucket = [row]
            }
        }
        if !bucket.isEmpty, let current = bucketDay {
            let label = current.formatted(date: .abbreviated, time: .omitted)
            groups.append((dateString: label, rows: bucket))
        }
        return groups
    }
}
