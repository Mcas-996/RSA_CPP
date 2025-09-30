#pragma once
#include <string>
#include <vector>
#include <exception>
#include "RSA.hpp"
#include "Auto.h"
#include "prepare.hpp"
#include <iostream>
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

    RSA::KeyPair KP;
    std::string result;
    void input_KP_window() {
        using namespace ImGui;
        Begin("RSA");

        // Create separate buffers for each input field
        static char public_key_buffer[64] = "";
        static char private_key_buffer[64] = "";
        static char mod_number_buffer[64] = "";

        // Display prompt text first
        Text("Enter Public Key:");
        // Add label to input field, using different buffers
        InputText("##PublicKey", public_key_buffer, IM_ARRAYSIZE(public_key_buffer));

        Text("Enter Private Key:");
        InputText("##PrivateKey", private_key_buffer, IM_ARRAYSIZE(private_key_buffer));

        Text("Enter Mod Number:");
        InputText("##ModNumber", mod_number_buffer, IM_ARRAYSIZE(mod_number_buffer));

        // Add conversion button (optional, more secure)
        if (Button("Process Keys")) {
            long long puk = 0, prk = 0, modn = 0;

            // Convert public key
            if (strlen(public_key_buffer) > 0) {
                try {
                    std::string pub_str = public_key_buffer;
                    puk = std::stoll(pub_str);
                }
                catch (const std::exception& e) {
                    TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid Public Key!");
                }
            }

            // Convert private key
            if (strlen(private_key_buffer) > 0) {
                try {
                    std::string priv_str = private_key_buffer;
                    prk = std::stoll(priv_str);
                }
                catch (const std::exception& e) {
                    TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid Private Key!");
                }
            }

            // Convert modulus
            if (strlen(mod_number_buffer) > 0) {
                try {
                    std::string mod_str = mod_number_buffer;
                    modn = std::stoll(mod_str);
                }
                catch (const std::exception& e) {
                    TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid Mod Number!");
                }
            }

            // Display results
            Text("Public Key: %lld", puk);
            Text("Private Key: %lld", prk);
            Text("Mod Number: %lld", modn);

            // Output to console (optional)
            std::cout << "Public Key: " << puk << " Private Key: " << prk << " Modulus: " << modn << std::endl;
            
            KP = { puk, prk, modn };
        }

        End();
    }

    void Generate_KP_window() {
        ImGui::Begin("Generate");
        if (ImGui::Button("generate a new key")) {
            KP = RSA::generateKeyPair();
        }
        ImGui::End();
    }

    void Show_KP_window() {
        ImGui::Begin("Key");
        // Numeric variables
         long long public_key = 
            KP.publicKey;
        long long private_key = 
            KP.privateKey;
        long long mod_number = 
            KP.modulus;

        // Corresponding character buffers
        static char public_key_str[32] = "";
        static char private_key_str[32] = "";
        static char mod_number_str[32] = "";

        // Convert numbers to strings
        snprintf(public_key_str, sizeof(public_key_str), "%lld", public_key);
        snprintf(private_key_str, sizeof(private_key_str), "%lld", private_key);
        snprintf(mod_number_str, sizeof(mod_number_str), "%lld", mod_number);

        // Display using read-only input fields
        ImGui::Text("Public Key:");
        ImGui::InputText("##PublicKeyDisplay", 
            public_key_str, sizeof(public_key_str),
            ImGuiInputTextFlags_ReadOnly);

        ImGui::Text("Private Key:");
        ImGui::InputText("##PrivateKeyDisplay", 
            private_key_str, sizeof(private_key_str),
            ImGuiInputTextFlags_ReadOnly);

        ImGui::Text("Mod Number:");
        ImGui::InputText("##ModNumberDisplay", 
            mod_number_str, sizeof(mod_number_str),
            ImGuiInputTextFlags_ReadOnly);

        ImGui::End();
    }

    void enpt() {
        using namespace ImGui;
        Begin("encrypt");
        static char buffer[4096];
        Text("text:");
        InputText("", buffer, sizeof(buffer));
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
        Begin("descrypt");
        static char buffer[4096];
        Text("text:");
        InputText("", buffer, sizeof(buffer));
        std::string s(buffer);
        Text("length: %s\n", std::to_string(s.size()).c_str());
        if (Button("decrypt")) {
            auto a = RSA::stringToCiphertext(s);
            s = RSA::decryptText(a, KP);
            result = s;
        }
       
        End();
    }
    void show_res() {
        using namespace ImGui;
        Begin("result");
        static char result_buffer[2048] = "";
        snprintf(result_buffer, sizeof(result_buffer), "%s", result.c_str());
        InputText("##ResultDisplay", 
                         result_buffer, 
                         sizeof(result_buffer),
                         ImGuiInputTextFlags_ReadOnly);
        End();
    }
}