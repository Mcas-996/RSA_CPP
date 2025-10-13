#include "prepare.hpp"
#include "Iwanna.hpp"
#include "RSA.hpp"
#include "third_party/cppcodec/cppcodec/base64_rfc4648.hpp"
#include "file_dialog.hpp"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <filesystem>
#include <system_error>

#include "misc/cpp/imgui_stdlib.h"

#if defined(_WIN32)
#include <windows.h>
extern LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

#ifndef DEFAULT_IMGUI_INI_CONTENT
static const char DEFAULT_IMGUI_INI_CONTENT[] = R"([Window][Debug##Default]
Pos=60,60
Size=400,400

[Window][Dear ImGui Demo]
Pos=650,20
Size=550,680

[Window][Traditional RSA]
Pos=28,84
Size=320,640

[Window][OpenSSL PEM - Keys]
Pos=360,84
Size=320,260

[Window][OpenSSL PEM - Encrypt]
Pos=360,360
Size=320,220

[Window][OpenSSL PEM - Decrypt]
Pos=360,590
Size=320,220

[Window][OpenSSL PEM - Encrypt File]
Pos=700,84
Size=320,300

[Window][OpenSSL PEM - Decrypt File]
Pos=700,400
Size=320,300

[Window][Result]
Pos=1050,84
Size=220,300

)";
#endif

namespace {
struct PemUiState {
    RSAUtil::PemKeyPair keyPair{};
    bool hasKey = false;
    bool hasPublicKey = false;
    bool hasPrivateKey = false;
    int keyBits = 2048;
    std::string publicKeyPem;
    std::string privateKeyPem;
    std::string publicKeyPath;
    std::string privateKeyPath;
    std::string plaintext;
    std::string ciphertextBase64;
    std::string decryptInput;
    std::string decryptOutput;
    std::string statusMessage;
    std::string encryptStatus;
    std::string decryptStatus;
    std::string encryptInputPath;
    std::string encryptOutputPath;
    std::string decryptInputPath;
    std::string decryptOutputPath;
    std::string encryptFileStatus;
    std::string decryptFileStatus;
    int paddingIndex = 0;
};

constexpr const char* kPaddingLabels[] = {"OAEP (SHA-1)", "PKCS#1 v1.5"};
constexpr int kPaddingValues[] = {RSA_PKCS1_OAEP_PADDING, RSA_PKCS1_PADDING};

std::string encodeBase64(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }
    return cppcodec::base64_rfc4648::encode(data);
}

bool decodeBase64(const std::string& text, std::vector<uint8_t>& out, std::string& errorMessage) {
    try {
        out = cppcodec::base64_rfc4648::decode(text);
        return true;
    } catch (const std::exception& ex) {
        errorMessage = ex.what();
        return false;
    }
}

bool loadTextFile(const std::string& path, std::string& content, std::string& errorMessage) {
    if (path.empty()) {
        errorMessage = "File path cannot be empty";
        return false;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        errorMessage = "Unable to open file: " + path;
        return false;
    }
    content.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return true;
}

bool saveTextFile(const std::string& path, const std::string& content, std::string& errorMessage) {
    if (path.empty()) {
        errorMessage = "File path cannot be empty";
        return false;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        errorMessage = "Unable to write file: " + path;
        return false;
    }
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!output) {
        errorMessage = "File write failed: " + path;
        return false;
    }
    return true;
}

void refreshKeyMetadata(PemUiState& state) {
    state.keyPair.publicKeyPem = state.publicKeyPem;
    state.keyPair.privateKeyPem = state.privateKeyPem;
    if (!state.publicKeyPem.empty()) {
        try {
            state.keyPair.keyBits = RSAUtil::getKeyBitsFromPublicKey(state.publicKeyPem);
        } catch (const std::exception&) {
            state.keyPair.keyBits = 0;
        }
    } else {
        state.keyPair.keyBits = 0;
    }
    state.hasPublicKey = !state.keyPair.publicKeyPem.empty();
    state.hasPrivateKey = !state.keyPair.privateKeyPem.empty();
    state.hasKey = state.hasPublicKey && state.hasPrivateKey;
}

