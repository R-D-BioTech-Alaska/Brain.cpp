#pragma once

#include <string>
#include <memory>
#include <iostream>
#include "ggml_model.h"
#include "gguf_model.h"

class ModelLoader {
public:
    ModelLoader(const std::string& model_path)
        : model_path_(model_path), loaded_(false) {
        loadModel();
    }

    bool isLoaded() const { return loaded_; }
    void* getModel() const { return model_data_.get(); }

private:
    std::string model_path_;
    bool loaded_;
    std::unique_ptr<void> model_data_;

    void loadModel() {
        if (model_path_.substr(model_path_.find_last_of(".") + 1) == "ggml") {
            model_data_ = ggml_load_model(model_path_);
        } else if (model_path_.substr(model_path_.find_last_of(".") + 1) == "gguf") {
            model_data_ = gguf_load_model(model_path_);
        } else {
            throw std::runtime_error("Unsupported model format.");
        }
        loaded_ = (model_data_ != nullptr);
    }
};
