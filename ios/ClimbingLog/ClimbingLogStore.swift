//
//  ClimbingLogStore.swift
//  ClimbingLog
//

import Foundation
import Observation

@Observable
class ClimbingLogStore {
    private(set) var entries: [EntryViewModel] = []
    private var logHandle: OpaquePointer!

    static var logFileURL: URL {
        FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("climbing_log.json")
    }

    init() {
        logHandle = cl_log_create()
        let path = Self.logFileURL.path(percentEncoded: false)
        if FileManager.default.fileExists(atPath: path) {
            cl_log_deserialize(logHandle, path)
        }
        refreshEntries()
    }

    deinit {
        cl_log_destroy(logHandle)
    }

    func serialize() {
        cl_log_serialize(logHandle, Self.logFileURL.path(percentEncoded: false))
    }

    func addEntry(_ data: CLEntryData) {
        cl_log_add_entry(logHandle, data)
        refreshEntries()
        serialize()
    }

    func updateEntry(id: UInt64, data: CLEntryData) {
        cl_log_set_entry(logHandle, id, data)
        refreshEntries()
        serialize()
    }

    func removeEntry(id: UInt64) {
        cl_log_remove_entry(logHandle, id)
        refreshEntries()
        serialize()
    }

    // ─── Private ──────────────────────────────────────────────────────────────

    private func refreshEntries() {
        let count = cl_log_size(logHandle)
        guard count > 0 else {
            entries = []
            return
        }
        let buffer = UnsafeMutablePointer<CLEntry>.allocate(capacity: count)
        defer { buffer.deallocate() }
        let fetched = cl_log_get_entries(logHandle, 0, count, buffer)
        // Reverse so newest entries (highest timestamp) appear first.
        entries = (0..<fetched).map { EntryViewModel(from: buffer[$0]) }.reversed()
    }
}
