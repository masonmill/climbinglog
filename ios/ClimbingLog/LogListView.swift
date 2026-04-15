//
//  LogListView.swift
//  ClimbingLog
//

import SwiftUI

struct LogListView: View {
    @Environment(ClimbingLogStore.self) private var store
    @State private var showingAddSession = false
    @State private var sessionToDelete: SessionRow?

    // Climbs that have no sessions — shown at the bottom so they're never lost.
    private var sessionlessClimbs: [ClimbViewModel] {
        store.climbs.filter { $0.sessionCount == 0 }
    }

    var body: some View {
        NavigationStack {
            Group {
                if store.sessionsByDate.isEmpty && sessionlessClimbs.isEmpty {
                    emptyState
                } else {
                    list
                }
            }
            .navigationTitle("Climbing Log")
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    NavigationLink(destination: SettingsView()) {
                        Image(systemName: "gear")
                    }
                }
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button {
                        showingAddSession = true
                    } label: {
                        Image(systemName: "plus")
                    }
                }
            }
        }
        .sheet(isPresented: $showingAddSession) {
            SessionFormView(store: store)
        }
        .alert("Delete Session", isPresented: .init(
            get: { sessionToDelete != nil },
            set: { if !$0 { sessionToDelete = nil } }
        )) {
            Button("Delete", role: .destructive) {
                if let row = sessionToDelete {
                    store.removeSession(climbID: row.climbID, sessionID: row.sessionID)
                }
                sessionToDelete = nil
            }
            Button("Cancel", role: .cancel) {
                sessionToDelete = nil
            }
        } message: {
            if let row = sessionToDelete {
                Text("Delete this session for \"\(row.climbName)\"? This cannot be undone.")
            }
        }
    }

    // ─── Subviews ─────────────────────────────────────────────────────────────

    private var list: some View {
        List {
            // Date-grouped session rows, newest day first.
            ForEach(store.sessionsByDate, id: \.dateString) { group in
                Section(group.dateString) {
                    ForEach(group.rows) { row in
                        if let climb = store.climb(id: row.climbID) {
                            NavigationLink(destination: ClimbDetailView(climb: climb)) {
                                SessionRowView(row: row)
                            }
                            .swipeActions(edge: .trailing, allowsFullSwipe: false) {
                                Button(role: .destructive) {
                                    sessionToDelete = row
                                } label: {
                                    Label("Delete", systemImage: "trash")
                                }
                            }
                        }
                    }
                }
            }

            // Climbs that exist but have no sessions yet.
            if !sessionlessClimbs.isEmpty {
                Section("No Sessions") {
                    ForEach(sessionlessClimbs) { climb in
                        NavigationLink(destination: ClimbDetailView(climb: climb)) {
                            ClimbRowView(climb: climb)
                        }
                    }
                }
            }
        }
        .listStyle(.insetGrouped)
    }

    private var emptyState: some View {
        ContentUnavailableView {
            Label("No Climbs", systemImage: "figure.climbing")
        } description: {
            Text("Log your first session to get started.")
        } actions: {
            Button("Log Session") { showingAddSession = true }
                .buttonStyle(.borderedProminent)
        }
    }
}

// ─── Session row (date-grouped list) ──────────────────────────────────────────

private struct SessionRowView: View {
    let row: SessionRow

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(row.climbName)
                    .font(.body)
                Text("\(row.grade.displayName) · \(row.board.shortName) · \(Int(row.incline))° · \(row.attempts) \(row.attempts == 1 ? "attempt" : "attempts")")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            if row.label != .project {
                Text(row.label.rawValue)
                    .font(.caption)
                    .foregroundStyle(row.label.color)
            }
        }
        .padding(.vertical, 2)
    }
}

// ─── Climb row (session-less fallback section) ─────────────────────────────────

private struct ClimbRowView: View {
    let climb: ClimbViewModel

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(climb.name)
                .font(.body)
            Text("\(climb.grade.displayName) · \(climb.board.shortName)")
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .padding(.vertical, 2)
    }
}
