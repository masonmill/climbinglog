//
//  ClimbDetailView.swift
//  ClimbingLog
//

import SwiftUI

struct ClimbDetailView: View {
    @Environment(ClimbingLogStore.self) private var store
    let climb: ClimbViewModel
    // The specific session the user tapped to navigate here (if any).
    // When set, the toolbar "Edit" action opens that session's edit form
    // instead of the climb-identity form.
    var focusedSession: SessionViewModel? = nil
    @State private var activeSheet: ActiveSheet?
    @State private var sessionToDelete: SessionViewModel?

    // Single sheet dispatcher — multiple .sheet modifiers on the same view
    // don't fire reliably in SwiftUI, so we funnel all sheets through this.
    private enum ActiveSheet: Identifiable {
        case addSession
        case editClimb
        case editSession(SessionViewModel)

        var id: String {
            switch self {
            case .addSession:              "add"
            case .editClimb:                "editClimb"
            case .editSession(let s):       "editSession-\(s.id)"
            }
        }
    }

    var body: some View {
        let sessions = store.sessions(for: climb.id)
        let sentCount = sessions.filter(\.sent).count

        List {
            Section {
                LabeledContent("Grade", value: climb.grade.displayName)
                LabeledContent("Board", value: climb.board.displayName)
                if sentCount > 0 {
                    LabeledContent("Sends", value: "\(sentCount)")
                }
            }

            Section {
                if sessions.isEmpty {
                    Text("No sessions yet.")
                        .foregroundStyle(.secondary)
                } else {
                    ForEach(sessions) { session in
                        Button {
                            activeSheet = .editSession(session)
                        } label: {
                            SessionRowView(session: session)
                        }
                        .buttonStyle(.plain)
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
                Button { activeSheet = .addSession } label: {
                    Image(systemName: "plus")
                }
            }
            ToolbarItem(placement: .secondaryAction) {
                Button {
                    if let focused = focusedSession {
                        activeSheet = .editSession(focused)
                    } else {
                        activeSheet = .editClimb
                    }
                } label: {
                    Label(focusedSession != nil ? "Edit Session" : "Edit Climb",
                          systemImage: "pencil")
                }
            }
            if focusedSession != nil {
                ToolbarItem(placement: .secondaryAction) {
                    Button { activeSheet = .editClimb } label: {
                        Label("Edit Climb", systemImage: "square.and.pencil")
                    }
                }
            }
        }
        .sheet(item: $activeSheet) { sheet in
            switch sheet {
            case .addSession:
                SessionFormView(store: store, climbID: climb.id, climbName: climb.name,
                                board: climb.board, grade: climb.grade)
            case .editClimb:
                ClimbFormView(store: store, climb: climb)
            case .editSession(let session):
                SessionFormView(store: store, climb: climb, session: session)
            }
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
