#include "device/DailyDropApp.h"

#ifdef ARDUINO

#include <ctime>

#include <inkkit/SdStream.h>
#include <inkkit/Storage.h>

#include "core/Archive.h"
#include "device/Config.h"

namespace dailydrop {

namespace {

// Hard caps so a malformed or oversized digest cannot exhaust RAM; the
// companion builder enforces smaller limits at build time (docs/FORMAT.md).
constexpr size_t kMaxSections = 12;
constexpr size_t kMaxLinesPerSection = 200;

std::string todayIso() {
  // halClock.begin() (called in main) seeds time() from the RTC when present.
  // TODO(hardware-test): confirm the RTC path; without a clock this yields
  // 1970-01-01, and the reader simply shows the newest stored digest instead.
  const time_t now = time(nullptr);
  struct tm parts;
  gmtime_r(&now, &parts);
  char buf[11];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", parts.tm_year + 1900, parts.tm_mon + 1,
           parts.tm_mday);
  return std::string(buf);
}

}  // namespace

void DailyDropApp::begin() {
  inkkit::sd::ensureDir(kAppRoot);
  pruneArchive();
  refreshArchiveNames();
  // Open the newest stored digest; if none, show a friendly empty state.
  if (!archive_.empty() && loadDrop(std::string(kAppRoot) + "/" + archive_.front())) {
    screen_ = Screen::Reader;
    renderReader(true);
  } else {
    screen_ = Screen::Message;
    renderMessage("DailyDrop", "No digests yet. Press SYNC to fetch today's drop.");
  }
}

void DailyDropApp::onEvent(const DropEvent& e) {
  switch (e.kind) {
    case DropEvent::Kind::Meta:
      if (e.key == "date") drop_.date = e.text;
      if (e.key == "title") drop_.title = e.text;
      break;
    case DropEvent::Kind::Section:
      if (drop_.sections.size() < kMaxSections) {
        drop_.sections.push_back({e.text, {}});
      }
      break;
    case DropEvent::Kind::Heading:
    case DropEvent::Kind::Text:
    case DropEvent::Kind::Rule: {
      if (drop_.sections.empty()) drop_.sections.push_back({"Digest", {}});
      auto& lines = drop_.sections.back().lines;
      if (lines.size() >= kMaxLinesPerSection) break;
      uint8_t kind = 0;
      if (e.kind == DropEvent::Kind::Heading) kind = 1;
      if (e.kind == DropEvent::Kind::Rule) kind = 2;
      lines.push_back({kind, e.text});
      break;
    }
  }
}

bool DailyDropApp::loadDrop(const std::string& path) {
  drop_ = LoadedDrop{};
  HalFile file;
  if (!inkkit::sd::openRead(kTag, path.c_str(), file)) return false;
  DropParser parser(*this);
  bool ok = true;
  inkkit::readLines(file, [&](const std::string& line) {
    if (ok) ok = parser.feedLine(line);
  });
  file.close();
  if (!ok || !parser.sawMagic() || drop_.sections.empty()) return false;
  if (drop_.date.empty()) drop_.date = dropDate(path);
  section_ = 0;
  scroll_ = 0;
  return true;
}

void DailyDropApp::pruneArchive() {
  std::vector<std::string> names;
  inkkit::sd::listFiles(kAppRoot, ".drop",
                        [&](const std::string& path) {
                          const size_t slash = path.find_last_of('/');
                          names.push_back(slash == std::string::npos ? path
                                                                     : path.substr(slash + 1));
                        });
  for (const auto& doomed : pruneList(names)) {
    const std::string full = std::string(kAppRoot) + "/" + doomed;
    Storage.remove(full.c_str());
  }
}

void DailyDropApp::refreshArchiveNames() {
  std::vector<std::string> names;
  inkkit::sd::listFiles(kAppRoot, ".drop",
                        [&](const std::string& path) {
                          const size_t slash = path.find_last_of('/');
                          names.push_back(slash == std::string::npos ? path
                                                                     : path.substr(slash + 1));
                        });
  archive_ = archiveList(names);
  if (archiveSel_ >= static_cast<int>(archive_.size())) archiveSel_ = 0;
}

void DailyDropApp::doSync() {
  renderMessage("Sync", "Connecting...");
  FetchResult r = FetchResult::NoConfig;
  if (fetcher_.loadConfig()) {
    r = fetcher_.fetch(todayIso());
  }
  if (r == FetchResult::Ok) {
    pruneArchive();
    refreshArchiveNames();
    if (loadDrop(dropPathFor(todayIso()))) {
      screen_ = Screen::Reader;
      renderReader(true);
      return;
    }
  }
  screen_ = Screen::Message;
  renderMessage("Sync", fetchResultLabel(r));
}

