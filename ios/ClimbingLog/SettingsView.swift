//
//  SettingsView.swift
//  ClimbingLog
//

import SwiftUI

struct SettingsView: View {
    @Environment(ClimbingLogStore.self) private var store
    @State private var owner = GitHubConfig.owner
    @State private var repo  = GitHubConfig.repo
    @State private var pat   = GitHubConfig.pat ?? ""
    @State private var showPAT = false

    var body: some View {
        Form {
            Section("GitHub Repository") {
                LabeledContent("Owner") {
                    TextField("username", text: $owner)
                        .multilineTextAlignment(.trailing)
                        .autocorrectionDisabled()
                        .textInputAutocapitalization(.never)
                }
                LabeledContent("Repository") {
                    TextField("climbinglog", text: $repo)
                        .multilineTextAlignment(.trailing)
                        .autocorrectionDisabled()
                        .textInputAutocapitalization(.never)
                }
            }

            Section {
                LabeledContent("Token") {
                    HStack(spacing: 8) {
                        Group {
                            if showPAT {
                                TextField("ghp_…", text: $pat)
                            } else {
                                SecureField("ghp_…", text: $pat)
                            }
                        }
                        .multilineTextAlignment(.trailing)
                        .autocorrectionDisabled()
                        .textInputAutocapitalization(.never)
                        Button {
                            showPAT.toggle()
                        } label: {
                            Image(systemName: showPAT ? "eye.slash" : "eye")
                                .foregroundStyle(.secondary)
                        }
                        .buttonStyle(.plain)
                    }
                }
            } header: {
                Text("Personal Access Token")
            } footer: {
                Text("Classic PAT: enable public_repo.")
            }

            Section {
                Button("Save") { save() }
                    .disabled(!hasChanges)
                Button("Push to GitHub") {
                    Task { await store.sync() }
                }
                .disabled(!GitHubConfig.isConfigured || store.syncState == .syncing)
                Button("Pull from GitHub") {
                    Task { await store.pullFromRemote() }
                }
                .disabled(!GitHubConfig.isConfigured || store.syncState == .syncing)
            }

            Section("Sync Status") {
                LabeledContent("State") {
                    Text(syncStatusText)
                        .foregroundStyle(syncStatusColor)
                }
            }
        }
        .navigationTitle("GitHub Sync")
        .navigationBarTitleDisplayMode(.inline)
    }

    // ─── Private ──────────────────────────────────────────────────────────────

    private var hasChanges: Bool {
        owner != GitHubConfig.owner ||
        repo  != GitHubConfig.repo  ||
        pat   != (GitHubConfig.pat ?? "")
    }

    private var syncStatusText: String {
        switch store.syncState {
        case .idle:             store.isDirty ? "Pending sync" : "Up to date"
        case .syncing:          "Syncing…"
        case .synced(let date): "Synced at \(date.formatted(date: .omitted, time: .shortened))"
        case .failed(let msg):  "Failed: \(msg)"
        }
    }

    private var syncStatusColor: Color {
        switch store.syncState {
        case .idle:    store.isDirty ? .orange : .secondary
        case .syncing: .blue
        case .synced:  .green
        case .failed:  .red
        }
    }

    private func save() {
        GitHubConfig.owner = owner
        GitHubConfig.repo  = repo
        if pat.isEmpty {
            GitHubConfig.deletePAT()
        } else {
            GitHubConfig.setPAT(pat)
        }
    }
}
