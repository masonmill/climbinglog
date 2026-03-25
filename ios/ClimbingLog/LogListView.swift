//
//  LogListView.swift
//  ClimbingLog
//

import SwiftUI

struct LogListView: View {
    @Environment(ClimbingLogStore.self) private var store
    @State private var showingAddEntry = false
    @State private var entryToDelete: EntryViewModel?

    var body: some View {
        NavigationStack {
            Group {
                if store.entries.isEmpty {
                    emptyState
                } else {
                    list
                }
            }
            .navigationTitle("Climbing Log")
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button {
                        showingAddEntry = true
                    } label: {
                        Image(systemName: "plus")
                    }
                }
            }
        }
        .sheet(isPresented: $showingAddEntry) {
            EntryFormView(store: store)
        }
        .alert("Delete Entry", isPresented: .init(
            get: { entryToDelete != nil },
            set: { if !$0 { entryToDelete = nil } }
        )) {
            Button("Delete", role: .destructive) {
                if let entry = entryToDelete {
                    store.removeEntry(id: entry.id)
                }
                entryToDelete = nil
            }
            Button("Cancel", role: .cancel) {
                entryToDelete = nil
            }
        } message: {
            if let entry = entryToDelete {
                Text("Delete \"\(entry.name)\"? This cannot be undone.")
            }
        }
    }

    // ─── Subviews ─────────────────────────────────────────────────────────────

    private var list: some View {
        List {
            ForEach(store.entries) { entry in
                NavigationLink(destination: EntryDetailView(entry: entry)) {
                    EntryRowView(entry: entry)
                }
                .swipeActions(edge: .trailing, allowsFullSwipe: false) {
                    Button(role: .destructive) {
                        entryToDelete = entry
                    } label: {
                        Label("Delete", systemImage: "trash")
                    }
                }
            }
        }
        .listStyle(.insetGrouped)
    }

    private var emptyState: some View {
        VStack(spacing: 12) {
            Image(systemName: "figure.climbing")
                .font(.system(size: 52))
                .foregroundStyle(.secondary)
            Text("No Entries")
                .font(.title2)
                .fontWeight(.semibold)
            Text("Tap + to log your first problem.")
                .font(.subheadline)
                .foregroundStyle(.secondary)
        }
    }
}

// ─── Row ──────────────────────────────────────────────────────────────────────

private struct EntryRowView: View {
    let entry: EntryViewModel

    var body: some View {
        HStack(alignment: .center, spacing: 12) {
            VStack(alignment: .leading, spacing: 4) {
                Text(entry.name)
                    .font(.body)
                    .fontWeight(.medium)
                HStack(spacing: 6) {
                    gradeChip
                    Text(entry.board.displayName)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            Spacer()

            VStack(alignment: .trailing, spacing: 4) {
                Text(entry.date.formatted(date: .abbreviated, time: .omitted))
                    .font(.caption)
                    .foregroundStyle(.secondary)
                Image(systemName: entry.sent ? "checkmark.circle.fill" : "circle")
                    .foregroundStyle(entry.sent ? .green : .secondary)
                    .font(.system(size: 16))
            }
        }
        .padding(.vertical, 2)
    }

    private var gradeChip: some View {
        Text(entry.grade.displayName)
            .font(.caption)
            .fontWeight(.medium)
            .padding(.horizontal, 7)
            .padding(.vertical, 2)
            .background(entry.grade.color.opacity(0.15))
            .foregroundStyle(entry.grade.color)
            .clipShape(Capsule())
    }
}
