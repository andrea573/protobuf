#include "google/protobuf/compiler/allowlists/allowlists.h"

#include <fstream>
#include <string>

#include "devtools/build/runtime/get_runfiles_dir.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/allowlists/allowlist.h"

namespace google {
namespace protobuf {
namespace compiler {

namespace {
inline constexpr absl::string_view kAllowlistPathPrefix =
    "third_party/protobuf/compiler/allowlists/";

#ifndef SWIG
using AllowlistMap = absl::flat_hash_map<std::string, internal::AllowlistInfo>;
#endif

absl::flat_hash_set<std::string> GetContents(absl::string_view path) {
  std::string full_path =
      absl::StrCat(devtools_build::GetRunfilesDir(), "/google3/", path);
  std::ifstream ifstream(full_path);
  absl::flat_hash_set<std::string> contents;
  if (!ifstream) {
    ifstream = std::ifstream(path);
  }
  if (ifstream) {
    std::string line;
    while (std::getline(ifstream, line)) {
      if (!absl::StartsWith(line, "//")) {
        contents.insert(line);
      }
    }
  }
  return contents;
}

void LoadAllowlist(absl::string_view allowlist_path, AllowlistMap* map,
                   internal::AllowlistFlags flag) {
  std::string local_path =
      absl::StrCat(kAllowlistPathPrefix, allowlist_path, ".txt");
  map->emplace(allowlist_path,
               internal::AllowlistInfo(GetContents(local_path), flag));
}

void LoadAllowlist(absl::string_view allowlist_path, AllowlistMap* map) {
  LoadAllowlist(allowlist_path, map, internal::AllowlistFlags::kNone);
}

AllowlistMap LoadAllowlists() {
  static const auto map = [] {
    auto* m = new AllowlistMap();
    LoadAllowlist("weak_imports", m);
    LoadAllowlist("test_allowlist_empty_allow_all", m,
                  internal::AllowlistFlags::kAllowAllWhenEmpty);
    LoadAllowlist("test_allowlist_empty_allow_none", m);
    LoadAllowlist("test_allowlist", m);
    return m;
  }();
  return *map;
}
}  // namespace

bool IsAllowlisted(absl::string_view allowlist, absl::string_view file) {
  const auto map = LoadAllowlists();
  ABSL_CHECK(map.contains(allowlist));
  const auto info = map.at(allowlist);
  if (!info.empty()) {
    return info.IsAllowlisted(file);
  }
  return info.flag() == internal::AllowlistFlags::kAllowAllWhenEmpty;
}
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
