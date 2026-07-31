// Wi-Fi digest fetcher: connects to the configured network, GETs today's
// digest and streams it to the SD card. All of this is device-only and marked
// TODO(hardware-test); the decision logic (URL building, failure taxonomy) is
// kept small and explicit so failure states render honestly on screen.
//
// Config file (/dailydrop/config.txt), one "key value" per line:
//   ssid <network>
//   pass <password>
//   url  <base url, e.g. https://example.com/digests or a GitHub raw prefix>
//
// The fetch URL is <url>/<YYYY-MM-DD>.drop.
#pragma once

#ifdef ARDUINO

#include <string>

namespace dailydrop {

enum class FetchResult : uint8_t {
  Ok,           // digest downloaded and stored
  NoConfig,     // config.txt missing or incomplete
  NoWifi,       // could not associate/get an IP
  NoDigest,     // server reachable but no digest for that date (404)
  HttpError,    // any other HTTP failure
  StorageError, // SD write failed
  Partial,      // connection dropped mid-body; partial file removed
};

const char* fetchResultLabel(FetchResult r);

class Fetcher {
 public:
  // Loads config.txt. Returns false when the file is missing/incomplete.
  bool loadConfig();

  // Blocking fetch of the digest for `isoDate` into dropPathFor(isoDate).
  // TODO(hardware-test): Wi-Fi association, TLS-less HTTP fetch and the
  // partial-download cleanup path all need a real device and network.
  FetchResult fetch(const std::string& isoDate);

  const std::string& url() const { return url_; }

 private:
  std::string ssid_;
  std::string pass_;
  std::string url_;
};

}  // namespace dailydrop

#endif  // ARDUINO
