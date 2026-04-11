//
//  ClimbDetailView.swift
//  ClimbingLog
//

import SwiftUI

struct ClimbDetailView: View {
    @Environment(ClimbingLogStore.self) private var store
    let climb: ClimbViewModel
    @State private var showingAddSession = false
    @State private var showingEditClimb = false
    @State private var sessionToDelete: SessionViewModel?

    var body: some View {
        let sessions = store.sessions(for: climb.id)
        let sentCount = sessions.filter(\.sent).count

        List {
            Section {
                HStack(spacing: 12) {
                    Text(climb.grade.displayName)
                        .font(.subheadline)
                        .fontWeight(.semibold)
                        .padding(.horizontal, 10)
                        .padding(.vertical, 5)
                        .background(climb.grade.color.opacity(0.15))
                        .foregroundStyle(climb.grade.color)
                        .clipShape(Capsule())
                    Text(climb.board.displayName)
                        .font(.subheadline)
                        .foregroundStyle(.secondary)
                    Spacer()
                    if sentCount > 0 {
                        Text("\(sentCount) send\(sentCount == 1 ? "" : "s")")
                            .font(.caption)
                            .foregroundStyle(.green)
                    }
                }
            }

            Section {
                if sessions.isEmpty {
                    Text("No sessions yet.")
                        .foregroundStyle(.secondary)
                } else {
                    ForEach(sessions) { session in
                        SessionRowView(session: session)
                            .swipeActions(edge: .trailing, allowsFullSwipe: false) {
                                Button(role: .destructive) {
                                    sessionToDelete = session
                                } label: {
                                    Label("Delete", systemImage: "trash")
                                }
                            }
                    }
                }
            } header: {
                Text("Sessions (\(sessions.count))")
            }
        }
        .navigationTitle(climb.name)
        .navigationBarTitleDisplayMode(.large)
        .toolbar {
            ToolbarItem(placement: .primaryAction) {
                Button { showingAddSession = true } label: {
                    Image(systemName: "plus")
                }
            }
            ToolbarItem(placement: .secondaryAction) {
                Button { showingEditClimb = true } label: {
                    Label("Edit Climb", systemImage: "pencil")
                }
            }
        }
        .sheet(isPresented: $showingAddSession) {
            SessionFormView(store: store, climbID: climb.id, climbName: climb.name,
                            board: climb.board, grade: climb.grade)
        }
        .sheet(isPresented: $showingEditClimb) {
            ClimbFormView(store: store, climb: climb)
        }
        .alert("Delete Session", isPresented: .init(
            get: { sessionToDelete != nil },
            set: { if !$0 { sessionToDelete = nil } }
        )) {
            Button("Delete", role: .destructive) {
                if let session = sessionToDelete {
                    store.removeSession(climbID: climb.id, sessionID: session.id)
                }
                sessionToDelete = nil
            }
            Button("Cancel", role: .cancel) {
                sessionToDelete = nil
            }
        } message: {
            Text("Delete this session? This cannot be undone.")
        }
    }
}

// ─── Session Row ──────────────────────────────────────────────────────────────

private struct SessionRowView: View {
    let session: SessionViewModel

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(session.date.formatted(date: .abbreviated, time: .omitted))
                    .font(.body)
                HStack(spacing: 8) {
                    Text("\(session.attempts) attempt\(session.attempts == 1 ? "" : "s")")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    Text(String(format: "%.0f°", session.incline))
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            Spacer()

            Image(systemName: session.sent ? "checkmark.circle.fill" : "circle")
                .foregroundStyle(session.sent ? .green : .secondary)
                .font(.system(size: 16))
        }
        .padding(.vertical, 2)
    }
}
