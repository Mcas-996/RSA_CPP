#pragma once
#include <string>
#include <vector>
#include <exception>
#include "RSA.hpp"
#include "Auto.h"
#include "prepare.hpp"
#include "third_party/cppcodec/cppcodec/base64_rfc4648.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <iostream>
#include "file_dialog.hpp"
//namespace Myspace {
//	void mywindow() {
//		using namespace ImGui;
//		Begin("RSA");
//		long long puk = 0, prk = 0, modn = 0;
//		static char bbuffer[64];
//		Text("Enter Public Key:\n");
//		InputText("", bbuffer, IM_ARRAYSIZE(bbuffer));
//		std::string str(bbuffer);
//		puk = std::stoll(str);
//		Text("\nEnter Private Key:\n");
//		InputText("", bbuffer, IM_ARRAYSIZE(bbuffer));
//		str = bbuffer;
//		prk = std::stoll(str);
//		Text("\nEnter Mod number:\n");
//		InputText("", bbuffer, IM_ARRAYSIZE(bbuffer));
//		str = bbuffer;
//		modn = std::stoll(str);
//		std::cout << "puk: "  <<puk <<" prk: " << prk << std::endl;
//		End();
//	}
//};

int flag = 0;
namespace Myspace {

    namespace detail {
        struct AutoWrapUserData {
            int wrap_column;
            bool break_after_comma;
        };

        static int AutoWrapCallback(ImGuiInputTextCallbackData* data) {
            auto* ctx = static_cast<AutoWrapUserData*>(data->UserData);
            if (!ctx || ctx->wrap_column <= 0)
                return 0;

            int line_start = 0;
            for (int i = 0; i < data->BufTextLen; ++i) {
                const char c = data->Buf[i];
                if (c == '\n') {
                    line_start = i + 1;
                    continue;
                }
                if (ctx->break_after_comma && c == ',') {
                    line_start = i + 1;
                    continue;
                }

                const int line_length = i - line_start + 1;
                if (line_length > ctx->wrap_column) {
                    if (data->BufTextLen + 1 >= data->BufSize)
                        break;
                    data->InsertChars(i, "\n");
                    line_start = i + 1;
                    ++i; // Skip the newline we just inserted.
                }
            }
            return 0;
        }

        static std::string sanitizeCipherInput(const char* buffer) {
            std::string cleaned;
            const size_t len = std::strlen(buffer);
            cleaned.reserve(len);
            for (size_t i = 0; i < len; ++i) {
                const unsigned char c = static_cast<unsigned char>(buffer[i]);
                if (!std::isspace(c))
                    cleaned.push_back(static_cast<char>(c));
            }
            return cleaned;
        }

        static std::string stripWhitespace(const std::string& input) {
            std::string cleaned;
            cleaned.reserve(input.size());
            for (unsigned char c : input) {
                if (!std::isspace(c)) {
                    cleaned.push_back(static_cast<char>(c));
                }
            }
            return cleaned;
        }

        static std::string trimWhitespace(const std::string& input) {
            auto begin = std::find_if_not(input.begin(), input.end(),
                [](unsigned char c) { return std::isspace(c); });
            auto rend = std::find_if_not(input.rbegin(), input.rend(),
                [](unsigned char c) { return std::isspace(c); });
            if (begin == input.end()) {
                return {};
            }
            return std::string(begin, rend.base());
        }

        static std::string normalizePath(const std::string& input) {
            std::string trimmed = trimWhitespace(input);
            if (trimmed.size() >= 2 &&
                ((trimmed.front() == '"' && trimmed.back() == '"') ||
                 (trimmed.front() == '\'' && trimmed.back() == '\''))) {
                trimmed = trimmed.substr(1, trimmed.size() - 2);
            }
            return trimmed;
        }

        static std::string readFileBinary(const std::string& path) {
            const std::filesystem::path fsPath = std::filesystem::u8path(path);
            std::ifstream file(fsPath, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Failed to open file: " + std::string(fsPath.u8string()));
            }
            std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return data;
        }