void DailyDropApp::tick() {
  buttons_.update();

  if (buttons_.wasPressed(kBtnSync)) {
    doSync();
    return;
  }

  switch (screen_) {
    case Screen::Reader: {
      const int pageLines = (tr_.height() - 40) / tr_.lineHeight();
      if (buttons_.wasPressed(kBtnSelect)) {
        if (!drop_.sections.empty()) {
          section_ = (section_ + 1) % static_cast<int>(drop_.sections.size());
          scroll_ = 0;
          renderReader(false);
        }
      } else if (buttons_.wasPressed(kBtnDown)) {
        const auto& lines = drop_.sections[static_cast<size_t>(section_)].lines;
        if (scroll_ + pageLines < static_cast<int>(lines.size())) {
          scroll_ += pageLines;
          renderReader(false);
        }
      } else if (buttons_.wasPressed(kBtnUp)) {
        if (scroll_ > 0) {
          scroll_ -= pageLines;
          if (scroll_ < 0) scroll_ = 0;
          renderReader(false);
        }
      } else if (buttons_.wasPressed(kBtnBack)) {
        refreshArchiveNames();
        screen_ = Screen::Archive;
        renderArchive(true);
      }
      break;
    }
    case Screen::Archive: {
      if (buttons_.wasPressed(kBtnDown) && archiveSel_ + 1 < static_cast<int>(archive_.size())) {
        ++archiveSel_;
        renderArchive(false);
      } else if (buttons_.wasPressed(kBtnUp) && archiveSel_ > 0) {
        --archiveSel_;
        renderArchive(false);
      } else if (buttons_.wasPressed(kBtnSelect) && !archive_.empty()) {
        if (loadDrop(std::string(kAppRoot) + "/" + archive_[static_cast<size_t>(archiveSel_)])) {
          screen_ = Screen::Reader;
          renderReader(true);
        }
      } else if (buttons_.wasPressed(kBtnBack) && !drop_.sections.empty()) {
        screen_ = Screen::Reader;
        renderReader(true);
      }
      break;
    }
    case Screen::Message: {
      if (buttons_.wasPressed(kBtnBack) || buttons_.wasPressed(kBtnSelect)) {
        refreshArchiveNames();
        screen_ = Screen::Archive;
        renderArchive(true);
      }
      break;
    }
  }
}

void DailyDropApp::renderReader(bool full) {
  tr_.clear();
  const auto& sec = drop_.sections[static_cast<size_t>(section_)];

  // Header bar: date + section name + position.
  std::string head = drop_.date + "  " + sec.name;
  char pos[16];
  snprintf(pos, sizeof(pos), "  %d/%d", section_ + 1, static_cast<int>(drop_.sections.size()));
  head += pos;
  tr_.textInverted(8, 6, head, tr_.lineHeight() + 6);

  int y = tr_.lineHeight() + 18;
  const auto& lines = sec.lines;
  for (size_t i = static_cast<size_t>(scroll_); i < lines.size(); ++i) {
    if (y + tr_.lineHeight() > tr_.height() - 8) break;
    const auto& ln = lines[i];
    if (ln.kind == 2) {
      tr_.hline(8, y + tr_.lineHeight() / 2, tr_.width() - 16);
    } else if (ln.kind == 1) {
      tr_.textInverted(8, y, ln.text, tr_.lineHeight() + 2);
    } else {
      tr_.text(8, y, ln.text);
    }
    y += tr_.lineHeight();
  }
  tr_.flush(full);
}

void DailyDropApp::renderArchive(bool full) {
  tr_.clear();
  tr_.textInverted(8, 6, "Archive (14 days)  BACK returns", tr_.lineHeight() + 6);
  int y = tr_.lineHeight() + 18;
  if (archive_.empty()) {
    tr_.text(8, y, "No stored digests.");
  }
  for (size_t i = 0; i < archive_.size(); ++i) {
    if (y + tr_.lineHeight() > tr_.height() - 8) break;
    const std::string marker = (static_cast<int>(i) == archiveSel_) ? "> " : "  ";
    tr_.text(8, y, marker + archive_[i]);
    y += tr_.lineHeight();
  }
  tr_.flush(full);
}

void DailyDropApp::renderMessage(const std::string& headline, const std::string& detail) {
  tr_.clear();
  tr_.textInverted(8, 6, headline, tr_.lineHeight() + 6);
  tr_.text(8, tr_.height() / 2 - tr_.lineHeight(), detail);
  tr_.text(8, tr_.height() - 2 * tr_.lineHeight(),
           "SYNC fetches today. SELECT opens the archive.");
  tr_.flush(true);
}

}  // namespace dailydrop

#endif  // ARDUINO
