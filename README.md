# climbinglog

Data store for a personal climbing log. This repo holds a single file, `data/log.json`, which is read and written by the `/climbing/edit` editor on [masonmil.com](https://masonmil.com), and read by the public `/climbing` page. There is no app or build in this repo — it exists so the log data has its own git history.

## `data/log.json`

```json
{
  "nextClimbID": 29,
  "climbs": [
    {
      "id": 27,
      "name": "A Wok to Remember",
      "board": "MoonBoard 2019",
      "grade": "6a+/V3",
      "nextSessionID": 1,
      "sessions": [
        { "id": 0, "timestamp": 1775589480, "attempts": 2, "incline": 40, "sent": true, "location": "Planet Rock Ann Arbor", "notes": "optional" }
      ]
    }
  ]
}
```

- Key order is exactly as shown. `notes` is the last session key.
- `board` is one of `"MoonBoard 2019"`, `"MoonBoard 2024"`.
- `grade` is one of `"6a+/V3"`, `"6b/V4"`, `"6c/V5"`, `"7a/V6"`, `"7a+/V7"`.
- `timestamp` is Unix epoch seconds.
- `attempts` is an integer from 1 to 999. `incline` is an integer from 0 to 70. `sent` is a boolean.
- `location` is required on every session and is one of `"Planet Rock Ann Arbor"`, `"Movement Long Island City"`.
- Every climb has at least one session; a climb with an empty `sessions` array is invalid.
- `notes` is optional and left out when empty. When present, it is trimmed, 1-280 characters (Unicode code points), and may contain line breaks. Notes are public.
- Climb IDs come from `nextClimbID`; session IDs come from the owning climb's `nextSessionID`. Both counters only increase and are never reused, even after deletion.
- `climbs` are sorted by `name` in code-point order. Each climb's `sessions` are sorted by `timestamp` ascending.
- The file is serialized with 2-space indentation, non-ASCII characters written as-is, and a single trailing newline.
- A climb is identified by `name` + `board`, compared exactly and case-sensitively. Duplicate name + board pairs are allowed.

## Editing

All edits go through the website's `/climbing/edit` editor, which is private and requires GitHub sign-in as the site owner. It reads and writes this file via the GitHub Contents API using a fine-grained PAT scoped to Contents read/write on this repo. Every save is one commit here, with a message describing the change (e.g. `Log session: <climb name>`).

Do not hand-edit `data/log.json` directly — the editor is the source of truth for the file's format and will re-sort and re-serialize it on the next save regardless.
