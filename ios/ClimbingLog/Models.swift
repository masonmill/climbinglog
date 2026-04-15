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

    var shortName: String {
        switch self {
        case .mb2019: "MB 2019"
        case .mb2024: "MB 2024"
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
    var sessionCount: Int
    var everSent: Bool

    init(from climb: CLClimb, sessionCount: Int = 0, everSent: Bool = false) {
        id = climb.id
        name = withUnsafeBytes(of: climb.data.name) { raw in
            String(cString: raw.baseAddress!.assumingMemoryBound(to: CChar.self))
        }
        board = Board(climb.data.board)
        grade = Grade(climb.data.grade)
        self.sessionCount = sessionCount
        self.everSent = everSent
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

// ─── SessionLabel ─────────────────────────────────────────────────────────────

enum SessionLabel: String {
    case flash = "Flash"
    case dayFlash = "Day flash"
    case sent = "Sent"
    case `repeat` = "Repeat"
    case project = "Project"

    var color: Color {
        switch self {
        case .flash, .dayFlash, .sent: .green
        case .repeat: .blue
        case .project: .secondary
        }
    }
}

/// Computes the send label for a session given its position in chronological order.
/// `allChronologicalSessions` must be sorted oldest-first.
func computeSessionLabel(
    sent: Bool,
    attempts: UInt32,
    sessionIndex: Int,
    allChronologicalSessions: [SessionViewModel]
) -> SessionLabel {
    guard sent else { return .project }
    let previouslySent = allChronologicalSessions.prefix(sessionIndex).contains(where: \.sent)
    if previouslySent { return .repeat }
    if attempts == 1 && sessionIndex == 0 { return .flash }
    if attempts == 1 { return .dayFlash }
    return .sent
}

// ─── SessionRow ───────────────────────────────────────────────────────────────

/// A flattened session record enriched with its parent climb's info and a send label.
/// Used for the date-grouped main list view.
struct SessionRow: Identifiable {
    let climbID: UInt64
    let sessionID: UInt64
    let climbName: String
    let board: Board
    let grade: Grade
    let date: Date
    let attempts: UInt32
    let incline: Double
    let sent: Bool
    let label: SessionLabel
    let sessionCountForClimb: Int

    var id: String { "\(climbID)-\(sessionID)" }
}
