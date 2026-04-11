//
//  ClimbingLogApp.swift
//  ClimbingLog
//
//  Created by Mason on 3/25/26.
//

import SwiftUI

@main
struct ClimbingLogApp: App {
    @State private var store = ClimbingLogStore()
    @Environment(\.scenePhase) private var scenePhase

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environment(store)
                .task {
                    await store.syncIfNeeded()
                    await store.pullFromRemote()
                }
        }
        .onChange(of: scenePhase) { _, phase in
            if phase == .background {
                store.serialize()
                Task { await store.syncIfNeeded() }
            }
        }
    }
}
