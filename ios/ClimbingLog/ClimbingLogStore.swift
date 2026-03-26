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
    private(set) var entries: [EntryViewModel] = []
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
        refreshEntries()
    }

    func serialize() {
        cl_log_serialize(handle.ptr, Self.logFileURL.path(percentEncoded: false))
    }

    func addEntry(_ data: CLEntryData) {
        cl_log_add_entry(handle.ptr, data)
        refreshEntries()
        serialize()
        isDirty = true
    }

    func updateEntry(id: UInt64, data: CLEntryData) {
        cl_log_set_entry(handle.ptr, id, data)
        refreshEntries()
        serialize()
        isDirty = true
    }

    func removeEntry(id: UInt64) {
        cl_log_remove_entry(handle.ptr, id)
        refreshEntries()
        serialize()
        isDirty = true
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

    // ─── Private ──────────────────────────────────────────────────────────────

    private func refreshEntries() {
        let count = cl_log_size(handle.ptr)
        guard count > 0 else {
            entries = []
            return
        }
        let buffer = UnsafeMutablePointer<CLEntry>.allocate(capacity: count)
        defer { buffer.deallocate() }
        let fetched = cl_log_get_entries(handle.ptr, 0, count, buffer)
        // Reverse so newest entries (highest timestamp) appear first.
        entries = (0..<fetched).map { EntryViewModel(from: buffer[$0]) }.reversed()
    }
}
