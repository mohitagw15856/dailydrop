#include "device/Fetcher.h"

#ifdef ARDUINO

#include <HTTPClient.h>
#include <WiFi.h>

#include <inkkit/SdStream.h>
#include <inkkit/Storage.h>

#include "core/Archive.h"
#include "device/Config.h"

namespace dailydrop {

const char* fetchResultLabel(FetchResult r) {
  switch (r) {
    case FetchResult::Ok: return "Digest updated";
    case FetchResult::NoConfig: return "No sync config (see config.txt)";
    case FetchResult::NoWifi: return "No Wi-Fi connection";
    case FetchResult::NoDigest: return "No new digest yet";
    case FetchResult::HttpError: return "Server error";
    case FetchResult::StorageError: return "SD card write failed";
    case FetchResult::Partial: return "Download interrupted";
  }
  return "Unknown";
}

bool Fetcher::loadConfig() {
  std::string text;
  if (!inkkit::sd::readWholeFile(kConfigPath, text)) return false;
  ssid_.clear();
  pass_.clear();
  url_.clear();
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find('\n', pos);
    if (end == std::string::npos) end = text.size();
    std::string line = text.substr(pos, end - pos);
    pos = end + 1;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const size_t sp = line.find(' ');
    if (sp == std::string::npos) continue;
    const std::string key = line.substr(0, sp);
    const std::string value = line.substr(sp + 1);
    if (key == "ssid") ssid_ = value;
    else if (key == "pass") pass_ = value;
    else if (key == "url") url_ = value;
  }
  return !ssid_.empty() && !url_.empty();
}

FetchResult Fetcher::fetch(const std::string& isoDate) {
  if (ssid_.empty() || url_.empty()) return FetchResult::NoConfig;

  // TODO(hardware-test): association timing, RSSI edge cases, captive portals.
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_.c_str(), pass_.c_str());
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 15000) {
      WiFi.mode(WIFI_OFF);
      return FetchResult::NoWifi;
    }
    delay(100);
  }

  const std::string fetchUrl = url_ + "/" + isoDate + ".drop";
  HTTPClient http;
  http.setTimeout(10000);
  FetchResult result = FetchResult::HttpError;

  if (http.begin(fetchUrl.c_str())) {
    const int code = http.GET();
    if (code == 404) {
      result = FetchResult::NoDigest;
    } else if (code == 200) {
      const std::string path = dropPathFor(isoDate);
      const std::string tmp = path + ".tmp";
      inkkit::sd::ensureDir(kAppRoot);
      HalFile out;
      if (!inkkit::sd::openWrite(kTag, tmp.c_str(), out)) {
        result = FetchResult::StorageError;
      } else {
        // Stream the body in small chunks; the digest is pre-wrapped text so
        // a 1 KB buffer keeps RAM pressure negligible.
        WiFiClient* stream = http.getStreamPtr();
        int remaining = http.getSize();  // -1 when chunked
        uint8_t buf[1024];
        bool ioError = false;
        while (http.connected() && (remaining > 0 || remaining == -1)) {
          const size_t avail = stream->available();
          if (avail == 0) {
            delay(10);
            if (!http.connected()) break;
            continue;
          }
          const int n = stream->readBytes(buf, avail > sizeof(buf) ? sizeof(buf) : avail);
          if (n <= 0) break;
          if (out.write(buf, static_cast<size_t>(n)) != static_cast<size_t>(n)) {
            ioError = true;
            break;
          }
          if (remaining > 0) remaining -= n;
          if (remaining == 0) break;
        }
        out.close();
        const bool complete = !ioError && (remaining == 0 || remaining == -1);
        if (ioError) {
          Storage.remove(tmp.c_str());
          result = FetchResult::StorageError;
        } else if (!complete) {
          // TODO(hardware-test): verify partial bodies are detected and the
          // temp file removed when Wi-Fi drops mid-transfer.
          Storage.remove(tmp.c_str());
          result = FetchResult::Partial;
        } else {
          Storage.remove(path.c_str());
          if (Storage.rename(tmp.c_str(), path.c_str())) {
            result = FetchResult::Ok;
          } else {
            result = FetchResult::StorageError;
          }
        }
      }
    }
    http.end();
  }

  WiFi.mode(WIFI_OFF);
  return result;
}

}  // namespace dailydrop

#endif  // ARDUINO
