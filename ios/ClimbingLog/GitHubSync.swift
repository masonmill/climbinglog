//
//  GitHubSync.swift
//  ClimbingLog
//

import Foundation
import Security

// ─── Config ───────────────────────────────────────────────────────────────────

/// Non-sensitive GitHub settings (owner, repo) stored in UserDefaults.
/// The PAT is stored in the Keychain.
enum GitHubConfig {
    static var owner: String {
        get { UserDefaults.standard.string(forKey: "github_owner") ?? "masonmill" }
        set { UserDefaults.standard.set(newValue, forKey: "github_owner") }
    }

    static var repo: String {
        get { UserDefaults.standard.string(forKey: "github_repo") ?? "climbinglog" }
        set { UserDefaults.standard.set(newValue, forKey: "github_repo") }
    }

    static var isConfigured: Bool { !owner.isEmpty && !repo.isEmpty && pat != nil }

    // ─── Keychain ─────────────────────────────────────────────────────────────

    private static let keychainService = "com.climbinglog.github-pat"

    static var pat: String? {
        let query: [CFString: Any] = [
            kSecClass:       kSecClassGenericPassword,
            kSecAttrService: keychainService,
            kSecReturnData:  true,
            kSecMatchLimit:  kSecMatchLimitOne
        ]
        var result: AnyObject?
        guard SecItemCopyMatching(query as CFDictionary, &result) == errSecSuccess,
              let data = result as? Data else { return nil }
        return String(data: data, encoding: .utf8)
    }

    static func setPAT(_ token: String) {
        let data = Data(token.utf8)
        let query: [CFString: Any] = [
            kSecClass:       kSecClassGenericPassword,
            kSecAttrService: keychainService
        ]
        let status = SecItemUpdate(query as CFDictionary, [kSecValueData: data] as CFDictionary)
        if status == errSecItemNotFound {
            var newItem = query
            newItem[kSecValueData] = data
            SecItemAdd(newItem as CFDictionary, nil)
        }
    }

    static func deletePAT() {
        let query: [CFString: Any] = [
            kSecClass:       kSecClassGenericPassword,
            kSecAttrService: keychainService
        ]
        SecItemDelete(query as CFDictionary)
    }
}

// ─── Service ──────────────────────────────────────────────────────────────────

actor GitHubSyncService {
    static let shared = GitHubSyncService()

    private let filePath = "data/log.json"

    /// SHA of the file currently on GitHub, cached to avoid a GET before every PUT.
    private var cachedSHA: String? {
        get { UserDefaults.standard.string(forKey: "github_file_sha") }
        set { UserDefaults.standard.set(newValue, forKey: "github_file_sha") }
    }

    // ─── Errors ───────────────────────────────────────────────────────────────

    enum SyncError: Error, LocalizedError {
        case conflict
        case http(Int)

        var errorDescription: String? {
            switch self {
            case .conflict:    "SHA conflict — retrying with fresh SHA."
            case .http(let c): "GitHub API returned HTTP \(c)."
            }
        }
    }

    // ─── Public ───────────────────────────────────────────────────────────────

    /// Reads `fileURL` and pushes its contents to GitHub.
    /// On a 409 conflict the SHA cache is refreshed and the push is retried once.
    /// Config is passed in by the caller so this actor never touches @MainActor state.
    func sync(fileURL: URL, owner: String, repo: String, token: String) async throws {
        let content = try Data(contentsOf: fileURL)
        do {
            try await push(content: content, sha: cachedSHA, owner: owner, repo: repo, token: token)
        } catch SyncError.conflict {
            let freshSHA = try await fetchSHA(owner: owner, repo: repo, token: token)
            try await push(content: content, sha: freshSHA, owner: owner, repo: repo, token: token)
        }
    }

    // ─── Private ──────────────────────────────────────────────────────────────

    private func apiURL(owner: String, repo: String) -> URL {
        URL(string: "https://api.github.com/repos/\(owner)/\(repo)/contents/\(filePath)")!
    }

    private func makeRequest(owner: String, repo: String, token: String) -> URLRequest {
        var req = URLRequest(url: apiURL(owner: owner, repo: repo))
        req.setValue("Bearer \(token)",              forHTTPHeaderField: "Authorization")
        req.setValue("application/vnd.github+json", forHTTPHeaderField: "Accept")
        req.setValue("2022-11-28",                  forHTTPHeaderField: "X-GitHub-Api-Version")
        return req
    }

    private func push(content: Data, sha: String?, owner: String, repo: String, token: String) async throws {
        var req = makeRequest(owner: owner, repo: repo, token: token)
        req.httpMethod = "PUT"

        var body: [String: String] = [
            "message": "Update climbing log",
            "content": content.base64EncodedString()
        ]
        if let sha { body["sha"] = sha }
        req.httpBody = try JSONEncoder().encode(body)

        let (data, response) = try await URLSession.shared.data(for: req)
        let status = (response as! HTTPURLResponse).statusCode

        if status == 409 { throw SyncError.conflict }
        guard (200..<300).contains(status) else { throw SyncError.http(status) }

        // Cache the new blob SHA from the response so the next push skips a GET.
        if let json   = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
           let file   = json["content"] as? [String: Any],
           let newSHA = file["sha"] as? String {
            cachedSHA = newSHA
        }
    }

    private func fetchSHA(owner: String, repo: String, token: String) async throws -> String {
        let (data, response) = try await URLSession.shared.data(for: makeRequest(owner: owner, repo: repo, token: token))
        let status = (response as! HTTPURLResponse).statusCode
        guard (200..<300).contains(status),
              let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let sha  = json["sha"] as? String else {
            throw SyncError.http(status)
        }
        cachedSHA = sha
        return sha
    }
}
