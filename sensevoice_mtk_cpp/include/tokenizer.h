#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>
#include <unordered_map>

/**
 * SenseVoice 分词器类
 */
class SenseVoiceTokenizer {
public:
    SenseVoiceTokenizer();
    ~SenseVoiceTokenizer();

    /**
     * 从文件加载词汇表
     * @param tokens_path tokens.txt 文件路径
     * @return true 表示成功, false 表示失败
     */
    bool LoadVocab(const std::string &tokens_path);

    /**
     * 将 token ID 转换为文本
     * @param token_id Token ID
     * @return 对应的文本字符串
     */
    std::string IdToToken(int token_id) const;

    /**
     * 解码 token ID 序列
     * @param token_ids Token ID 向量
     * @return 解码后的文本
     */
    std::string Decode(const std::vector<int> &token_ids) const;

    /**
     * 从 logits 中解码最优路径 (greedy decoding)
     * @param logits 模型输出 logits (num_frames x vocab_size)
     * @param num_frames 帧数
     * @param vocab_size 词汇表大小
     * @return 识别文本
     */
    std::string DecodeLogits(const float *logits, int num_frames, int vocab_size) const;

    int GetVocabSize() const { return id_to_token_.size(); }

private:
    std::unordered_map<int, std::string> id_to_token_;
    int blank_id_;  // CTC blank token ID
    int unk_id_;    // Unknown token ID
    int sos_id_;    // Start of sequence ID
    int eos_id_;    // End of sequence ID
};

#endif // TOKENIZER_H
