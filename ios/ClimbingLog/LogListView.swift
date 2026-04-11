//
//  LogListView.swift
//  ClimbingLog
//

import SwiftUI

struct LogListView: View {
    @Environment(ClimbingLogStore.self) private var store
    @State private var showingAddSession = false
    @State private var climbToDelete: ClimbViewModel?

    var body: some View {
        NavigationStack {
            Group {
                if store.climbs.isEmpty {
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
        .alert("Delete Climb", isPresented: .init(
            get: { climbToDelete != nil },
            set: { if !$0 { climbToDelete = nil } }
        )) {
            Button("Delete", role: .destructive) {
                if let climb = climbToDelete {
                    store.removeClimb(id: climb.id)
                }
                climbToDelete = nil
            }
            Button("Cancel", role: .cancel) {
                climbToDelete = nil
            }
        } message: {
            if let climb = climbToDelete {
                Text("Delete \"\(climb.name)\" and all its sessions? This cannot be undone.")
            }
        }
    }

    // ─── Subviews ─────────────────────────────────────────────────────────────

    private var list: some View {
        List {
            ForEach(store.climbs) { climb in
                NavigationLink(destination: ClimbDetailView(climb: climb)) {
                    ClimbRowView(climb: climb, store: store)
                }
                .swipeActions(edge: .trailing, allowsFullSwipe: false) {
                    Button(role: .destructive) {
                        climbToDelete = climb
                    } label: {
                        Label("Delete", systemImage: "trash")
                    }
                }
            }
        }
        .listStyle(.plain)
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

// ─── Row ──────────────────────────────────────────────────────────────────────

private struct ClimbRowView: View {
    let climb: ClimbViewModel
    let store: ClimbingLogStore

    var body: some View {
        let sessions = store.sessions(for: climb.id)
        let everSent = sessions.contains(where: \.sent)

        HStack(spacing: 12) {
            Text(climb.grade.displayName)
                .font(.caption)
                .fontWeight(.semibold)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(climb.grade.color.opacity(0.15))
                .foregroundStyle(climb.grade.color)
                .clipShape(Capsule())

            VStack(alignment: .leading, spacing: 2) {
                Text(climb.name)
                    .font(.body)
                    .fontWeight(.medium)
                Text(climb.board.displayName)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            if everSent {
                Image(systemName: "checkmark.circle.fill")
                    .foregroundStyle(.green)
                    .font(.system(size: 18))
            }
        }
        .padding(.vertical, 4)
    }
}
