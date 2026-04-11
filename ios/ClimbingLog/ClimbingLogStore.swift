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

    // ─── Sync ─────────────────────────────────────────────────────────────

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
            return
        }
        let buffer = UnsafeMutablePointer<CLClimb>.allocate(capacity: count)
        defer { buffer.deallocate() }
        let fetched = cl_log_get_climbs(handle.ptr, 0, count, buffer)
        climbs = (0..<fetched).map { i in
            let climbID = buffer[i].id
            let sCount = cl_log_session_count(handle.ptr, climbID)
            var everSent = false
            if sCount > 0 {
                let sBuf = UnsafeMutablePointer<CLSession>.allocate(capacity: sCount)
                defer { sBuf.deallocate() }
                let sFetched = cl_log_get_sessions(handle.ptr, climbID, 0, sCount, sBuf)
                everSent = (0..<sFetched).contains { sBuf[$0].data.sent }
            }
            return ClimbViewModel(from: buffer[i], sessionCount: sCount, everSent: everSent)
        }
    }
}
