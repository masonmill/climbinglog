//
//  ContentView.swift
//  ClimbingLog
//
//  Created by Mason on 3/25/26.
//

import SwiftUI

struct ContentView: View {
    var body: some View {
        LogListView()
    }
}

#Preview {
    ContentView()
        .environment(ClimbingLogStore())
}