        static void writeFileBinary(const std::string& path, const std::string& data) {
            const std::filesystem::path fsPath = std::filesystem::u8path(path);
            std::ofstream file(fsPath, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Failed to open file for writing: " + std::string(fsPath.u8string()));
            }
            file.write(data.data(), static_cast<std::streamsize>(data.size()));
            if (!file) {
                throw std::runtime_error("Failed to write file: " + std::string(fsPath.u8string()));
            }
        }

        struct StringInputCallbackData {
            std::string* str;
            ImGuiInputTextCallback chained_callback;
            void* chained_user_data;
        };

        static int InputTextStringCallback(ImGuiInputTextCallbackData* data) {
            auto* user = static_cast<StringInputCallbackData*>(data->UserData);
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                user->str->resize(data->BufTextLen);
                data->Buf = user->str->data();
                return 0;
            }
            if (user->chained_callback) {
                data->UserData = user->chained_user_data;
                const int result = user->chained_callback(data);
                data->UserData = user;
                return result;
            }
            return 0;
        }

        static bool InputTextMultilineString(const char* label,
                                             std::string& str,
                                             const ImVec2& size,
                                             ImGuiInputTextFlags flags,
                                             ImGuiInputTextCallback callback = nullptr,
                                             void* user_data = nullptr) {
            flags |= ImGuiInputTextFlags_CallbackResize;
            if (str.capacity() == 0) {
                str.reserve(1024);
            }
            if (str.capacity() < str.size() + 1) {
                str.reserve(str.size() + 1);
            }
            // Ensure null terminator exists for ImGui to read.
            str.push_back('\0');
            str.pop_back();

            StringInputCallbackData cb{ &str, callback, user_data };
            return ImGui::InputTextMultiline(label,
                                             str.data(),
                                             str.capacity() + 1,
                                             size,
                                             flags,
                                             InputTextStringCallback,
                                             &cb);
        }

        static std::string encodeRawBase64(const std::string& data) {
            return cppcodec::base64_rfc4648::encode(
                reinterpret_cast<const uint8_t*>(data.data()), data.size());
        }

        static std::string decodeBase64ToString(const std::string& base64) {
            std::vector<uint8_t> bytes = cppcodec::base64_rfc4648::decode(base64);
            return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }

        static std::string fileToBase64String(const std::string& path) {
            return encodeRawBase64(readFileBinary(path));
        }

        static void base64StringToFile(const std::string& base64, const std::string& path) {
            writeFileBinary(path, decodeBase64ToString(base64));
        }

        static std::string encodeCiphertextBase64(const std::vector<long long>& values) {
            std::vector<uint8_t> bytes;
            bytes.reserve(values.size() * sizeof(uint64_t));
            for (long long value : values) {
                uint64_t uvalue = static_cast<uint64_t>(value);
                for (int byte = 0; byte < 8; ++byte) {
                    bytes.push_back(static_cast<uint8_t>((uvalue >> (byte * 8)) & 0xFF));
                }
            }
            return cppcodec::base64_rfc4648::encode(bytes);
        }

        static std::vector<long long> decodeCiphertextBase64(const std::string& encoded) {
            std::vector<uint8_t> bytes = cppcodec::base64_rfc4648::decode(encoded);
            if (bytes.size() % sizeof(uint64_t) != 0) {
                throw std::invalid_argument("Base64 ciphertext length mismatch");
            }
            std::vector<long long> values(bytes.size() / sizeof(uint64_t));
            for (size_t i = 0; i < values.size(); ++i) {
                uint64_t value = 0;
                for (int byte = 0; byte < 8; ++byte) {
                    value |= static_cast<uint64_t>(bytes[i * 8 + byte]) << (byte * 8);
                }
                values[i] = static_cast<long long>(value);
            }
            return values;
        }

        static bool isNumericCiphertext(const std::string& input) {
            if (input.empty()) {
                return true;
            }
            return input.find_first_not_of("0123456789,-") == std::string::npos;
        }

        static std::vector<long long> parseCiphertext(const std::string& input) {
            const std::string cleaned = stripWhitespace(input);
            if (cleaned.empty()) {
                return {};
            }
            try {
                return decodeCiphertextBase64(cleaned);
            } catch (const std::exception&) {
                if (isNumericCiphertext(cleaned)) {
                    return RSAUtil::stringToCiphertext(cleaned);
                }
                throw;
            }
        }

