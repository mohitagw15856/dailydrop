// Archive policy for stored digests: which files to keep, which to prune.
// Pure logic so the 14-day window is host-tested; the device layer supplies
// the directory listing and performs the deletions.
#pragma once

#include <string>
#include <vector>

namespace dailydrop {

constexpr int kArchiveDays = 14;

// Given the digest filenames present on card (any order, non-digest names
// ignored), returns the filenames to DELETE so that only the newest
// `keepDays` digests remain. Date order is lexical order for YYYY-MM-DD.
std::vector<std::string> pruneList(std::vector<std::string> names, int keepDays = kArchiveDays);

// Digest filenames to show in the archive browser: newest first, at most
// `keepDays` entries, non-digest names ignored.
std::vector<std::string> archiveList(std::vector<std::string> names, int keepDays = kArchiveDays);

// The on-card path for a given date's digest, e.g. "/dailydrop/2026-07-31.drop".
std::string dropPathFor(const std::string& isoDate);

}  // namespace dailydrop
