#include "tokenizer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

SenseVoiceTokenizer::SenseVoiceTokenizer()
    : blank_id_(0), unk_id_(2), sos_id_(1), eos_id_(2) {
}

SenseVoiceTokenizer::~SenseVoiceTokenizer() {
}

bool SenseVoiceTokenizer::LoadVocab(const std::string &tokens_path) {
    std::ifstream file(tokens_path);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open tokens file: %s\n", tokens_path.c_str());
        return false;
    }

    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        // tokens.txt 格式: "token_text token_id"
        // 例如: "<blank> 0", "的 3", "一 4"

        size_t last_space = line.rfind(' ');
        if (last_space == std::string::npos) {
            continue;  // 跳过格式错误的行
        }

        std::string token = line.substr(0, last_space);
        std::string id_str = line.substr(last_space + 1);

        try {
            int token_id = std::stoi(id_str);
            id_to_token_[token_id] = token;

            // 识别特殊 token
            if (token == "<blank>") {
                blank_id_ = token_id;
            } else if (token == "<unk>") {
                unk_id_ = token_id;
            } else if (token == "<s>") {
                sos_id_ = token_id;
            } else if (token == "</s>") {
                eos_id_ = token_id;
            }
        } catch (const std::exception &e) {
            fprintf(stderr, "Error parsing line %d: %s\n", line_num, line.c_str());
        }

        line_num++;
    }

    file.close();
    printf("✅ Loaded %zu tokens from %s\n", id_to_token_.size(), tokens_path.c_str());
    printf("   blank_id=%d, unk_id=%d, sos_id=%d, eos_id=%d\n",
           blank_id_, unk_id_, sos_id_, eos_id_);

    return !id_to_token_.empty();
}

std::string SenseVoiceTokenizer::IdToToken(int token_id) const {
    auto it = id_to_token_.find(token_id);
    if (it != id_to_token_.end()) {
        return it->second;
    }
    return "<unk>";
}

std::string SenseVoiceTokenizer::Decode(const std::vector<int> &token_ids) const {
    std::string result;

    for (int token_id : token_ids) {
        // 跳过特殊 token
        if (token_id == blank_id_ || token_id == sos_id_ || token_id == eos_id_) {
            continue;
        }

        std::string token = IdToToken(token_id);

        // 跳过特殊标记
        if (token.find('<') == 0 && token.find('>') == token.length() - 1) {
            continue;
        }

        result += token;
    }

    return result;
}

std::string SenseVoiceTokenizer::DecodeLogits(
    const float *logits,
    int num_frames,
    int vocab_size
) const {
    std::vector<int> token_ids;

    // Greedy decoding with CTC collapse
    int prev_token = blank_id_;

    for (int t = 0; t < num_frames; t++) {
        const float *frame_logits = logits + t * vocab_size;

        // 找到最大概率的 token
        int max_id = 0;
        float max_score = frame_logits[0];

        for (int i = 1; i < vocab_size; i++) {
            if (frame_logits[i] > max_score) {
                max_score = frame_logits[i];
                max_id = i;
            }
        }

        // CTC collapse: 跳过连续重复的 token 和 blank
        if (max_id != blank_id_ && max_id != prev_token) {
            token_ids.push_back(max_id);
        }

        prev_token = max_id;
    }

    return Decode(token_ids);
}
