#include "file_dialog_detail.hpp"

#include <string>
#include <vector>

namespace {

int expect_equal(const std::string& actual, const std::string& expected) {
    if (actual == expected) {
        return 0;
    }
    return 1;
}

}  // namespace

int main() {
    using namespace platform::dialog::detail;

    if (expect_equal(build_command_exists_check("zenity"),
                     "command -v zenity >/dev/null 2>&1")) {
        return 1;
    }

    const std::string script = "return \"Hello\"";
    if (expect_equal(build_applescript_command(script),
                     "osascript -e \"" + script + "\"")) {
        return 1;
    }

    const std::vector<std::string> patterns{"*.txt", "*.md"};
    if (expect_equal(build_zenity_command(true, "Save File", patterns),
                     "zenity --file-selection --save --title=\"Save File\" --file-filter=\"Files | *.txt *.md\"")) {
        return 1;
    }

    if (expect_equal(build_kdialog_command(false, "Open File", patterns),
                     "kdialog --getopenfilename \"\" \"*.txt *.md\" --title \"Open File\"")) {
        return 1;
    }

    return 0;
}

