//
//  EntryDetailView.swift
//  ClimbingLog
//

import SwiftUI

struct EntryDetailView: View {
    @Environment(ClimbingLogStore.self) private var store
    let entry: EntryViewModel
    @State private var showingEdit = false

    var body: some View {
        Form {
            Section("Problem") {
                LabeledContent("Name", value: entry.name)
                LabeledContent("Date", value: entry.date.formatted(date: .long, time: .omitted))
                LabeledContent("Board", value: entry.board.displayName)
                LabeledContent("Grade") {
                    Text(entry.grade.displayName)
                        .foregroundStyle(entry.grade.color)
                        .fontWeight(.medium)
                }
            }

            Section("Session") {
                LabeledContent("Attempts", value: String(entry.attempts))
                LabeledContent("Incline", value: String(format: "%.0f°", entry.incline))
                LabeledContent("Sent") {
                    HStack(spacing: 4) {
                        Image(systemName: entry.sent ? "checkmark.circle.fill" : "circle")
                            .foregroundStyle(entry.sent ? .green : .secondary)
                        Text(entry.sent ? "Yes" : "No")
                            .foregroundStyle(entry.sent ? .primary : .secondary)
                    }
                }
            }
        }
        .navigationTitle(entry.name)
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
            ToolbarItem(placement: .navigationBarTrailing) {
                Button("Edit") { showingEdit = true }
            }
        }
        .sheet(isPresented: $showingEdit) {
            // Look up the current version of this entry in case it was updated.
            if let current = store.entries.first(where: { $0.id == entry.id }) {
                EntryFormView(store: store, entry: current)
            }
        }
    }
}
