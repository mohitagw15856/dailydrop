// Host unit tests for the portable DailyDrop core: the .drop parser and the
// archive prune/list policy. Plain asserts, no framework, mirrors the other
// ecosystem apps' host suites.
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "core/Archive.h"
#include "core/DropDoc.h"

using namespace dailydrop;

namespace {

int g_checks = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    ++g_checks;                                                        \
    if (!(cond)) {                                                     \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      return 1;                                                        \
    }                                                                  \
  } while (0)

struct CollectingSink : DropSink {
  std::vector<DropEvent> events;
  void onEvent(const DropEvent& e) override { events.push_back(e); }
};

int testParserHappyPath() {
  CollectingSink sink;
  DropParser p(sink);
  CHECK(p.feedLine("DROP 1"));
  CHECK(p.feedLine("M date 2026-07-31"));
  CHECK(p.feedLine("M title Thursday briefing"));
  CHECK(p.feedLine("S News"));
  CHECK(p.feedLine("H Kernel 7.0 released"));
  CHECK(p.feedLine("T The long-awaited release brings"));
  CHECK(p.feedLine("T scheduler improvements."));
  CHECK(p.feedLine("R"));
  CHECK(p.feedLine("S Weather"));
  CHECK(p.feedLine("T Sunny, 24C."));
  CHECK(p.sawMagic());
  CHECK(p.sectionCount() == 2);
  CHECK(sink.events.size() == 9);
  CHECK(sink.events[0].kind == DropEvent::Kind::Meta);
  CHECK(sink.events[0].key == "date");
  CHECK(sink.events[0].text == "2026-07-31");
  CHECK(sink.events[2].kind == DropEvent::Kind::Section);
  CHECK(sink.events[2].text == "News");
  CHECK(sink.events[3].kind == DropEvent::Kind::Heading);
  CHECK(sink.events[6].kind == DropEvent::Kind::Rule);
  return 0;
}

int testParserRejectsBadMagic() {
  CollectingSink sink;
  DropParser p(sink);
  CHECK(!p.feedLine("DROP 2"));
  CHECK(!p.sawMagic());
  CollectingSink sink2;
  DropParser p2(sink2);
  CHECK(!p2.feedLine("hello"));
  return 0;
}

int testParserToleratesUnknownTagsAndBlanks() {
  CollectingSink sink;
  DropParser p(sink);
  CHECK(p.feedLine("DROP 1"));
  CHECK(p.feedLine(""));
  CHECK(p.feedLine("X future tag"));
  CHECK(p.feedLine("S Quote"));
  CHECK(p.feedLine("T Fortune favours the brave."));
  CHECK(sink.events.size() == 2);
  return 0;
}

int testFilenames() {
  CHECK(isDropFilename("2026-07-31.drop"));
  CHECK(!isDropFilename("2026-7-31.drop"));
  CHECK(!isDropFilename("2026-07-31.drop.tmp"));
  CHECK(!isDropFilename("notes.txt"));
  CHECK(!isDropFilename("20260731.drop"));
  CHECK(dropDate("/dailydrop/2026-07-31.drop") == "2026-07-31");
  CHECK(dropDate("2025-01-02.drop") == "2025-01-02");
  CHECK(dropDate("/dailydrop/config.txt").empty());
  CHECK(dropPathFor("2026-07-31") == "/dailydrop/2026-07-31.drop");
  return 0;
}

int testPrune() {
  std::vector<std::string> names;
  for (int d = 1; d <= 20; ++d) {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "2026-07-%02d.drop", d);
    names.push_back(buf);
  }
  names.push_back("config.txt");  // ignored
  auto doomed = pruneList(names, 14);
  CHECK(doomed.size() == 6);
  // The oldest six go; the list is newest-first ordering's tail.
  CHECK(doomed.front() == "2026-07-06.drop");
  CHECK(doomed.back() == "2026-07-01.drop");

  auto shown = archiveList(names, 14);
  CHECK(shown.size() == 14);
  CHECK(shown.front() == "2026-07-20.drop");
  CHECK(shown.back() == "2026-07-07.drop");

  CHECK(pruneList({"2026-07-01.drop"}, 14).empty());
  CHECK(pruneList({}, 14).empty());
  return 0;
}

}  // namespace

int main() {
  if (testParserHappyPath()) return 1;
  if (testParserRejectsBadMagic()) return 1;
  if (testParserToleratesUnknownTagsAndBlanks()) return 1;
  if (testFilenames()) return 1;
  if (testPrune()) return 1;
  std::printf("%d checks, 0 failures\n", g_checks);
  return 0;
}
