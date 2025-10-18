#include "file_dialog.hpp"
#include "file_dialog_detail.hpp"

#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

namespace {

std::wstring utf8_to_wide(const std::string& input) {
    if (input.empty())
        return {};
    const int size_needed = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (size_needed <= 0)
        return {};
    std::wstring result(static_cast<size_t>(size_needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, result.data(), size_needed);
    // Remove potential trailing null inserted by MultiByteToWideChar.
    if (!result.empty() && result.back() == L'\0') {
        result.pop_back();
    }
    return result;
}

std::string wide_to_utf8(const std::wstring& input) {
    if (input.empty())
        return {};
    const int size_needed = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
        return {};
    std::string result(static_cast<size_t>(size_needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, result.data(), size_needed, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    return result;
}

std::wstring build_filter(const char* filter) {
    if (!filter || !*filter) {
        return std::wstring(L"All Files (*.*)\0*.*\0\0", 22);
    }

    const std::wstring wide = utf8_to_wide(filter);
    if (wide.empty())
        return std::wstring(L"All Files (*.*)\0*.*\0\0", 22);

    std::wstring formatted;
    formatted.reserve(wide.size() + 2);
    for (wchar_t ch : wide) {
        if (ch == L'|') {
            formatted.push_back(L'\0');
        } else {
            formatted.push_back(ch);
        }
    }
    if (formatted.empty() || formatted.back() != L'\0') {
        formatted.push_back(L'\0');
    }
    formatted.push_back(L'\0');
    return formatted;
}

std::optional<std::string> invoke_dialog(bool save_mode, const char* title, const char* filter) {
    wchar_t buffer[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
    const std::wstring wide_title = title ? utf8_to_wide(title) : std::wstring();
    ofn.lpstrTitle = wide_title.empty() ? nullptr : wide_title.c_str();
    const std::wstring wide_filter = build_filter(filter);
    ofn.lpstrFilter = wide_filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;

    if (!save_mode) {
        ofn.Flags |= OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameW(&ofn) == TRUE) {
            return wide_to_utf8(buffer);
        }
    } else {
        ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        if (GetSaveFileNameW(&ofn) == TRUE) {
            return wide_to_utf8(buffer);
        }
    }
    return std::nullopt;
}

}  // namespace

namespace platform::dialog {

std::optional<std::string> open_file(const char* title, const char* filter) {
    return invoke_dialog(false, title, filter);
}

std::optional<std::string> save_file(const char* title, const char* filter) {
    return invoke_dialog(true, title, filter);
}

}  // namespace platform::dialog

#else  // _WIN32

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace {

std::vector<std::string> split_filter_tokens(const char* filter) {
    std::vector<std::string> tokens;
    if (!filter || !*filter)
        return tokens;

    std::string current;
    for (const char* p = filter; *p; ++p) {
        if (*p == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(*p);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::vector<std::string> extract_filter_patterns(const char* filter) {
    const std::vector<std::string> tokens = split_filter_tokens(filter);

    std::vector<std::string> patterns;
    bool expectPattern = false;
    for (const std::string& token : tokens) {
        if (token.empty())
            continue;
        if (expectPattern || token.find('*') != std::string::npos || token.find('?') != std::string::npos) {
            patterns.push_back(token);
        }
        expectPattern = !expectPattern;
    }

    if (patterns.empty()) {
        patterns.emplace_back("*");
    }
    return patterns;
}

}  // namespace

#if defined(__APPLE__)

std::string escape_applescript(const std::string& input) {
    std::string escaped;
    for (char c : input) {
        if (c == '"') {
            escaped += "\\\"";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else {
            escaped.push_back(c);
        }
    }
    return escaped;
}

std::optional<std::string> run_applescript(const std::string& script) {
    const std::string command = platform::dialog::detail::build_applescript_command(script);

    std::array<char, 512> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
        return std::nullopt;

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get())) {
        result.append(buffer.data());
    }
    const int exit_code = pclose(pipe.release());
    if (exit_code != 0 || result.empty())
        return std::nullopt;

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result.empty() ? std::nullopt : std::optional<std::string>(result);
}

std::vector<std::string> extract_extensions(const std::vector<std::string>& patterns) {
    std::vector<std::string> extensions;
    for (const std::string& pattern : patterns) {
        if (pattern.size() > 2 && pattern[0] == '*' && pattern[1] == '.') {
            std::string ext = pattern.substr(2);
            if (ext.find_first_of("*?") == std::string::npos) {
                extensions.push_back(ext);
            }
        }
    }
    return extensions;
}

std::optional<std::string> show_cocoa_dialog(bool save_mode, const char* title, const char* filter) {
    const std::vector<std::string> patterns = extract_filter_patterns(filter);
    const std::vector<std::string> extensions = extract_extensions(patterns);

    std::string script;
    if (save_mode) {
        script = "set chosenFile to choose file name";
    } else {
        script = "set chosenFile to choose file";
    }
    if (title && *title) {
        script += " with prompt \"" + escape_applescript(title) + "\"";
    }
    if (!extensions.empty()) {
        script += " of type {";
        for (size_t i = 0; i < extensions.size(); ++i) {
            if (i != 0) {
                script += ", ";
            }
            script += "\"" + extensions[i] + "\"";
        }
        script += "}";
    }
    script += "\nPOSIX path of chosenFile";

    return run_applescript(script);
}

#else  // __APPLE__

bool command_exists(const char* command) {
    const std::string check = platform::dialog::detail::build_command_exists_check(command);
    const int rc = std::system(check.c_str());
    return rc == 0;
}

std::optional<std::string> run_command_capture(const std::string& command) {
    std::array<char, 512> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
        return std::nullopt;

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get())) {
        result.append(buffer.data());
    }
    const int exit_code = pclose(pipe.release());
    if (exit_code != 0 || result.empty())
        return std::nullopt;

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result.empty() ? std::nullopt : std::optional<std::string>(result);
}

std::optional<std::string> show_zenity_dialog(bool save_mode, const char* title, const char* filter) {
    if (!command_exists("zenity"))
        return std::nullopt;

    const std::vector<std::string> patterns = extract_filter_patterns(filter);
    const std::string command = platform::dialog::detail::build_zenity_command(save_mode, title, patterns);

    return run_command_capture(command);
}

std::optional<std::string> show_kdialog_dialog(bool save_mode, const char* title, const char* filter) {
    if (!command_exists("kdialog"))
        return std::nullopt;

    const std::vector<std::string> patterns = extract_filter_patterns(filter);
    const std::string command = platform::dialog::detail::build_kdialog_command(save_mode, title, patterns);

    return run_command_capture(command);
}

#endif  // __APPLE__

std::optional<std::string> run_dialog(bool save_mode, const char* title, const char* filter) {
#if defined(__APPLE__)
    return show_cocoa_dialog(save_mode, title, filter);
#else
    if (auto zenity = show_zenity_dialog(save_mode, title, filter)) {
        return zenity;
    }
    if (auto kdialog = show_kdialog_dialog(save_mode, title, filter)) {
        return kdialog;
    }
    return std::nullopt;
#endif
}

}  // namespace

namespace platform::dialog {

std::optional<std::string> open_file(const char* title, const char* filter) {
    return run_dialog(false, title, filter);
}

std::optional<std::string> save_file(const char* title, const char* filter) {
    return run_dialog(true, title, filter);
}

}  // namespace platform::dialog

#endif  // _WIN32
