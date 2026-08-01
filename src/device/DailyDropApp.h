// Screen state machine: today's digest (section per screen), the 14-day
// archive browser, and sync status messages.
#pragma once

#ifdef ARDUINO

#include <string>
#include <vector>

#include <inkkit/Buttons.h>
#include <inkkit/Display.h>

#include "core/DropDoc.h"
#include "device/Fetcher.h"
#include "device/TextRenderer.h"

namespace dailydrop {

// A digest loaded for display. Digests are companion-capped (docs/FORMAT.md)
// so whole-document line storage stays within a few kilobytes.
struct LoadedDrop {
  std::string date;
  std::string title;
  struct Line {
    uint8_t kind;  // 0 text, 1 heading, 2 rule
    std::string text;
  };
  struct Section {
    std::string name;
    std::vector<Line> lines;
  };
  std::vector<Section> sections;
};

class DailyDropApp : public DropSink {
 public:
  DailyDropApp(TextRenderer& tr, inkkit::Buttons& buttons, Fetcher& fetcher)
      : tr_(tr), buttons_(buttons), fetcher_(fetcher) {}

  void begin();
  void tick();

  // DropSink: receives events while a digest file is being parsed.
  void onEvent(const DropEvent& e) override;

 private:
  enum class Screen : uint8_t { Reader, Archive, Message };

  bool loadDrop(const std::string& path);
  void pruneArchive();
  void refreshArchiveNames();
  void doSync();

  void renderReader(bool full);
  void renderArchive(bool full);
  void renderMessage(const std::string& headline, const std::string& detail);

  TextRenderer& tr_;
  inkkit::Buttons& buttons_;
  Fetcher& fetcher_;

  Screen screen_ = Screen::Message;
  LoadedDrop drop_;
  int section_ = 0;
  int scroll_ = 0;

  std::vector<std::string> archive_;
  int archiveSel_ = 0;
};

}  // namespace dailydrop

#endif  // ARDUINO
