#pragma once
#include <string>
#include <vector>
#include <exception>
#include "RSA.hpp"
#include "Auto.h"
#include "prepare.hpp"
#include <iostream>
#include <cstring>
#include <cstdio>
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

        static std::string sanitizeNumericInput(const char* buffer) {
            std::string cleaned;
            const size_t len = std::strlen(buffer);
            cleaned.reserve(len);
            for (size_t i = 0; i < len; ++i) {
                const char c = buffer[i];
                if (c != '\n' && c != '\r' && c != ' ')
                    cleaned.push_back(c);
            }
            return cleaned;
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

    RSA::KeyPair KP;
    std::string result;
    void input_KP_window() {
        using namespace ImGui;
        // Set default position/size only on first use (honors imgui.ini if present)
        ImGui::SetNextWindowPos(ImVec2(30, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(420, 260), ImGuiCond_FirstUseEver);
        Begin("RSA");

        // Create separate buffers for each input field
        static char public_key_buffer[KEY_BUFFER_SIZE] = "";
        static char private_key_buffer[KEY_BUFFER_SIZE] = "";
        static char mod_number_buffer[KEY_BUFFER_SIZE] = "";
        static detail::AutoWrapUserData key_wrap{ 64, false };

        // Display prompt text first
        Text("Enter Public Key:");
        // Add label to input field, using different buffers
        InputTextMultiline("##public_key",
                           public_key_buffer,
                           KEY_BUFFER_SIZE,
                           ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 3),
                           ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways,
                           detail::AutoWrapCallback,
                           &key_wrap);

        Text("Enter Private Key:");
        InputTextMultiline("##PrivateKey",
                           private_key_buffer,
                           KEY_BUFFER_SIZE,
                           ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 3),
                           ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways,
                           detail::AutoWrapCallback,
                           &key_wrap);

        Text("Enter Mod Number:");
        InputTextMultiline("##ModNumber",
                           mod_number_buffer,
                           KEY_BUFFER_SIZE,
                           ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 3),
                           ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways,
                           detail::AutoWrapCallback,
                           &key_wrap);

        // Add conversion button (optional, more secure)
        if (Button("Process Keys")) {
            std::string puk = "", prk = "", modn = "";

            // Convert public key
            if (strlen(public_key_buffer) > 0) {
                try {
                    puk = detail::sanitizeNumericInput(public_key_buffer);
                }
                catch (const std::exception& e) {
                    TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid Public Key!");
                }
            }

            // Convert private key
            if (strlen(private_key_buffer) > 0) {
                try {
                    prk = detail::sanitizeNumericInput(private_key_buffer);
                }
                catch (const std::exception& e) {
                    TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid Private Key!");
                }
            }

            // Convert modulus
            if (strlen(mod_number_buffer) > 0) {
                try {
                    modn = detail::sanitizeNumericInput(mod_number_buffer);
                }
                catch (const std::exception& e) {
                    TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid Mod Number!");
                }
            }

            // Display results
            TextWrapped("Public Key: %s", puk.c_str());
            TextWrapped("Private Key: %s", prk.c_str());
            TextWrapped("Mod Number: %s", modn.c_str());

            // Output to console (optional)
            std::cout << "Public Key: " << puk << " Private Key: " << prk << " Modulus: " << modn << std::endl;
            
            KP = { puk, prk, modn };
        }

        End();
    }

    void Generate_KP_window() {
        ImGui::SetNextWindowPos(ImVec2(30, 340), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(260, 90), ImGuiCond_FirstUseEver);
        ImGui::Begin("Generate");
        if (ImGui::Button("generate a new key")) {
            KP = RSA::generateKeyPair();
        }
        ImGui::End();
    }

    void Show_KP_window() {
        ImGui::SetNextWindowPos(ImVec2(30, 440), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(520, 360), ImGuiCond_FirstUseEver);
        ImGui::Begin("Key");
        // String variables
        std::string public_key = KP.publicKey;
        std::string private_key = KP.privateKey;
        std::string mod_number = KP.modulus;

        // Corresponding character buffers
        static char public_key_str[KEY_BUFFER_SIZE] = "";
        static char private_key_str[KEY_BUFFER_SIZE] = "";
        static char mod_number_str[KEY_BUFFER_SIZE] = "";

        // Convert strings to character arrays
        const std::string wrapped_public = detail::wrapForDisplay(public_key, 64);
        const std::string wrapped_private = detail::wrapForDisplay(private_key, 64);
        const std::string wrapped_mod = detail::wrapForDisplay(mod_number, 64);

        snprintf(public_key_str, sizeof(public_key_str), "%s", wrapped_public.c_str());
        snprintf(private_key_str, sizeof(private_key_str), "%s", wrapped_private.c_str());
        snprintf(mod_number_str, sizeof(mod_number_str), "%s", wrapped_mod.c_str());

        // Display using read-only input fields
        ImGui::Text("Public Key:");
        ImGui::InputTextMultiline("##PublicKeyDisplay",
            public_key_str, sizeof(public_key_str),
            ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 5),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

        ImGui::Text("Private Key:");
        ImGui::InputTextMultiline("##PrivateKeyDisplay",
            private_key_str, sizeof(private_key_str),
            ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 5),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

        ImGui::Text("Mod Number:");
        ImGui::InputTextMultiline("##ModNumberDisplay",
            mod_number_str, sizeof(mod_number_str),
            ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 5),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

        ImGui::End();
    }

    void enpt() {
        using namespace ImGui;
        ImGui::SetNextWindowPos(ImVec2(560, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(520, 280), ImGuiCond_FirstUseEver);
        Begin("encrypt");
        static char buffer[4096];
        Text("text:");
        InputTextMultiline("##text_input",
                           buffer,
                           sizeof(buffer),
                           ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 8),
                           ImGuiInputTextFlags_NoHorizontalScroll);
        std::string s(buffer);
        Text("length: %s\n", std::to_string(s.size()).c_str());
        if (Button("encrypt")) {
            const std::string input_buffer = s;
            std::vector<long long> res = RSA::encryptText(input_buffer, KP);
            /*for (auto i : res) {
                std::cout << (std::to_string(i).c_str()) << std::endl;
            }*/ // Debug content
            result = RSA::ciphertextToString(res);
        }
        End();
    }
    void dept() {
        using namespace ImGui;
        ImGui::SetNextWindowPos(ImVec2(560, 360), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(520, 280), ImGuiCond_FirstUseEver);
        Begin("descrypt");
        static char buffer[4096];
        static detail::AutoWrapUserData cipher_wrap{ 64, true };
        Text("text:");
        InputTextMultiline("##decrypt_text",
                           buffer,
                           sizeof(buffer),
                           ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 8),
                           ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CallbackAlways,
                           detail::AutoWrapCallback,
                           &cipher_wrap);
        std::string s(buffer);
        Text("length: %s\n", std::to_string(s.size()).c_str());
        if (Button("decrypt")) {
            auto a = RSA::stringToCiphertext(detail::sanitizeNumericInput(buffer));
            s = RSA::decryptText(a, KP);
            result = s;
        }
       
        End();
    }
    void show_res() {
        using namespace ImGui;
        ImGui::SetNextWindowPos(ImVec2(560, 660), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(520, 280), ImGuiCond_FirstUseEver);
        Begin("result");
        static char result_buffer[4096] = "";
        const std::string wrapped_result = detail::wrapForDisplay(result, 64, true);
        snprintf(result_buffer, sizeof(result_buffer), "%s", wrapped_result.c_str());
        InputTextMultiline("##ResultDisplay",
                         result_buffer,
                         sizeof(result_buffer),
                         ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 12),
                         ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
        End();
    }
}
