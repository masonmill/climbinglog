//
//  EntryFormView.swift
//  ClimbingLog
//

import SwiftUI

struct EntryFormView: View {
    @Environment(\.dismiss) private var dismiss
    let store: ClimbingLogStore
    let entryID: UInt64?

    @State private var name: String
    @State private var date: Date
    @State private var board: Board
    @State private var grade: Grade
    @State private var attempts: Int
    @State private var incline: Double
    @State private var sent: Bool

    init(store: ClimbingLogStore, entry: EntryViewModel? = nil) {
        self.store = store
        self.entryID = entry?.id
        _name     = State(initialValue: entry?.name ?? "")
        _date     = State(initialValue: entry?.date ?? Date())
        _board    = State(initialValue: entry?.board ?? .mb2019)
        _grade    = State(initialValue: entry?.grade ?? .v3)
        _attempts = State(initialValue: Int(entry?.attempts ?? 1))
        _incline  = State(initialValue: entry?.incline ?? 40.0)
        _sent     = State(initialValue: entry?.sent ?? false)
    }

    var body: some View {
        NavigationStack {
            Form {
                Section("Problem") {
                    TextField("Name", text: $name)
                    DatePicker("Date", selection: $date, displayedComponents: .date)
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

                Section("Session") {
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
            .navigationTitle(entryID == nil ? "New Entry" : "Edit Entry")
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

    private func save() {
        let data = makeEntryData(name: name, date: date, board: board, grade: grade,
                                 attempts: UInt32(attempts), incline: incline, sent: sent)
        if let id = entryID {
            store.updateEntry(id: id, data: data)
        } else {
            store.addEntry(data)
        }
        dismiss()
    }
}
