#include "file_dialog_detail.hpp"

#include <string>
#include <vector>

namespace {

std::string escape_shell(const char* text) {
    std::string escaped = "\"";
    if (text) {
        for (const char* p = text; *p; ++p) {
            if (*p == '"' || *p == '\\' || *p == '$' || *p == '`') {
                escaped.push_back('\\');
            }
            escaped.push_back(*p);
        }
    }
    escaped.push_back('"');
    return escaped;
}

}  // namespace

namespace platform::dialog::detail {

std::string build_command_exists_check(const char* command) {
    std::string check = "command -v ";
    check += command ? command : "";
    check += " >/dev/null 2>&1";
    return check;
}

std::string build_applescript_command(const std::string& script) {
    std::string command = "osascript -e \"";
    command += script;
    command += "\"";
    return command;
}

std::string build_zenity_command(bool save_mode, const char* title, const std::vector<std::string>& patterns) {
    std::string command = "zenity --file-selection";
    if (save_mode) {
        command += " --save";
    }
    if (title && *title) {
        command += " --title=";
        command += escape_shell(title);
    }
    if (!patterns.empty()) {
        command += " --file-filter=\"Files |";
        for (const std::string& pattern : patterns) {
            command.push_back(' ');
            command += pattern;
        }
        command += "\"";
    }
    return command;
}

std::string build_kdialog_command(bool save_mode, const char* title, const std::vector<std::string>& patterns) {
    std::string command = "kdialog ";
    command += save_mode ? "--getsavefilename " : "--getopenfilename ";
    command += "\"\"";

    if (!patterns.empty()) {
        command += " \"";
        for (size_t i = 0; i < patterns.size(); ++i) {
            if (i != 0) {
                command.push_back(' ');
            }
            command += patterns[i];
        }
        command += "\"";
    }
    if (title && *title) {
        command += " --title ";
        command += escape_shell(title);
    }
    return command;
}

}  // namespace platform::dialog::detail

