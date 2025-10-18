#pragma once

#include <string>
#include <vector>

namespace platform::dialog::detail {

// Builds the shell command used to check for an executable's presence.
std::string build_command_exists_check(const char* command);

// Builds the osascript invocation used on macOS to run an AppleScript snippet.
std::string build_applescript_command(const std::string& script);

// Builds the zenity command used on Linux/Unix platforms.
std::string build_zenity_command(bool save_mode, const char* title, const std::vector<std::string>& patterns);

// Builds the kdialog command used on Linux/Unix platforms.
std::string build_kdialog_command(bool save_mode, const char* title, const std::vector<std::string>& patterns);

}  // namespace platform::dialog::detail

