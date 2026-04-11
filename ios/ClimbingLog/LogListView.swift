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
                    ClimbRowView(climb: climb)
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
        .listStyle(.insetGrouped)
    }

    private var emptyState: some View {
        VStack(spacing: 12) {
            Image(systemName: "figure.climbing")
                .font(.system(size: 52))
                .foregroundStyle(.secondary)
            Text("No Climbs")
                .font(.title2)
                .fontWeight(.semibold)
            Text("Tap + to log your first session.")
                .font(.subheadline)
                .foregroundStyle(.secondary)
        }
    }
}

// ─── Row ──────────────────────────────────────────────────────────────────────

private struct ClimbRowView: View {
    let climb: ClimbViewModel

    var body: some View {
        HStack(alignment: .center, spacing: 12) {
            VStack(alignment: .leading, spacing: 4) {
                Text(climb.name)
                    .font(.body)
                    .fontWeight(.medium)
                HStack(spacing: 6) {
                    gradeChip
                    Text(climb.board.displayName)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            Spacer()

            VStack(alignment: .trailing, spacing: 4) {
                Text("\(climb.sessionCount) session\(climb.sessionCount == 1 ? "" : "s")")
                    .font(.caption)
                    .foregroundStyle(.secondary)
                Image(systemName: climb.everSent ? "checkmark.circle.fill" : "circle")
                    .foregroundStyle(climb.everSent ? .green : .secondary)
                    .font(.system(size: 16))
            }
        }
        .padding(.vertical, 2)
    }

    private var gradeChip: some View {
        Text(climb.grade.displayName)
            .font(.caption)
            .fontWeight(.medium)
            .padding(.horizontal, 7)
            .padding(.vertical, 2)
            .background(climb.grade.color.opacity(0.15))
            .foregroundStyle(climb.grade.color)
            .clipShape(Capsule())
    }
}
