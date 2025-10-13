#pragma once

#include <optional>
#include <string>

namespace platform::dialog {

// Opens a native file picker dialog in "Open" mode.
// Returns the selected path in UTF-8 when the user confirms; std::nullopt on cancel or failure.
std::optional<std::string> open_file(const char* title = nullptr, const char* filter = nullptr);

// Opens a native file picker dialog in "Save" mode.
// Returns the selected path in UTF-8 when the user confirms; std::nullopt on cancel or failure.
std::optional<std::string> save_file(const char* title = nullptr, const char* filter = nullptr);

}  // namespace platform::dialog

