#include "core/Archive.h"

#include <algorithm>

#include "core/DropDoc.h"

namespace dailydrop {

namespace {

std::vector<std::string> digestsNewestFirst(std::vector<std::string>& names) {
  std::vector<std::string> digests;
  digests.reserve(names.size());
  for (const auto& n : names) {
    if (isDropFilename(n)) digests.push_back(n);
  }
  std::sort(digests.begin(), digests.end(), std::greater<std::string>());
  return digests;
}

}  // namespace

std::vector<std::string> pruneList(std::vector<std::string> names, int keepDays) {
  std::vector<std::string> digests = digestsNewestFirst(names);
  if (keepDays < 0) keepDays = 0;
  if (static_cast<int>(digests.size()) <= keepDays) return {};
  return std::vector<std::string>(digests.begin() + keepDays, digests.end());
}

std::vector<std::string> archiveList(std::vector<std::string> names, int keepDays) {
  std::vector<std::string> digests = digestsNewestFirst(names);
  if (keepDays >= 0 && static_cast<int>(digests.size()) > keepDays) {
    digests.resize(static_cast<size_t>(keepDays));
  }
  return digests;
}

std::string dropPathFor(const std::string& isoDate) {
  return "/dailydrop/" + isoDate + ".drop";
}

}  // namespace dailydrop
