#include "file_dialog.hpp"

#include <optional>
#include <string>

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

namespace platform::dialog {

std::optional<std::string> open_file(const char*, const char*) {
    return std::nullopt;
}

std::optional<std::string> save_file(const char*, const char*) {
    return std::nullopt;
}

}  // namespace platform::dialog

#endif  // _WIN32
