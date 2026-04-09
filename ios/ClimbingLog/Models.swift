//
//  Models.swift
//  ClimbingLog
//

import Foundation
import SwiftUI

// ─── Board ────────────────────────────────────────────────────────────────────

enum Board: Int32, CaseIterable, Hashable {
    case mb2019 = 0
    case mb2024 = 1

    init(_ cl: CLBoard) {
        self = Board(rawValue: Int32(cl.rawValue)) ?? .mb2019
    }

    var clValue: CLBoard { CLBoard(UInt32(rawValue)) }

    var displayName: String {
        switch self {
        case .mb2019: "MoonBoard 2019"
        case .mb2024: "MoonBoard 2024"
        }
    }
}

// ─── Grade ────────────────────────────────────────────────────────────────────

enum Grade: Int32, CaseIterable, Hashable {
    case v3 = 0, v4 = 1, v5 = 2, v6 = 3, v7 = 4

    init(_ cl: CLGrade) {
        self = Grade(rawValue: Int32(cl.rawValue)) ?? .v3
    }

    var clValue: CLGrade { CLGrade(UInt32(rawValue)) }

    var displayName: String {
        switch self {
        case .v3: "6a+/V3"
        case .v4: "6b/V4"
        case .v5: "6c/V5"
        case .v6: "7a/V6"
        case .v7: "7a+/V7"
        }
    }

    var color: Color {
        switch self {
        case .v3: .green
        case .v4: .blue
        case .v5: .orange
        case .v6: .red
        case .v7: .purple
        }
    }
}

// ─── ClimbViewModel ───────────────────────────────────────────────────────────

struct ClimbViewModel: Identifiable {
    let id: UInt64
    var name: String
    var board: Board
    var grade: Grade

    init(from climb: CLClimb) {
        id = climb.id
        name = withUnsafeBytes(of: climb.data.name) { raw in
            String(cString: raw.baseAddress!.assumingMemoryBound(to: CChar.self))
        }
        board = Board(climb.data.board)
        grade = Grade(climb.data.grade)
    }
}

// ─── SessionViewModel ─────────────────────────────────────────────────────────

struct SessionViewModel: Identifiable {
    let id: UInt64
    let climbID: UInt64
    var date: Date
    var attempts: UInt32
    var incline: Double
    var sent: Bool

    init(from session: CLSession, climbID: UInt64) {
        id = session.id
        self.climbID = climbID
        date = Date(timeIntervalSince1970: TimeInterval(session.data.timestamp))
        attempts = session.data.attempts
        incline = session.data.incline
        sent = session.data.sent
    }
}

// ─── CLClimbData builder ──────────────────────────────────────────────────────

func makeClimbData(name: String, board: Board, grade: Grade) -> CLClimbData {
    var data = CLClimbData()
    data.board = board.clValue
    data.grade = grade.clValue
    let truncated = String(name.prefix(255))
    truncated.utf8CString.withUnsafeBytes { src in
        withUnsafeMutableBytes(of: &data.name) { dst in
            dst.copyMemory(from: UnsafeRawBufferPointer(start: src.baseAddress,
                                                        count: min(src.count, 256)))
        }
    }
    return data
}

// ─── CLSessionData builder ────────────────────────────────────────────────────

func makeSessionData(date: Date, attempts: UInt32, incline: Double, sent: Bool) -> CLSessionData {
    var data = CLSessionData()
    data.timestamp = Int64(date.timeIntervalSince1970)
    data.attempts = attempts
    data.incline = incline
    data.sent = sent
    return data
}
