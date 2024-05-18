#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "nlohmann/json.hpp"
#include "model_loader.h"
#include "chat_interface.h"

// Configuration for threading
const int NUM_THREADS = std::thread::hardware_concurrency();

class BrainChat {
public:
    BrainChat(const std::string& model_path, const std::string& config_path)
        : model_path_(model_path), config_path_(config_path) {
        loadModel();
        loadConfig();
    }

    void startChat() {
        std::cout << "Starting chat session..." << std::endl;
        std::string input;
        while (true) {
            std::cout << "You: ";
            std::getline(std::cin, input);
            if (input == "exit") {
                break;
            }
            std::string response = getResponse(input);
            std::cout << "Brain: " << response << std::endl;
        }
    }

private:
    std::string model_path_;
    std::string config_path_;
    std::unique_ptr<ModelLoader> model_;
    ChatInterface chat_interface_;
    nlohmann::json config_;

    void loadModel() {
        model_ = std::make_unique<ModelLoader>(model_path_);
        if (!model_->isLoaded()) {
            throw std::runtime_error("Failed to load model.");
        }
    }

    void loadConfig() {
        std::ifstream config_file(config_path_);
        if (!config_file.is_open()) {
            throw std::runtime_error("Failed to open config file.");
        }
        config_file >> config_;
        config_file.close();
    }

    std::string getResponse(const std::string& input) {
        std::vector<std::string> tokens = tokenize(input);
        std::string response;
        std::vector<std::thread> threads;
        std::mutex response_mutex;
        std::condition_variable cv;
        bool response_ready = false;

        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back([this, &tokens, &response, &response_mutex, &cv, &response_ready, i]() {
                std::string part_response = chat_interface_.generateResponse(tokens, i, model_.get());
                {
                    std::lock_guard<std::mutex> lock(response_mutex);
                    response += part_response;
                }
                {
                    std::lock_guard<std::mutex> lock(response_mutex);
                    response_ready = true;
                    cv.notify_one();
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        std::unique_lock<std::mutex> lock(response_mutex);
        cv.wait(lock, [&response_ready]() { return response_ready; });

        return response;
    }

    std::vector<std::string> tokenize(const std::string& input) {
        std::vector<std::string> tokens;
        size_t start = 0, end = 0;
        while ((end = input.find(' ', start)) != std::string::npos) {
            tokens.push_back(input.substr(start, end - start));
            start = end + 1;
        }
        tokens.push_back(input.substr(start));
        return tokens;
    }
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <model_path> <config_path>" << std::endl;
        return 1;
    }

    try {
        BrainChat brain_chat(argv[1], argv[2]);
        brain_chat.startChat();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
