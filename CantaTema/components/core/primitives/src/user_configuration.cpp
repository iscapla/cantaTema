#include "primitives/user_configuration.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

UserConfiguration::UserConfiguration() {
    set_default_values();
}

void UserConfiguration::set_default_values() {
    whisper = WhisperConfig{};
    reference_extraction = ReferenceExtractionConfig{};
    comparison = ComparisonConfig{};
}

std::string UserConfiguration::to_json() const {
    std::ostringstream ss;
    ss << std::boolalpha;
    ss << "{"
       << "\"whisper\":{"
       << "\"model_name\":\"" << whisper.model_name << "\","
       << "\"language\":\"" << whisper.language << "\","
       << "\"use_gpu\":" << whisper.use_gpu << ","
       << "\"enable_timestamps\":" << whisper.enable_timestamps << ","
       << "\"enable_confidence\":" << whisper.enable_confidence << ","
       << "\"thread_count\":" << whisper.thread_count
       << "},"
       << "\"reference_extraction\":{"
       << "\"max_pdf_page_count\":" << reference_extraction.max_pdf_page_count << ","
       << "\"importance_weight_bold\":" << std::fixed << std::setprecision(2) << reference_extraction.importance_weight_bold << ","
       << "\"importance_weight_italic\":" << reference_extraction.importance_weight_italic << ","
       << "\"importance_weight_underline\":" << reference_extraction.importance_weight_underline << ","
       << "\"importance_weight_bg_color\":" << reference_extraction.importance_weight_bg_color << ","
       << "\"min_chunk_word_count\":" << reference_extraction.min_chunk_word_count
       << "},"
       << "\"comparison\":{"
       << "\"embedding_model_name\":\"" << comparison.embedding_model_name << "\","
       << "\"embedding_gpu_offload_layers\":" << comparison.embedding_gpu_offload_layers << ","
       << "\"use_role_prefixes\":" << comparison.use_role_prefixes << ","
       << "\"passage_prefix\":\"" << comparison.passage_prefix << "\","
       << "\"query_prefix\":\"" << comparison.query_prefix << "\","
       << "\"similarity_threshold\":" << comparison.similarity_threshold << ","
       << "\"numeric_boost\":" << comparison.numeric_boost << ","
       << "\"numeric_mismatch_penalty\":" << comparison.numeric_mismatch_penalty << ","
       << "\"temporal_penalty_weight\":" << comparison.temporal_penalty_weight << ","
       << "\"short_chunk_word_threshold\":" << comparison.short_chunk_word_threshold << ","
       << "\"lexical_mismatch_scaling_factor\":" << comparison.lexical_mismatch_scaling_factor << ","
       << "\"speed_weight\":" << comparison.speed_weight << ","
       << "\"clarity_weight\":" << comparison.clarity_weight << ","
       << "\"pacing_weight\":" << comparison.pacing_weight
       << "}"
       << "}";
    return ss.str();
}

static std::string extract_string_val(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    size_t start = pos + needle.length();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}

static float extract_float_val(const std::string& json, const std::string& key, float default_val) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return default_val;
    size_t start = pos + needle.length();
    try {
        return std::stof(json.substr(start));
    } catch (...) {
        return default_val;
    }
}

static int extract_int_val(const std::string& json, const std::string& key, int default_val) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return default_val;
    size_t start = pos + needle.length();
    try {
        return std::stoi(json.substr(start));
    } catch (...) {
        return default_val;
    }
}

static bool extract_bool_val(const std::string& json, const std::string& key, bool default_val) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return default_val;
    size_t start = pos + needle.length();
    if (json.compare(start, 4, "true") == 0) return true;
    if (json.compare(start, 5, "false") == 0) return false;
    return default_val;
}

bool UserConfiguration::from_json(const std::string& json_str) {
    if (json_str.empty() || json_str[0] != '{') {
        return false;
    }

    std::string w_model = extract_string_val(json_str, "model_name");
    if (!w_model.empty()) whisper.model_name = w_model;

    std::string w_lang = extract_string_val(json_str, "language");
    if (!w_lang.empty()) whisper.language = w_lang;

    whisper.use_gpu = extract_bool_val(json_str, "use_gpu", whisper.use_gpu);
    whisper.enable_timestamps = extract_bool_val(json_str, "enable_timestamps", whisper.enable_timestamps);
    whisper.enable_confidence = extract_bool_val(json_str, "enable_confidence", whisper.enable_confidence);
    whisper.thread_count = extract_int_val(json_str, "thread_count", whisper.thread_count);

    reference_extraction.max_pdf_page_count = extract_int_val(json_str, "max_pdf_page_count", reference_extraction.max_pdf_page_count);
    reference_extraction.importance_weight_bold = extract_float_val(json_str, "importance_weight_bold", reference_extraction.importance_weight_bold);
    reference_extraction.importance_weight_italic = extract_float_val(json_str, "importance_weight_italic", reference_extraction.importance_weight_italic);
    reference_extraction.importance_weight_underline = extract_float_val(json_str, "importance_weight_underline", reference_extraction.importance_weight_underline);
    reference_extraction.importance_weight_bg_color = extract_float_val(json_str, "importance_weight_bg_color", reference_extraction.importance_weight_bg_color);
    reference_extraction.min_chunk_word_count = extract_int_val(json_str, "min_chunk_word_count", static_cast<int>(reference_extraction.min_chunk_word_count));

    std::string e_model = extract_string_val(json_str, "embedding_model_name");
    if (!e_model.empty()) comparison.embedding_model_name = e_model;

    comparison.embedding_gpu_offload_layers = extract_int_val(json_str, "embedding_gpu_offload_layers", comparison.embedding_gpu_offload_layers);
    comparison.use_role_prefixes = extract_bool_val(json_str, "use_role_prefixes", comparison.use_role_prefixes);

    std::string p_prefix = extract_string_val(json_str, "passage_prefix");
    if (!p_prefix.empty()) comparison.passage_prefix = p_prefix;

    std::string q_prefix = extract_string_val(json_str, "query_prefix");
    if (!q_prefix.empty()) comparison.query_prefix = q_prefix;

    comparison.similarity_threshold = extract_float_val(json_str, "similarity_threshold", comparison.similarity_threshold);
    comparison.numeric_boost = extract_float_val(json_str, "numeric_boost", comparison.numeric_boost);
    comparison.numeric_mismatch_penalty = extract_float_val(json_str, "numeric_mismatch_penalty", comparison.numeric_mismatch_penalty);
    comparison.temporal_penalty_weight = extract_float_val(json_str, "temporal_penalty_weight", comparison.temporal_penalty_weight);
    comparison.short_chunk_word_threshold = extract_int_val(json_str, "short_chunk_word_threshold", static_cast<int>(comparison.short_chunk_word_threshold));
    comparison.lexical_mismatch_scaling_factor = extract_float_val(json_str, "lexical_mismatch_scaling_factor", comparison.lexical_mismatch_scaling_factor);
    comparison.speed_weight = extract_float_val(json_str, "speed_weight", comparison.speed_weight);
    comparison.clarity_weight = extract_float_val(json_str, "clarity_weight", comparison.clarity_weight);
    comparison.pacing_weight = extract_float_val(json_str, "pacing_weight", comparison.pacing_weight);

    return true;
}
