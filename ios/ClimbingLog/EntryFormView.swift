//
//  SessionFormView.swift
//  ClimbingLog
//

import SwiftUI

// ─── SessionFormView ──────────────────────────────────────────────────────────
//
// Used for logging a new session. When opened from ClimbDetailView, the climb
// fields are pre-filled and locked. When opened from the + button in the list,
// the name field supports autocomplete from existing climbs.

struct SessionFormView: View {
    @Environment(\.dismiss) private var dismiss
    let store: ClimbingLogStore

    // If editing an existing session on a known climb
    let climbID: UInt64?
    let sessionID: UInt64?

    @State private var name: String
    @State private var board: Board
    @State private var grade: Grade
    @State private var date: Date
    @State private var attempts: Int
    @State private var incline: Double
    @State private var sent: Bool

    @State private var suggestions: [ClimbViewModel] = []
    @State private var isExistingClimb: Bool

    // New session on a known climb (from ClimbDetailView)
    init(store: ClimbingLogStore, climbID: UInt64, climbName: String,
         board: Board, grade: Grade) {
        self.store = store
        self.climbID = climbID
        self.sessionID = nil
        _name = State(initialValue: climbName)
        _board = State(initialValue: board)
        _grade = State(initialValue: grade)
        _date = State(initialValue: Date())
        _attempts = State(initialValue: 1)
        _incline = State(initialValue: 40.0)
        _sent = State(initialValue: false)
        _isExistingClimb = State(initialValue: true)
    }

    // New session (from + button, no climb selected)
    init(store: ClimbingLogStore) {
        self.store = store
        self.climbID = nil
        self.sessionID = nil
        _name = State(initialValue: "")
        _board = State(initialValue: .mb2019)
        _grade = State(initialValue: .v3)
        _date = State(initialValue: Date())
        _attempts = State(initialValue: 1)
        _incline = State(initialValue: 40.0)
        _sent = State(initialValue: false)
        _isExistingClimb = State(initialValue: false)
    }

    private var climbLocked: Bool { climbID != nil }

    var body: some View {
        NavigationStack {
            Form {
                Section("Problem") {
                    if climbLocked {
                        LabeledContent("Name", value: name)
                        LabeledContent("Board", value: board.displayName)
                        LabeledContent("Grade", value: grade.displayName)
                    } else {
                        TextField("Name", text: $name)
                            .onChange(of: name) { _, newValue in
                                updateSuggestions(for: newValue)
                            }
                            .autocorrectionDisabled()

                        if !suggestions.isEmpty {
                            ForEach(suggestions) { climb in
                                Button {
                                    selectSuggestion(climb)
                                } label: {
                                    HStack {
                                        Text(climb.name)
                                            .foregroundStyle(.primary)
                                        Spacer()
                                        Text(climb.grade.displayName)
                                            .font(.caption)
                                            .foregroundStyle(climb.grade.color)
                                        Text(climb.board.displayName)
                                            .font(.caption)
                                            .foregroundStyle(.secondary)
                                    }
                                }
                            }
                        }

                        if !isExistingClimb {
                            Picker("Board", selection: $board) {
                                ForEach(Board.allCases, id: \.self) { board in
                                    Text(board.displayName).tag(board)
                                }
                            }
                            Picker("Grade", selection: $grade) {
                                ForEach(Grade.allCases, id: \.self) { grade in
                                    Text(grade.displayName).tag(grade)
                                }
                            }
                        } else {
                            LabeledContent("Board", value: board.displayName)
                            LabeledContent("Grade", value: grade.displayName)
                        }
                    }
                }

                Section("Session") {
                    DatePicker("Date", selection: $date, displayedComponents: .date)
                    Stepper("Attempts: \(attempts)", value: $attempts, in: 1...999)
                    VStack(alignment: .leading, spacing: 4) {
                        HStack {
                            Text("Incline")
                            Spacer()
                            Text(String(format: "%.0f°", incline))
                                .foregroundStyle(.secondary)
                        }
                        Slider(value: $incline, in: 0...70, step: 1)
                    }
                    Toggle("Sent", isOn: $sent)
                }
            }
            .navigationTitle("Log Session")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Save") { save() }
                        .disabled(name.trimmingCharacters(in: .whitespaces).isEmpty)
                }
            }
        }
    }

    // ─── Autocomplete ─────────────────────────────────────────────────────

    private func updateSuggestions(for query: String) {
        let trimmed = query.trimmingCharacters(in: .whitespaces)
        guard !trimmed.isEmpty else {
            suggestions = []
            isExistingClimb = false
            return
        }
        suggestions = store.climbs.filter {
            $0.name.localizedCaseInsensitiveContains(trimmed)
        }
        // If user typed exactly an existing name, auto-select it
        if let exact = suggestions.first(where: { $0.name == trimmed }) {
            selectSuggestion(exact)
        }
    }

    private func selectSuggestion(_ climb: ClimbViewModel) {
        name = climb.name
        board = climb.board
        grade = climb.grade
        isExistingClimb = true
        suggestions = []
    }

    // ─── Save ─────────────────────────────────────────────────────────────

    private func save() {
        let sessionData = makeSessionData(date: date, attempts: UInt32(attempts),
                                           incline: incline, sent: sent)

        if let cid = climbID {
            // Adding session to a known climb
            store.addSession(climbID: cid, data: sessionData)
        } else if let existing = store.existingClimb(name: name, board: board) {
            // Adding session to an existing climb found by name+board
            store.addSession(climbID: existing.id, data: sessionData)
        } else {
            // Create a new climb, then add the session
            let climbData = makeClimbData(name: name, board: board, grade: grade)
            let newClimbID = store.addClimb(climbData)
            store.addSession(climbID: newClimbID, data: sessionData)
        }
        dismiss()
    }
}

// ─── ClimbFormView ────────────────────────────────────────────────────────────
//
// Edit the identity (name, board, grade) of an existing climb.

struct ClimbFormView: View {
    @Environment(\.dismiss) private var dismiss
    let store: ClimbingLogStore
    let climbID: UInt64

    @State private var name: String
    @State private var board: Board
    @State private var grade: Grade

    init(store: ClimbingLogStore, climb: ClimbViewModel) {
        self.store = store
        self.climbID = climb.id
        _name = State(initialValue: climb.name)
        _board = State(initialValue: climb.board)
        _grade = State(initialValue: climb.grade)
    }

    var body: some View {
        NavigationStack {
            Form {
                Section("Problem") {
                    TextField("Name", text: $name)
                    Picker("Board", selection: $board) {
                        ForEach(Board.allCases, id: \.self) { board in
                            Text(board.displayName).tag(board)
                        }
                    }
                    Picker("Grade", selection: $grade) {
                        ForEach(Grade.allCases, id: \.self) { grade in
                            Text(grade.displayName).tag(grade)
                        }
                    }
                }
            }
            .navigationTitle("Edit Climb")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Save") {
                        let data = makeClimbData(name: name, board: board, grade: grade)
                        store.updateClimb(id: climbID, data: data)
                        dismiss()
                    }
                    .disabled(name.trimmingCharacters(in: .whitespaces).isEmpty)
                }
            }
        }
    }
}