        static std::string wrapForDisplay(const std::string& input, int wrap_column, bool break_after_comma = false) {
            if (wrap_column <= 0)
                return input;

            std::string output;
            output.reserve(input.size() + input.size() / wrap_column + 1);

            int line_length = 0;
            for (char c : input) {
                output.push_back(c);

                if (c == '\n') {
                    line_length = 0;
                    continue;
                }

                if (break_after_comma && c == ',') {
                    output.push_back('\n');
                    line_length = 0;
                    continue;
                }

                ++line_length;
                if (line_length >= wrap_column) {
                    output.push_back('\n');
                    line_length = 0;
                }
            }

            return output;
        }
    }

    static constexpr size_t KEY_BUFFER_SIZE = 512;

    RSAUtil::KeyPair KP;
    enum class ResultType {
        None,
        CiphertextBase64,
        Plaintext,
        Info,
        Error
    };
    ResultType resultType = ResultType::None;
    std::string resultPrimary;
    std::string resultSecondary;
    // Compute a 50% screen workspace anchored near the top-left and helper to place windows proportionally
    static inline void SetWindowPosSizeRatio(const char* /*name*/, ImVec2 rel_pos, ImVec2 rel_size)
    {
        ImVec2 disp = ImGui::GetIO().DisplaySize;
        const float scale = 0.5f; // target overall area ~50%
        ImVec2 work_size = ImVec2(disp.x * scale, disp.y * scale);
        const ImVec2 margin(20.0f, 40.0f); // anchor near top-left with a small padding
        ImVec2 work_origin = margin;
        ImVec2 pos = ImVec2(work_origin.x + work_size.x * rel_pos.x, work_origin.y + work_size.y * rel_pos.y);
        ImVec2 size = ImVec2(work_size.x * rel_size.x, work_size.y * rel_size.y);
        ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
    }

    void TraditionalRSAWindow() {
        using namespace ImGui;
        SetWindowPosSizeRatio("Traditional RSA", ImVec2(0.01f, 0.02f), ImVec2(0.46f, 0.94f));
        Begin("Traditional RSA");

        static char public_key_buffer[KEY_BUFFER_SIZE] = "";
        static char private_key_buffer[KEY_BUFFER_SIZE] = "";
        static char mod_number_buffer[KEY_BUFFER_SIZE] = "";
        static detail::AutoWrapUserData key_wrap{ 64, false };
        static std::string keyInputStatus;

        TextUnformatted("Manual Key Input");
        Separator();
        Text("Public Key:");
        InputTextMultiline("##ManualPublicKey",
                           public_key_buffer,
                           KEY_BUFFER_SIZE,
                           ImVec2(-FLT_MIN, GetTextLineHeight() * 3),
                           ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways,
                           detail::AutoWrapCallback,
                           &key_wrap);

        Text("Private Key:");
        InputTextMultiline("##ManualPrivateKey",
                           private_key_buffer,
                           KEY_BUFFER_SIZE,
                           ImVec2(-FLT_MIN, GetTextLineHeight() * 3),
                           ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways,
                           detail::AutoWrapCallback,
                           &key_wrap);

        Text("Mod Number:");
        InputTextMultiline("##ManualModNumber",
                           mod_number_buffer,
                           KEY_BUFFER_SIZE,
                           ImVec2(-FLT_MIN, GetTextLineHeight() * 3),
                           ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways,
                           detail::AutoWrapCallback,
                           &key_wrap);

        if (Button("Apply manual key pair")) {
            const std::string puk = detail::sanitizeCipherInput(public_key_buffer);
            const std::string prk = detail::sanitizeCipherInput(private_key_buffer);
            const std::string modn = detail::sanitizeCipherInput(mod_number_buffer);
            KP = { puk, prk, modn };
            keyInputStatus = "Manual key pair applied.";
        }
        SameLine();
        if (Button("Generate new key pair")) {
            KP = RSAUtil::generateKeyPair();
            snprintf(public_key_buffer, sizeof(public_key_buffer), "%s", KP.publicKey.c_str());
            snprintf(private_key_buffer, sizeof(private_key_buffer), "%s", KP.privateKey.c_str());
            snprintf(mod_number_buffer, sizeof(mod_number_buffer), "%s", KP.modulus.c_str());
            keyInputStatus = "Generated new key pair.";
        }
        if (!keyInputStatus.empty()) {
            TextWrapped("Status: %s", keyInputStatus.c_str());
        }

        Separator();
        TextUnformatted("Current Key Pair");
        Separator();

        const std::string wrapped_public = detail::wrapForDisplay(KP.publicKey, 64);
        const std::string wrapped_private = detail::wrapForDisplay(KP.privateKey, 64);
        const std::string wrapped_mod = detail::wrapForDisplay(KP.modulus, 64);

        static char public_key_str[KEY_BUFFER_SIZE] = "";
        static char private_key_str[KEY_BUFFER_SIZE] = "";
        static char mod_number_str[KEY_BUFFER_SIZE] = "";

        snprintf(public_key_str, sizeof(public_key_str), "%s", wrapped_public.c_str());
        snprintf(private_key_str, sizeof(private_key_str), "%s", wrapped_private.c_str());
        snprintf(mod_number_str, sizeof(mod_number_str), "%s", wrapped_mod.c_str());

        Text("Public Key:");
        InputTextMultiline("##DisplayPublicKey",
                           public_key_str,
                           sizeof(public_key_str),
                           ImVec2(-FLT_MIN, GetTextLineHeight() * 5),
                           ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

        Text("Private Key:");
        InputTextMultiline("##DisplayPrivateKey",
                           private_key_str,
                           sizeof(private_key_str),
                           ImVec2(-FLT_MIN, GetTextLineHeight() * 5),
                           ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

        Text("Mod Number:");
        InputTextMultiline("##DisplayModNumber",
                           mod_number_str,
                           sizeof(mod_number_str),
                           ImVec2(-FLT_MIN, GetTextLineHeight() * 5),
                           ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

        Separator();
        TextUnformatted("Encryption");
        Separator();
        static std::string encrypt_buffer;
        if (encrypt_buffer.capacity() < 262144) {
            encrypt_buffer.reserve(262144);
        }
        Text("Plaintext:");
        SameLine();
        if (Button("Get from result##TraditionalEncrypt")) {
            encrypt_buffer = resultPrimary;
        }
        static detail::AutoWrapUserData plaintext_wrap{ 64, false };
        detail::InputTextMultilineString("##TraditionalEncryptInput",
                                         encrypt_buffer,
                                         ImVec2(-FLT_MIN, GetTextLineHeight() * 8),
                                         ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways,
                                         detail::AutoWrapCallback,
                                         &plaintext_wrap);
        Text("Length: %zu", encrypt_buffer.size());
        if (Button("Encrypt##Traditional")) {
            if (KP.publicKey.empty() || KP.privateKey.empty() || KP.modulus.empty()) {
                resultPrimary = "Encrypt failed: please generate or input a key pair first.";
                resultSecondary.clear();
                resultType = ResultType::Error;
            } else {
                try {
                    const std::vector<long long> res = RSAUtil::encryptText(encrypt_buffer, KP);
                    resultPrimary = detail::encodeCiphertextBase64(res);
                    resultSecondary = RSAUtil::ciphertextToString(res);
                    resultType = ResultType::CiphertextBase64;
                } catch (const std::exception& e) {
                    resultPrimary = std::string("Encrypt failed: ") + e.what();
                    resultSecondary.clear();
                    resultType = ResultType::Error;
                    std::cerr << resultPrimary << std::endl;
                }
            }
        }

        Separator();
        TextUnformatted("Decryption");
        Separator();
        static std::string decrypt_buffer;
        if (decrypt_buffer.capacity() < 262144) {
            decrypt_buffer.reserve(262144);
        }
        static detail::AutoWrapUserData cipher_wrap{ 64, true };
        Text("Ciphertext:");
        SameLine();
        if (Button("Get from result##TraditionalDecrypt")) {
            decrypt_buffer = resultPrimary;
        }
        detail::InputTextMultilineString("##TraditionalDecryptInput",
                                         decrypt_buffer,
                                         ImVec2(-FLT_MIN, GetTextLineHeight() * 8),
                                         ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways,
                                         detail::AutoWrapCallback,
                                         &cipher_wrap);
        Text("Length: %zu", decrypt_buffer.size());
        if (Button("Decrypt##Traditional")) {
            if (KP.publicKey.empty() || KP.privateKey.empty() || KP.modulus.empty()) {
                resultPrimary = "Decrypt failed: please generate or input a key pair first.";
                resultSecondary.clear();
                resultType = ResultType::Error;
            } else {
                try {
                    const std::string cleaned = detail::sanitizeCipherInput(decrypt_buffer.c_str());
                    const std::vector<long long> ciphertext = detail::parseCiphertext(cleaned);
                    const std::string decrypted = RSAUtil::decryptText(ciphertext, KP);
                    resultPrimary = decrypted;
                    resultSecondary = RSAUtil::ciphertextToString(ciphertext);
                    resultType = ResultType::Plaintext;
                } catch (const std::exception& e) {
                    resultPrimary = std::string("Decrypt failed: ") + e.what();
                    resultSecondary.clear();
                    resultType = ResultType::Error;
                    std::cerr << resultPrimary << std::endl;
                }
            }
        }

        Separator();
        TextUnformatted("Base64 Helper");
        Separator();
        static char convert_path[512] = "";
        static char dest_path[512] = "";
        static detail::AutoWrapUserData base64_wrap{ 64, false };
        static std::string base64_buffer;
        if (base64_buffer.capacity() < 64 * 1024) {
            base64_buffer.reserve(64 * 1024);
        }

        InputText("Binary file path", convert_path, IM_ARRAYSIZE(convert_path));
        SameLine();
        if (Button("Browse##BinaryPathDialog")) {
            if (auto selected = platform::dialog::open_file("Select source file", "All Files (*.*)|*.*")) {
                std::snprintf(convert_path, IM_ARRAYSIZE(convert_path), "%s", selected->c_str());
            }
        }
        InputText("Target file path", dest_path, IM_ARRAYSIZE(dest_path));
        SameLine();
        if (Button("Browse##TargetPathDialog")) {
            if (auto selected = platform::dialog::save_file("Select destination path", "All Files (*.*)|*.*")) {
                std::snprintf(dest_path, IM_ARRAYSIZE(dest_path), "%s", selected->c_str());
            }
        }
        static bool enableWrap = true;
        Checkbox("Auto-wrap buffer", &enableWrap);
        ImGuiInputTextFlags bufferFlags = enableWrap ? ImGuiInputTextFlags_CallbackAlways : ImGuiInputTextFlags_None;
        SameLine();
        if (Button("Get from result##Base64Buffer")) {
            base64_buffer = resultPrimary;
        }
        detail::InputTextMultilineString("Base64 buffer",
                                         base64_buffer,
                                         ImVec2(-FLT_MIN, GetTextLineHeight() * 5),
                                         bufferFlags,
                                         enableWrap ? detail::AutoWrapCallback : nullptr,
                                         enableWrap ? static_cast<void*>(&base64_wrap) : nullptr);
        Text("Buffer length: %zu", base64_buffer.size());
        SameLine();
        if (Button("Copy buffer##Traditional")) {
            SetClipboardText(base64_buffer.c_str());
        }

        if (Button("Load file -> buffer##Traditional")) {
            const std::string srcPath = detail::normalizePath(convert_path);
            if (srcPath.empty()) {
                resultType = ResultType::Error;
                resultPrimary = "Convert failed: binary file path cannot be empty.";
                resultSecondary.clear();
            } else {
                try {
                    const std::string base64 = detail::fileToBase64String(srcPath);
                    base64_buffer = base64;
                    resultType = ResultType::Info;
                    resultPrimary = base64;
                    resultSecondary = "Base64 length: " + std::to_string(base64.size());
                } catch (const std::exception& e) {
                    resultType = ResultType::Error;
                    resultPrimary = std::string("Convert failed: ") + e.what();
                    resultSecondary.clear();
                }
            }
        }
        SameLine();
        if (Button("Save buffer -> file##Traditional")) {
            const std::string dstPath = detail::normalizePath(dest_path);
            if (dstPath.empty()) {
                resultType = ResultType::Error;
                resultPrimary = "Write failed: target file path cannot be empty.";
                resultSecondary.clear();
            } else {
                std::string base64Input = detail::trimWhitespace(base64_buffer);
                if (base64Input.empty()) {
                    resultType = ResultType::Error;
                    resultPrimary = "Write failed: Base64 buffer is empty.";
                    resultSecondary.clear();
                } else {
                    try {
                        const std::string decoded = detail::decodeBase64ToString(base64Input);
                        detail::writeFileBinary(dstPath, decoded);
                        resultType = ResultType::Info;
                        resultPrimary = "Binary file written to: " + dstPath;
                        resultSecondary = "Decoded bytes: " + std::to_string(decoded.size());
                    } catch (const std::exception& e) {
                        resultType = ResultType::Error;
                        resultPrimary = std::string("Write failed: ") + e.what();
                        resultSecondary.clear();
                    }
                }
            }
        }

        Separator();
        TextUnformatted("Result Viewer");
        Separator();
        if (resultType == ResultType::None) {
            Text("No result yet.");
        } else {
            int wrap_column = 64;
            const float avail_width = GetContentRegionAvail().x;
            if (avail_width > 0.0f) {
                const float char_width = CalcTextSize("M").x;
                if (char_width > 0.0f) {
                    wrap_column = static_cast<int>(avail_width / char_width);
                    if (wrap_column < 1) {
                        wrap_column = 1;
                    }
                }
            }

            const char* primaryLabel = "Result";
            const char* secondaryLabel = nullptr;
            switch (resultType) {
            case ResultType::CiphertextBase64:
                primaryLabel = "Ciphertext (Base64)";
                secondaryLabel = "Ciphertext (Numbers)";
                break;
            case ResultType::Plaintext:
                primaryLabel = "Plaintext";
                secondaryLabel = "Ciphertext (Numbers)";
                break;
            case ResultType::Info:
                primaryLabel = "Info";
                if (!resultSecondary.empty()) {
                    secondaryLabel = "Details";
                }
                break;
            case ResultType::Error:
                primaryLabel = "Status";
                break;
            default:
                break;
            }

            static char primary_buffer[8192] = "";
            const std::string wrapped_primary = detail::wrapForDisplay(resultPrimary, wrap_column);
            snprintf(primary_buffer, sizeof(primary_buffer), "%s", wrapped_primary.c_str());

            Text("%s:", primaryLabel);
            SameLine();
            if (Button("Copy##TraditionalPrimaryResult")) {
                SetClipboardText(resultPrimary.c_str());
            }
            InputTextMultiline("##TraditionalPrimaryResult",
                               primary_buffer,
                               sizeof(primary_buffer),
                               ImVec2(-FLT_MIN, GetTextLineHeight() * 9),
                               ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

            if (secondaryLabel && !resultSecondary.empty()) {
                Separator();
                Text("%s:", secondaryLabel);
                SameLine();
                if (Button("Copy##TraditionalSecondaryResult")) {
                    SetClipboardText(resultSecondary.c_str());
                }
                static char secondary_buffer[8192] = "";
                const std::string wrapped_secondary = detail::wrapForDisplay(resultSecondary, wrap_column, true);
                snprintf(secondary_buffer, sizeof(secondary_buffer), "%s", wrapped_secondary.c_str());
                InputTextMultiline("##TraditionalSecondaryResult",
                                   secondary_buffer,
                                   sizeof(secondary_buffer),
                                   ImVec2(-FLT_MIN, GetTextLineHeight() * 6),
                                   ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
            }
        }

        End();
    }

    void GlobalResultWindow() {
        using namespace ImGui;
        SetWindowPosSizeRatio("Result", ImVec2(0.52f, 0.70f), ImVec2(0.45f, 0.26f));
        Begin("Result");

        TextUnformatted("Latest result output");
        SameLine();
        if (Button("Copy##GlobalResult")) {
            SetClipboardText(resultPrimary.c_str());
        }

        static char global_result_buffer[8192] = "";
        const std::string wrapped = detail::wrapForDisplay(resultPrimary, 72);
        snprintf(global_result_buffer, sizeof(global_result_buffer), "%s", wrapped.c_str());
        InputTextMultiline("##GlobalResultOutput",
                           global_result_buffer,
                           sizeof(global_result_buffer),
                           ImVec2(-FLT_MIN, GetTextLineHeight() * 8),
                           ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

        End();
    }
}