PemUiState& GetPemUiState() {
    static PemUiState state;
    return state;
}

static void DrawPemKeyManagementTab(PemUiState& state) {
    ImGui::TextUnformatted("Manage PEM key pairs for OpenSSL RSA operations");

    int keyBitsInput = state.keyBits;
    if (ImGui::InputInt("Key size (bits)", &keyBitsInput)) {
        keyBitsInput = std::clamp(keyBitsInput, 512, 16384);
        state.keyBits = keyBitsInput;
    }
    if (ImGui::Button("Generate PEM key pair")) {
        try {
            state.keyPair = RSAUtil::generatePemKeyPair(state.keyBits);
            state.publicKeyPem = state.keyPair.publicKeyPem;
            state.privateKeyPem = state.keyPair.privateKeyPem;
            refreshKeyMetadata(state);
            state.statusMessage = "Key pair generated successfully";
        } catch (const std::exception& ex) {
            state.statusMessage = std::string("Generation failed: ") + ex.what();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset state")) {
        state = PemUiState{};
        state.keyBits = 2048;
        state.statusMessage = "State cleared";
    }
    if (!state.statusMessage.empty()) {
        ImGui::TextWrapped("Status: %s", state.statusMessage.c_str());
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Key files");

    ImGui::InputText("Public key path", &state.publicKeyPath);
    ImGui::SameLine();
    if (ImGui::Button("Open...##OpenPubKeyPath")) {
        if (auto p = platform::dialog::open_file("Select public key file", "PEM Files (*.pem)|*.pem|All Files (*.*)|*.*")) {
            state.publicKeyPath = *p;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As...##SavePubKeyPath")) {
        if (auto p = platform::dialog::save_file("Save public key as", "PEM Files (*.pem)|*.pem|All Files (*.*)|*.*")) {
            state.publicKeyPath = *p;
        }
    }
    if (ImGui::Button("Load public key from file")) {
        std::string error;
        if (loadTextFile(state.publicKeyPath, state.publicKeyPem, error)) {
            refreshKeyMetadata(state);
            state.statusMessage = "Public key loaded";
        } else {
            state.statusMessage = error;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save public key to file")) {
        std::string error;
        if (saveTextFile(state.publicKeyPath, state.publicKeyPem, error)) {
            state.statusMessage = "Public key saved";
        } else {
            state.statusMessage = error;
        }
    }

    ImGui::InputText("Private key path", &state.privateKeyPath);
    ImGui::SameLine();
    if (ImGui::Button("Open...##OpenPrivKeyPath")) {
        if (auto p = platform::dialog::open_file("Select private key file", "PEM Files (*.pem)|*.pem|All Files (*.*)|*.*")) {
            state.privateKeyPath = *p;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As...##SavePrivKeyPath")) {
        if (auto p = platform::dialog::save_file("Save private key as", "PEM Files (*.pem)|*.pem|All Files (*.*)|*.*")) {
            state.privateKeyPath = *p;
        }
    }
    if (ImGui::Button("Load private key from file")) {
        std::string error;
        if (loadTextFile(state.privateKeyPath, state.privateKeyPem, error)) {
            refreshKeyMetadata(state);
            state.statusMessage = "Private key loaded";
        } else {
            state.statusMessage = error;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save private key to file")) {
        std::string error;
        if (saveTextFile(state.privateKeyPath, state.privateKeyPem, error)) {
            state.statusMessage = "Private key saved";
        } else {
            state.statusMessage = error;
        }
    }

    const bool publicChanged = ImGui::InputTextMultiline("Public key PEM", &state.publicKeyPem, ImVec2(-FLT_MIN, 120.0f));
    const bool privateChanged = ImGui::InputTextMultiline("Private key PEM", &state.privateKeyPem, ImVec2(-FLT_MIN, 120.0f));
    if (publicChanged || privateChanged) {
        refreshKeyMetadata(state);
    }

    if (state.keyPair.keyBits > 0) {
        ImGui::Text("Detected key length: %d bits", state.keyPair.keyBits);
    }
}

static void DrawPemEncryptionTab(PemUiState& state) {
    ImGui::TextUnformatted("Encrypt plaintext with the loaded public key");

    ImGui::Combo("Padding##PemEncrypt", &state.paddingIndex, kPaddingLabels, IM_ARRAYSIZE(kPaddingLabels));

    ImGui::TextUnformatted("Plaintext");
    ImGui::SameLine();
    if (ImGui::Button("Get from result##PemPlaintext")) {
        state.plaintext = Myspace::resultPrimary;
    }
    ImGui::InputTextMultiline("##PemPlaintext", &state.plaintext, ImVec2(-FLT_MIN, 120.0f));

    if (ImGui::Button("Encrypt with public key")) {
        if (!state.hasPublicKey) {
            state.encryptStatus = "Provide a valid public key first";
        } else {
            try {
                const int padding = kPaddingValues[state.paddingIndex];
                const std::vector<uint8_t> encrypted = RSAUtil::encryptTextToBytes(state.plaintext, state.keyPair, padding);
                state.ciphertextBase64 = encodeBase64(encrypted);
                state.encryptStatus = "Encryption succeeded";
            } catch (const std::exception& ex) {
                state.encryptStatus = std::string("Encryption failed: ") + ex.what();
            }
        }
    }
    if (!state.encryptStatus.empty()) {
        ImGui::TextWrapped("Encrypt status: %s", state.encryptStatus.c_str());
    }
    ImGui::InputTextMultiline("Base64 ciphertext", &state.ciphertextBase64, ImVec2(-FLT_MIN, 120.0f));
    if (ImGui::Button("Push ciphertext to result")) {
        Myspace::resultPrimary = state.ciphertextBase64;
    }
}

static void DrawPemDecryptionTab(PemUiState& state) {
    ImGui::TextUnformatted("Decrypt Base64 ciphertext with the loaded private key");

    ImGui::Combo("Padding##PemDecrypt", &state.paddingIndex, kPaddingLabels, IM_ARRAYSIZE(kPaddingLabels));

    ImGui::TextUnformatted("Base64 input");
    ImGui::SameLine();
    if (ImGui::Button("Get from result##PemBase64Input")) {
        state.decryptInput = Myspace::resultPrimary;
    }
    ImGui::InputTextMultiline("##PemBase64Input", &state.decryptInput, ImVec2(-FLT_MIN, 120.0f));

    if (ImGui::Button("Decrypt with private key")) {
        if (!state.hasPrivateKey) {
            state.decryptStatus = "Provide a valid private key first";
        } else {
            std::vector<uint8_t> cipherBytes;
            std::string error;
            if (!decodeBase64(state.decryptInput, cipherBytes, error)) {
                state.decryptStatus = std::string("Base64 decode failed: ") + error;
            } else {
                try {
                    const int padding = kPaddingValues[state.paddingIndex];
                    state.decryptOutput = RSAUtil::decryptTextFromBytes(cipherBytes, state.keyPair, padding);
                    state.decryptStatus = "Decryption succeeded";
                } catch (const std::exception& ex) {
                    state.decryptStatus = std::string("Decryption failed: ") + ex.what();
                }
            }
        }
    }
    if (!state.decryptStatus.empty()) {
        ImGui::TextWrapped("Decrypt status: %s", state.decryptStatus.c_str());
    }
    ImGui::InputTextMultiline("Plaintext output", &state.decryptOutput, ImVec2(-FLT_MIN, 120.0f), ImGuiInputTextFlags_ReadOnly);
    if (ImGui::Button("Push plaintext to result")) {
        Myspace::resultPrimary = state.decryptOutput;
    }
}

static void DrawPemFileEncryptionTab(PemUiState& state) {
    ImGui::TextWrapped("Encrypt a source file with the loaded public key and write Base64 ciphertext to disk.");

    ImGui::Combo("Padding##PemFileEncrypt", &state.paddingIndex, kPaddingLabels, IM_ARRAYSIZE(kPaddingLabels));

    ImGui::InputText("Input file path", &state.encryptInputPath);
    ImGui::SameLine();
    if (ImGui::Button("Browse...##PemEncInput")) {
        if (auto p = platform::dialog::open_file("Select file to encrypt", "All Files (*.*)|*.*")) {
            state.encryptInputPath = *p;
        }
    }
    ImGui::InputText("Output Base64 path", &state.encryptOutputPath);
    ImGui::SameLine();
    if (ImGui::Button("Browse...##PemEncOutput")) {
        if (auto p = platform::dialog::save_file("Select Base64 output path", "Text Files (*.txt)|*.txt|All Files (*.*)|*.*")) {
            state.encryptOutputPath = *p;
        }
    }

    if (ImGui::Button("Encrypt file")) {
        if (!state.hasPublicKey) {
            state.encryptFileStatus = "Load or generate a public key first.";
        } else if (state.encryptInputPath.empty()) {
            state.encryptFileStatus = "Provide the source file path.";
        } else if (state.encryptOutputPath.empty()) {
            state.encryptFileStatus = "Provide the output file path.";
        } else {
            const std::string inputPath = Myspace::detail::normalizePath(state.encryptInputPath);
            const std::string outputPath = Myspace::detail::normalizePath(state.encryptOutputPath);
            if (inputPath != state.encryptInputPath) {
                state.encryptInputPath = inputPath;
            }
            if (outputPath != state.encryptOutputPath) {
                state.encryptOutputPath = outputPath;
            }
            if (inputPath.empty()) {
                state.encryptFileStatus = "Provide the source file path.";
                return;
            }
            if (outputPath.empty()) {
                state.encryptFileStatus = "Provide the output file path.";
                return;
            }
            std::string fileContent;
            std::string error;
            if (!loadTextFile(inputPath, fileContent, error)) {
                state.encryptFileStatus = error;
            } else {
                try {
                    const int padding = kPaddingValues[state.paddingIndex];
                    const std::vector<uint8_t> encrypted = RSAUtil::encryptTextToBytes(fileContent, state.keyPair, padding);
                    const std::string base64 = encodeBase64(encrypted);
                    state.ciphertextBase64 = base64;
                    state.encryptStatus = "Encryption succeeded (from file)";
                    if (saveTextFile(outputPath, base64, error)) {
                        state.encryptFileStatus = "File encrypted and Base64 written to output path.";
                    } else {
                        state.encryptFileStatus = error;
                    }
                } catch (const std::exception& ex) {
                    state.encryptFileStatus = std::string("File encryption failed: ") + ex.what();
                }
            }
        }
    }
    if (!state.encryptFileStatus.empty()) {
        ImGui::TextWrapped("Status: %s", state.encryptFileStatus.c_str());
    }
}
static void DrawPemFileDecryptionTab(PemUiState& state) {
    ImGui::TextWrapped("Decode a Base64 ciphertext file with the loaded private key and save the decrypted bytes.");

    ImGui::Combo("Padding##PemFileDecrypt", &state.paddingIndex, kPaddingLabels, IM_ARRAYSIZE(kPaddingLabels));

    ImGui::InputText("Base64 input path", &state.decryptInputPath);
    ImGui::SameLine();
    if (ImGui::Button("Browse...##PemDecInput")) {
        if (auto p = platform::dialog::open_file("Select Base64 ciphertext file", "Text Files (*.txt)|*.txt|All Files (*.*)|*.*")) {
            state.decryptInputPath = *p;
        }
    }
    ImGui::InputText("Plaintext output path", &state.decryptOutputPath);
    ImGui::SameLine();
    if (ImGui::Button("Browse...##PemDecOutput")) {
        if (auto p = platform::dialog::save_file("Select plaintext output path", "All Files (*.*)|*.*")) {
            state.decryptOutputPath = *p;
        }
    }

    if (ImGui::Button("Decrypt file")) {
        if (!state.hasPrivateKey) {
            state.decryptFileStatus = "Load or generate a private key first.";
        } else if (state.decryptInputPath.empty()) {
            state.decryptFileStatus = "Provide the Base64 ciphertext file path.";
        } else if (state.decryptOutputPath.empty()) {
            state.decryptFileStatus = "Provide the output file path.";
        } else {
            const std::string inputPath = Myspace::detail::normalizePath(state.decryptInputPath);
            const std::string outputPath = Myspace::detail::normalizePath(state.decryptOutputPath);
            if (inputPath != state.decryptInputPath) {
                state.decryptInputPath = inputPath;
            }
            if (outputPath != state.decryptOutputPath) {
                state.decryptOutputPath = outputPath;
            }
            if (inputPath.empty()) {
                state.decryptFileStatus = "Provide the Base64 ciphertext file path.";
                return;
            }
            if (outputPath.empty()) {
                state.decryptFileStatus = "Provide the output file path.";
                return;
            }
            std::string base64Content;
            std::string error;
            if (!loadTextFile(inputPath, base64Content, error)) {
                state.decryptFileStatus = error;
            } else {
                std::vector<uint8_t> cipherBytes;
                std::string decodeError;
                if (!decodeBase64(base64Content, cipherBytes, decodeError)) {
                    state.decryptFileStatus = std::string("Base64 decode failed: ") + decodeError;
                } else {
                    try {
                    const int padding = kPaddingValues[state.paddingIndex];
                        const std::string decrypted = RSAUtil::decryptTextFromBytes(cipherBytes, state.keyPair, padding);
                        state.decryptOutput = decrypted;
                        state.decryptStatus = "Decryption succeeded (from file)";
                        if (saveTextFile(outputPath, decrypted, error)) {
                            state.decryptFileStatus = "File decrypted and plaintext written to output path.";
                        } else {
                            state.decryptFileStatus = error;
                        }
                    } catch (const std::exception& ex) {
                        state.decryptFileStatus = std::string("File decryption failed: ") + ex.what();
                    }
                }
            }
        }
    }
    if (!state.decryptFileStatus.empty()) {
        ImGui::TextWrapped("Status: %s", state.decryptFileStatus.c_str());
    }
}

void ShowOpenSslPemWindow() {
    PemUiState& state = GetPemUiState();
    if (!ImGui::Begin("OpenSSL PEM")) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("OpenSslPemTabs")) {
        if (ImGui::BeginTabItem("Keys")) {
            DrawPemKeyManagementTab(state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Encrypt")) {
            DrawPemEncryptionTab(state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Decrypt")) {
            DrawPemDecryptionTab(state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Encrypt File")) {
            DrawPemFileEncryptionTab(state);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Decrypt File")) {
            DrawPemFileDecryptionTab(state);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
} // namespace

static void EnsureDefaultImGuiIni(ImGuiIO& io) {
    if (!io.IniFilename) {
        return;
    }
    std::ifstream ini_file(io.IniFilename);
    if (!ini_file.good()) {
        ImGui::LoadIniSettingsFromMemory(DEFAULT_IMGUI_INI_CONTENT);
        ImGui::SaveIniSettingsToDisk(io.IniFilename);
    }
}

static ImFont* LoadFontFromFile(const std::filesystem::path& path, float size_pixels, const ImWchar* glyph_ranges, ImFontConfig& base_cfg) {
    std::error_code ec;
    if (path.empty()) {
        std::cout << "ImGui: skip empty font path\n";
        return nullptr;
    }
    const bool exists = std::filesystem::exists(path, ec);
    if (!exists) {
        std::cout << "ImGui: font not found -> " << path.u8string() << '\n';
        if (ec) {
            std::cout << "ImGui: exists() error code " << ec.message() << '\n';
        }
        return nullptr;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg = base_cfg;

    const std::string extension = path.extension().string();
    if (extension == ".ttc" || extension == ".TTC") {
        cfg.FontNo = 0; // load first face in collections
    }

    ImFont* font = io.Fonts->AddFontFromFileTTF(path.string().c_str(), size_pixels, &cfg, glyph_ranges);
    if (!font)
        return nullptr;

    std::cout << "ImGui: loaded font " << path.u8string() << '\n';
    return font;
}

static void ConfigureFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    constexpr float kFontSizePx = 18.0f;
    const ImWchar* glyph_ranges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();

    ImFontConfig font_config;
    font_config.SizePixels = kFontSizePx;
    font_config.OversampleH = 2;
    font_config.OversampleV = 1;
    font_config.PixelSnapH = false;

    ImFont* loaded_font = nullptr;

    if (const char* env_font = std::getenv("RSA_CPP_FONT_PATH"); !loaded_font && env_font && *env_font) {
        loaded_font = LoadFontFromFile(std::filesystem::path(env_font), kFontSizePx, glyph_ranges, font_config);
    }

#if defined(_WIN32)
    const std::array<std::filesystem::path, 10> system_candidates = {
        std::filesystem::path("C:/Windows/Fonts/msyh.ttc"),
        std::filesystem::path("C:/Windows/Fonts/msyh.ttf"),
        std::filesystem::path("C:/Windows/Fonts/msyhl.ttc"),
        std::filesystem::path("C:/Windows/Fonts/simhei.ttf"),
        std::filesystem::path("C:/Windows/Fonts/simhei.ttc"),
        std::filesystem::path("C:/Windows/Fonts/simsun.ttc"),
        std::filesystem::path("C:/Windows/Fonts/msmincho.ttc"),
        std::filesystem::path("C:/Windows/Fonts/Deng.ttf"),
        std::filesystem::path("C:/Windows/Fonts/mingliu.ttc"),
        std::filesystem::path("C:/Windows/Fonts/FZSTK.TTF")
    };
#elif defined(__APPLE__)
    const std::array<std::filesystem::path, 6> system_candidates = {
        std::filesystem::path("/System/Library/Fonts/PingFang.ttc"),
        std::filesystem::path("/System/Library/Fonts/Hiragino Sans GB W3.ttc"),
        std::filesystem::path("/System/Library/Fonts/Songti.ttc"),
        std::filesystem::path("/Library/Fonts/SourceHanSansSC-Regular.otf"),
        std::filesystem::path("/Library/Fonts/ArialUnicode.ttf"),
        std::filesystem::path("/Library/Fonts/华文黑体.ttf")
    };
#else
    const std::array<std::filesystem::path, 8> system_candidates = {
        std::filesystem::path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"),
        std::filesystem::path("/usr/share/fonts/opentype/noto/NotoSansSC-Regular.otf"),
        std::filesystem::path("/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"),
        std::filesystem::path("/usr/share/fonts/truetype/noto/NotoSansSC-Regular.otf"),
        std::filesystem::path("/usr/share/fonts/opentype/wqy/wqy-zenhei.ttc"),
        std::filesystem::path("/usr/share/fonts/truetype/arphic/ukai.ttc"),
        std::filesystem::path("/usr/share/fonts/truetype/arphic/uming.ttc"),
        std::filesystem::path("/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf")
    };
#endif

    // Look for bundled fonts (relative to executable/workdir) first.
    if (!loaded_font) {
        const std::array<std::filesystem::path, 3> bundled_candidates = {
            std::filesystem::path("fonts/NotoSansSC-Regular.otf"),
            std::filesystem::path("fonts/SourceHanSansSC-Regular.otf"),
            std::filesystem::path("fonts/SourceHanSansCN-Regular.otf")
        };
        for (const auto& candidate : bundled_candidates) {
            loaded_font = LoadFontFromFile(candidate, kFontSizePx, glyph_ranges, font_config);
            if (loaded_font) {
                break;
            }
        }
    }

    if (!loaded_font) {
        for (const auto& candidate : system_candidates) {
            loaded_font = LoadFontFromFile(candidate, kFontSizePx, glyph_ranges, font_config);
            if (loaded_font) {
                break;
            }
        }
    }

    if (!loaded_font) {
        std::cout << "ImGui: falling back to default font; no CJK font found.\n";
        loaded_font = io.Fonts->AddFontDefault();
    }

    // Ensure there is an ASCII fallback font for symbols ImGui might use.
    if (loaded_font != io.Fonts->Fonts.front()) {
        io.Fonts->AddFontDefault();
    }

    io.FontDefault = loaded_font;
}

int main() {
    //std::cout << "Application starting..." << std::endl;

#if !defined(_WIN32) && defined(NO_GLFW)
    std::cerr << "Error: GLFW library not found. Cannot run GUI on Linux/macOS without GLFW." << std::endl;
    std::cerr << "Please install GLFW3 development libraries. On Debian/Ubuntu: sudo apt install libglfw3-dev" << std::endl;
    return 1;
#endif

    // 1. Create window and initialize graphics
    if (!InitializeWindowAndGraphics(1280, 800, "RSA_CPP GUI")) return 1;

    // 2. Initialize ImGui
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::StyleColorsDark();
    ConfigureFonts();
    EnsureDefaultImGuiIni(ImGui::GetIO());

#if defined(_WIN32)
    extern HWND g_hwnd;
    extern ID3D11Device* g_pd3dDevice;
    extern ID3D11DeviceContext* g_pd3dDeviceContext;
    ImGui_ImplWin32_Init(g_hwnd); ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
#else
#if defined(HAS_GLFW)
    extern GLFWwindow* g_window;
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init(GetOpenGLGLSLVersion());
#endif
#endif

    //std::cout << "Initialization complete, entering main loop..." << std::endl;

    // 4. Main loop
    int frameCount = 0; // For debugging

#if defined(_WIN32)
    MSG msg = {};
    ZeroMemory(&msg, sizeof(msg));

    while (msg.message != WM_QUIT) {
        frameCount++;
        //if (frameCount % 60 == 0) { // Output every 60 frames
        //    std::cout << "Ran " << frameCount << " frames" << std::endl;
        //}

        // Process all pending messages
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
#else
    while (true) {  // For Linux/macOS, we handle loop exit differently
        frameCount++;
        //if (frameCount % 60 == 0) { // Output every 60 frames
        //    std::cout << "Ran " << frameCount << " frames" << std::endl;
        //}

#if defined(HAS_GLFW)
        extern GLFWwindow* g_window;
        if (glfwWindowShouldClose(g_window))
            break;

        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();
#endif
#endif

        // Always execute rendering (this is key!)
        NewFrame();
        ImGui::NewFrame();

        // Menu: tools and layout helpers
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Layout")) {
                if (ImGui::MenuItem("Reset to Default")) {
                    ImGui::LoadIniSettingsFromMemory(DEFAULT_IMGUI_INI_CONTENT);
                    if (ImGui::GetIO().IniFilename)
                        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ShowOpenSslPemWindow();
        Myspace::GlobalResultWindow();

        Render();
    }

    // 5. Cleanup
#if defined(_WIN32)
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown();
#else
#if defined(HAS_GLFW)
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
#endif
#endif
    ImGui::DestroyContext();
    CleanupGraphicsAndWindow();
#if defined(_WIN32)
    return (int)msg.wParam;
#else
    return 0;
#endif
}
