/**
 * @file main_comparison_example.cpp
 * @brief Demonstration application showcasing audio decoding, Whisper speech recognition, PDF parsing, embedding generation, Faiss vector matching, and HTML report generation.
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <chrono>
#include <memory>
#include <sstream>
#include <algorithm>
#include <cctype>

#include <whisper.h>
#include <llama.h>

#include "sound_system/sound_converter.hpp"
#include "speech_recognition/whisper_speech_recognition.hpp"
#include "speech_recognition/voice_quality_analyzer.hpp"
#include "speech_recognition/transcript_sentence_stitcher.hpp"
#include "file_handler/text_handler.hpp"
#include "file_handler/text_chunk_extractor.hpp"
#include "embeddings/llama_embedding_engine.hpp"
#include "similarity/faiss_similarity_search.hpp"
#include "models/manager_models.hpp"
#include "configuration/configuration_system.hpp"
#include "primitives/tool_paths.hpp"
#include "primitives/utils_logger.hpp"
#include "reports/whisper_accuracy_visualizer.hpp"
#include "reports/text_comparison_visualizer.hpp"

namespace {

/**
 * @brief Custom log callback to filter out internal ggml/whisper/llama messages requested to be suppressed.
 */
void quiet_ggml_log_callback(ggml_log_level level, const char* text, void* user_data) {
    (void)level;
    (void)user_data;
    if (!text) return;
    std::string_view msg(text);
    if (msg.find("decode: cannot decode batches") != std::string_view::npos ||
        msg.find("output_reserve: reallocating output buffer") != std::string_view::npos) {
        return;
    }
}

/**
 * @brief RAII guard to delete temporary files when leaving scope or on failure.
 */
struct TempFileGuard {
    std::filesystem::path path;
    explicit TempFileGuard(const std::filesystem::path& p) : path(p) {}
    ~TempFileGuard() {
        if (!path.empty()) {
            std::error_code ec;
            if (std::filesystem::exists(path, ec)) {
                std::filesystem::remove(path, ec);
                std::cout << "[TempFileGuard] Cleaned up temporary file: " << path.string() << "\n";
            }
        }
    }
    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;
};

/**
 * @brief Helper to locate input data file in current directory or executable directory.
 */
std::filesystem::path locate_example_file(const std::string& filename) {
    std::vector<std::filesystem::path> candidates = {
        std::filesystem::current_path() / "CantaTema" / "example_data" / filename,
        std::filesystem::current_path() / "example_data" / filename,
        ToolPath::get_base_path() / "example_data" / filename,
        ToolPath::get_base_path() / "CantaTema" / "example_data" / filename,
        std::filesystem::current_path() / "build" / "bin" / "ComparisonExample" / "example_data" / filename
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return candidates[0];
}

/**
 * @brief Helper to locate Whisper model in example_data/data/models/whisper.
 */
std::filesystem::path locate_whisper_model() {
    std::vector<std::filesystem::path> dirs = {
        std::filesystem::current_path() / "CantaTema" / "example_data" / "data" / "models" / "whisper",
        ToolPath::get_base_path() / "CantaTema" / "example_data" / "data" / "models" / "whisper",
        std::filesystem::current_path() / "example_data" / "data" / "models" / "whisper",
        ToolPath::get_base_path() / "example_data" / "data" / "models" / "whisper",
        std::filesystem::current_path() / "build" / "bin" / "ComparisonExample" / "example_data" / "data" / "models" / "whisper"
    };

    std::vector<std::string> preferences = {
        "ggml-large-v3-turbo.bin",
        // "ggml-large-v3.bin",
        // "ggml-small.bin",
        // "ggml-base.bin",
        // "ggml-tiny.bin"
    };

    for (const auto& dir : dirs) {
        if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
            for (const auto& pref : preferences) {
                std::filesystem::path candidate = dir / pref;
                if (std::filesystem::exists(candidate)) {
                    return candidate;
                }
            }
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".bin") {
                    return entry.path();
                }
            }
        }
    }
    return "";
}

/**
 * @brief Helper to locate Llama embedding model in example_data/data/models/llama.
 */
std::filesystem::path locate_llama_model() {
    std::vector<std::filesystem::path> dirs = {
        std::filesystem::current_path() / "CantaTema" / "example_data" / "data" / "models" / "llama",
        ToolPath::get_base_path() / "CantaTema" / "example_data" / "data" / "models" / "llama",
        std::filesystem::current_path() / "example_data" / "data" / "models" / "llama",
        ToolPath::get_base_path() / "example_data" / "data" / "models" / "llama",
        std::filesystem::current_path() / "build" / "bin" / "ComparisonExample" / "example_data" / "data" / "models" / "llama"
    };

    for (const auto& dir : dirs) {
        if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".gguf") {
                    return entry.path();
                }
            }
        }
    }
    return "";
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    util_logger_init();
    whisper_log_set(quiet_ggml_log_callback, nullptr);
    llama_log_set(quiet_ggml_log_callback, nullptr);

    std::cout << "========================================================================\n";
    std::cout << "                     CantaTema - ComparisonExample                      \n";
    std::cout << "        Stand-Alone Developer Audio-to-PDF Analysis Pipeline           \n";
    std::cout << "========================================================================\n\n";

    // -------------------------------------------------------------------------
    // Resolve sample files from example_data
    // -------------------------------------------------------------------------
    // std::filesystem::path input_sound_file = locate_example_file("subject_es_1_p_1.opus");
    // std::filesystem::path pdf_file = locate_example_file("subject_es_1.pdf");

    std::filesystem::path input_sound_file{ "C:\\Users\\iscap\\Desktop\\temas_inspeccion\\tema_28.wav" };
    std::filesystem::path pdf_file{ "C:\\Users\\iscap\\Desktop\\temas_inspeccion\\tema_28.pdf" };

    std::cout << "[INIT] Target Sound File   : " << input_sound_file.string() << "\n";
    std::cout << "[INIT] Target PDF Reference: " << pdf_file.string() << "\n\n";

    if (!std::filesystem::exists(input_sound_file)) {
        std::cerr << "[ERROR] Sound file not found at: " << input_sound_file.string() << "\n";
        return 1;
    }
    if (!std::filesystem::exists(pdf_file)) {
        std::cerr << "[ERROR] PDF file not found at: " << pdf_file.string() << "\n";
        return 1;
    }

    std::string ext = input_sound_file.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::filesystem::path target_wav_path;
    std::unique_ptr<TempFileGuard> wav_guard;
    double step1_ms = 0.0;

    if (ext == ".opus") {
        std::cout << "========================================================================\n";
        std::cout << "  [STEP 1/5] Converting OPUS File into WAV Audio                       \n";
        std::cout << "========================================================================\n";
        auto t1_start = std::chrono::high_resolution_clock::now();

        std::filesystem::path temp_wav_path = std::filesystem::temp_directory_path() /
            ("comparison_example_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".wav");
        wav_guard = std::make_unique<TempFileGuard>(temp_wav_path);

        bool conv_ok = SoundConverter::convert_opus_to_wav(input_sound_file.string(), temp_wav_path.string());
        auto t1_end = std::chrono::high_resolution_clock::now();
        step1_ms = std::chrono::duration<double, std::milli>(t1_end - t1_start).count();

        if (!conv_ok || !std::filesystem::exists(temp_wav_path)) {
            std::cerr << "[ERROR] Failed to convert OPUS audio to WAV.\n";
            return 1;
        }

        auto wav_size_bytes = std::filesystem::file_size(temp_wav_path);
        std::cout << "[STEP 1] OPUS -> WAV conversion complete in " << std::fixed << std::setprecision(2) << step1_ms << " ms.\n";
        std::cout << "[STEP 1] Generated Temporary WAV: " << temp_wav_path.string() << " (" << wav_size_bytes << " bytes)\n\n";

        target_wav_path = temp_wav_path;
    } else if (ext == ".wav") {
        std::cout << "========================================================================\n";
        std::cout << "  [STEP 1/5] Audio is WAV format (No Conversion Required)             \n";
        std::cout << "========================================================================\n";
        std::cout << "[STEP 1] Using input WAV file directly: " << input_sound_file.string() << "\n\n";

        target_wav_path = input_sound_file;
    } else {
        std::cerr << "[ERROR] Unsupported audio format/extension '" << ext << "'. Finalizing execution.\n";
        return 1;
    }

    // =========================================================================
    // STEP 2: Convert Audio to Text via Whisper
    // =========================================================================
    std::cout << "========================================================================\n";
    std::cout << "  [STEP 2/5] Speech Recognition via Whisper.cpp                        \n";
    std::cout << "========================================================================\n";
    auto t2_start = std::chrono::high_resolution_clock::now();

    ManagerModels model_mgr;
    std::string whisper_model;
    std::filesystem::path example_whisper_path = locate_whisper_model();
    if (!example_whisper_path.empty()) {
        whisper_model = example_whisper_path.string();
        std::cout << "[STEP 2] Using Whisper Model from example_data: " << whisper_model << "\n";
    } else {
        whisper_model = model_mgr.auto_select_whisper_model();
        std::cout << "[STEP 2] Auto-selected Whisper Model: " << whisper_model << "\n";
        if (model_mgr.local_is_whisper_model_available(whisper_model) != RST_OK) {
            std::cout << "[STEP 2] Model '" << whisper_model << "' not found locally. Downloading from Hugging Face...\n";
            rst_code_e dl_rst = model_mgr.network_download_model(ModelType::Whisper, whisper_model, [](const DownloadProgress& p) {
                if (p.total_bytes > 0) {
                    float pct = (float)p.downloaded_bytes / (float)p.total_bytes * 100.0f;
                    std::cout << "\r[STEP 2] Downloading Whisper Model (" << p.file_name << "): " << (int)pct << "%" << std::flush;
                }
            });
            std::cout << "\n";
            if (dl_rst != RST_OK) {
                std::cerr << "[ERROR] Failed to download Whisper model.\n";
                return 1;
            }
        }
    }

    ISpeechRecognition::speech_recognition_config_t speech_cfg;
    speech_cfg.model_name = whisper_model;
    speech_cfg.language = "es";
    speech_cfg.use_gpu = ConfigurationSystem::getInstance().get_whisper_use_gpu();
    speech_cfg.progress_callback = [](int pct) {
        std::cout << "\r[STEP 2] Transcribing audio via Whisper: " << std::setw(3) << pct << "%" << std::flush;
    };

    WhisperSpeechRecognition speech_rec;
    rst_code_e rst = speech_rec.initialize(speech_cfg);
    if (rst != RST_OK) {
        std::cerr << "\n[ERROR] Failed to initialize Whisper speech recognition: " << get_rst_txt(rst) << "\n";
        return 1;
    }

    rst = speech_rec.submit_task(target_wav_path.string());
    std::cout << "\n";
    if (rst != RST_OK) {
        std::cerr << "[ERROR] Speech transcription task failed: " << get_rst_txt(rst) << "\n";
        return 1;
    }

    std::vector<TranscriptSegment> segments;
    speech_rec.get_segments(segments);

    auto t2_end = std::chrono::high_resolution_clock::now();
    double step2_ms = std::chrono::duration<double, std::milli>(t2_end - t2_start).count();

    std::cout << "[STEP 2] Transcription finished in " << std::fixed << std::setprecision(2) << step2_ms << " ms. Total segments: " << segments.size() << "\n";

    // Save full converted text on a single file (replaced on every execution)
    std::filesystem::path converted_text_path = std::filesystem::current_path() / "converted_text.txt";
    std::ofstream text_file(converted_text_path, std::ios::trunc);
    if (text_file.is_open()) {
        for (size_t i = 0; i < segments.size(); ++i) {
            text_file << segments[i].text;
            if (i + 1 < segments.size()) {
                text_file << "\n";
            }
        }
        text_file.close();
        std::cout << "[STEP 2] Saved full converted text to: " << converted_text_path.string() << "\n";
    } else {
        std::cerr << "[ERROR] Failed to write converted text file: " << converted_text_path.string() << "\n";
    }

    // Save text with all metrics (time, chunk, conf, logprob) on another file (replaced on every execution)
    std::filesystem::path converted_metrics_path = std::filesystem::current_path() / "converted_text_metrics.txt";
    std::ofstream metrics_file(converted_metrics_path, std::ios::trunc);
    if (metrics_file.is_open()) {
        for (size_t i = 0; i < segments.size(); ++i) {
            const auto& seg = segments[i];
            metrics_file << "Chunk #" << std::setw(2) << (i + 1)
                         << " | Time: [" << std::setw(6) << seg.start_time_ms << "ms - "
                         << std::setw(6) << seg.end_time_ms << "ms]"
                         << " | Duration: " << (seg.end_time_ms - seg.start_time_ms) << "ms"
                         << " | Conf: " << std::fixed << std::setprecision(2) << seg.confidence_score
                         << " | Logprob: " << std::setprecision(4) << seg.avg_logprob
                         << " | Text: " << seg.text << "\n";
        }
        metrics_file.close();
        std::cout << "[STEP 2] Saved text with metrics to: " << converted_metrics_path.string() << "\n\n";
    } else {
        std::cerr << "[ERROR] Failed to write converted text metrics file: " << converted_metrics_path.string() << "\n\n";
    }

    // =========================================================================
    // STEP 3: Extract Text and Features from PDF
    // =========================================================================
    std::cout << "========================================================================\n";
    std::cout << "  [STEP 3/5] Extracting Text & Features from Reference PDF              \n";
    std::cout << "========================================================================\n";
    auto t3_start = std::chrono::high_resolution_clock::now();

    TextFileHandler text_handler(pdf_file.string());
    text_handler.parse();

    std::cout << "[STEP 3] PDF Document Pages: " << text_handler.get_number_of_pages() << "\n";

    TextChunkExtractor chunk_extractor;
    std::vector<DocumentChunk> doc_chunks;
    rst = chunk_extractor.extract_chunks(text_handler, doc_chunks);
    if (rst != RST_OK || doc_chunks.empty()) {
        std::cerr << "[ERROR] Failed to extract chunks from PDF: " << get_rst_txt(rst) << "\n";
        return 1;
    }

    auto t3_end = std::chrono::high_resolution_clock::now();
    double step3_ms = std::chrono::duration<double, std::milli>(t3_end - t3_start).count();

    std::cout << "[STEP 3] Extracted " << doc_chunks.size() << " sentence chunks in " << step3_ms << " ms.\n";

    // Save extracted PDF chunks to file (replaced on every execution)
    std::filesystem::path pdf_chunks_path = std::filesystem::current_path() / "pdf_chunks.txt";
    std::ofstream pdf_chunks_file(pdf_chunks_path, std::ios::trunc);
    if (pdf_chunks_file.is_open()) {
        for (size_t i = 0; i < doc_chunks.size(); ++i) {
            const auto& chunk = doc_chunks[i];
            pdf_chunks_file << "Chunk #" << std::setw(3) << (i + 1)
                            << " | Weight: " << std::fixed << std::setprecision(2) << chunk.importance_weight
                            << " | Bold: " << (chunk.is_bold ? "YES" : " NO")
                            << " | Italic: " << (chunk.is_italic ? "YES" : " NO")
                            << " | Underline: " << (chunk.is_underlined ? "YES" : " NO")
                            << " | BgColor: " << (chunk.has_bg_color ? "YES" : " NO")
                            << " | Text: " << chunk.text << "\n";
        }
        pdf_chunks_file.close();
        std::cout << "[STEP 3] Saved PDF chunk detection to: " << pdf_chunks_path.string() << "\n\n";
    } else {
        std::cerr << "[ERROR] Failed to write PDF chunk detection file: " << pdf_chunks_path.string() << "\n\n";
    }

    // =========================================================================
    // STEP 4: Compare Both Texts via Embeddings & Similarity Search
    // =========================================================================
    std::cout << "========================================================================\n";
    std::cout << "  [STEP 4/5] Text Comparison via Llama Embeddings & Faiss Search        \n";
    std::cout << "========================================================================\n";
    auto t4_start = std::chrono::high_resolution_clock::now();

    std::filesystem::path llama_path = locate_llama_model();
    if (!llama_path.empty()) {
        std::cout << "[STEP 4] Using Llama Embedding Model from example_data: " << llama_path.string() << "\n";
    } else {
        std::string llama_model = model_mgr.auto_select_llama_model();
        std::cout << "[STEP 4] Auto-selected Llama Embedding Model: " << llama_model << "\n";

        if (model_mgr.local_is_llama_model_available(llama_model) != RST_OK) {
            std::cout << "[STEP 4] Model '" << llama_model << "' not found locally. Downloading from Hugging Face...\n";
            rst_code_e dl_rst = model_mgr.network_download_model(ModelType::Llama, llama_model, [](const DownloadProgress& p) {
                if (p.total_bytes > 0) {
                    float pct = (float)p.downloaded_bytes / (float)p.total_bytes * 100.0f;
                    std::cout << "\r[STEP 4] Downloading Llama Model (" << p.file_name << "): " << (int)pct << "%" << std::flush;
                }
            });
            std::cout << "\n";
            if (dl_rst != RST_OK) {
                std::cerr << "[ERROR] Failed to download Llama embedding model.\n";
                return 1;
            }
        }

        llama_path = ToolPath::get_path_for_models_llama() / llama_model;
        if (!std::filesystem::exists(llama_path)) {
            std::filesystem::path alt_path = ToolPath::get_path_for_models_llama() / (llama_model + ".gguf");
            if (std::filesystem::exists(alt_path)) {
                llama_path = alt_path;
            }
        }
    }

    LlamaEmbeddingEngine embedding_engine;
    if (!embedding_engine.load_model(llama_path)) {
        std::cerr << "[ERROR] Failed to load Llama embedding model from: " << llama_path.string() << "\n";
        return 1;
    }

    std::vector<std::string> pdf_embedding_texts;
    std::vector<std::string> pdf_display_texts;
    std::vector<float> pdf_weights;
    pdf_embedding_texts.reserve(doc_chunks.size());
    pdf_display_texts.reserve(doc_chunks.size());
    pdf_weights.reserve(doc_chunks.size());
    for (const auto& c : doc_chunks) {
        pdf_embedding_texts.push_back(c.contextual_text.empty() ? c.text : c.contextual_text);
        pdf_display_texts.push_back(c.text);
        pdf_weights.push_back(static_cast<float>(c.importance_weight));
    }

    std::vector<TranscriptSegment> stitched_segments = TranscriptSentenceStitcher::stitch_segments(segments);
    std::cout << "[STEP 4] Heuristic sentence stitching: reconstructed " << segments.size() << " acoustic segments into "
              << stitched_segments.size() << " complete sentence chunks.\n";

    std::vector<std::string> transcript_texts;
    transcript_texts.reserve(stitched_segments.size());
    for (const auto& s : stitched_segments) {
        transcript_texts.push_back(s.text);
    }

    std::cout << "[STEP 4] Generating vector embeddings for " << pdf_embedding_texts.size() << " PDF chunks & "
              << transcript_texts.size() << " transcript segments...\n";

    std::vector<std::vector<float>> pdf_embeddings;
    pdf_embeddings.reserve(pdf_embedding_texts.size());
    for (size_t i = 0; i < pdf_embedding_texts.size(); ++i) {
        pdf_embeddings.push_back(embedding_engine.generate_embedding(pdf_embedding_texts[i], EmbeddingRole::PASSAGE));
        int pct = static_cast<int>((i + 1) * 100 / pdf_embedding_texts.size());
        std::cout << "\r[STEP 4] Processing PDF Embeddings (" << (i + 1) << "/" << pdf_embedding_texts.size() << "): " << std::setw(3) << pct << "%" << std::flush;
    }
    std::cout << "\n";

    std::vector<std::vector<float>> transcript_embeddings;
    transcript_embeddings.reserve(transcript_texts.size());
    for (size_t i = 0; i < transcript_texts.size(); ++i) {
        transcript_embeddings.push_back(embedding_engine.generate_embedding(transcript_texts[i], EmbeddingRole::QUERY));
        int pct = static_cast<int>((i + 1) * 100 / transcript_texts.size());
        std::cout << "\r[STEP 4] Processing Transcript Embeddings (" << (i + 1) << "/" << transcript_texts.size() << "): " << std::setw(3) << pct << "%" << std::flush;
    }
    std::cout << "\n";

    FaissSimilaritySearch similarity_search;
    similarity_search.index_transcript_embeddings(transcript_embeddings);

    float sim_threshold = ConfigurationSystem::getInstance().get_coverage_similarity_threshold();
    SimilaritySearchOptions options;
    options.similarity_threshold = sim_threshold;
    options.numeric_boost = ConfigurationSystem::getInstance().get_coverage_numeric_boost();
    options.numeric_mismatch_penalty = ConfigurationSystem::getInstance().get_coverage_numeric_mismatch_penalty();
    options.temporal_penalty_weight = ConfigurationSystem::getInstance().get_coverage_temporal_penalty_weight();
    options.short_chunk_word_threshold = ConfigurationSystem::getInstance().get_coverage_short_chunk_word_threshold();
    options.lexical_mismatch_scaling_factor = ConfigurationSystem::getInstance().get_coverage_lexical_mismatch_scaling_factor();
    options.lexical_boost_weight = ConfigurationSystem::getInstance().get_coverage_lexical_boost_weight();

    auto matches = similarity_search.search_pdf_matches_advanced(
        pdf_embeddings,
        pdf_display_texts,
        transcript_texts,
        pdf_weights,
        options
    );

    auto t4_end = std::chrono::high_resolution_clock::now();
    double step4_ms = std::chrono::duration<double, std::milli>(t4_end - t4_start).count();

    size_t mentioned_count = 0;
    for (const auto& m : matches) {
        if (m.is_mentioned) mentioned_count++;
    }

    std::cout << "[STEP 4] Similarity search complete in " << step4_ms << " ms.\n";
    std::cout << "[STEP 4] Mentioned PDF Chunks: " << mentioned_count << " / " << matches.size()
              << " (Threshold: " << std::fixed << std::setprecision(2) << sim_threshold << ")\n";

    // Save PDF comparison results to file (replaced on every execution)
    std::filesystem::path pdf_comp_path = std::filesystem::current_path() / "pdf_comparison.txt";
    std::ofstream pdf_comp_file(pdf_comp_path, std::ios::trunc);
    if (pdf_comp_file.is_open()) {
        for (size_t i = 0; i < matches.size(); ++i) {
            const auto& m = matches[i];
            std::string matched_transcript = "[NOT MENTIONED]";
            std::string voice_chunks_str = "-1";
            if (m.best_transcript_chunk_index >= 0 && static_cast<size_t>(m.best_transcript_chunk_index) < stitched_segments.size()) {
                matched_transcript = stitched_segments[m.best_transcript_chunk_index].text;
                const auto& src_indices = stitched_segments[m.best_transcript_chunk_index].source_segment_indices;
                if (!src_indices.empty()) {
                    if (src_indices.size() == 1) {
                        voice_chunks_str = "#" + std::to_string(src_indices.front() + 1);
                    } else {
                        voice_chunks_str = "#" + std::to_string(src_indices.front() + 1) + "-#" + std::to_string(src_indices.back() + 1);
                    }
                } else {
                    voice_chunks_str = "#" + std::to_string(m.best_transcript_chunk_index + 1);
                }
            }

            pdf_comp_file << "PDF Chunk #" << std::setw(3) << (i + 1)
                          << " | Sim Score: " << std::fixed << std::setprecision(4) << m.similarity_score
                          << " | Mentioned: " << (m.is_mentioned ? "YES" : " NO")
                          << " | Missed Score: " << std::setprecision(2) << m.weighted_missed_score
                          << " | Match Voice Seg: " << std::setw(8) << voice_chunks_str
                          << " | PDF Text: " << (i < doc_chunks.size() ? doc_chunks[i].text : "")
                          << " | Transcript Text: " << matched_transcript << "\n";
        }
        pdf_comp_file.close();
        std::cout << "[STEP 4] Saved PDF comparison results to: " << pdf_comp_path.string() << "\n\n";
    } else {
        std::cerr << "[ERROR] Failed to write PDF comparison file: " << pdf_comp_path.string() << "\n\n";
    }

    // =========================================================================
    // STEP 5: Generate Metrics (Voice Metrics & Comparison Metrics)
    // =========================================================================
    std::cout << "========================================================================\n";
    std::cout << "  [STEP 5/5] Generating Voice & Coverage Metrics                        \n";
    std::cout << "========================================================================\n";
    auto t5_start = std::chrono::high_resolution_clock::now();

    VoiceQualityMetrics voice_metrics = VoiceQualityAnalyzer::analyze(segments);
    double coverage_pct = doc_chunks.empty() ? 0.0 : (static_cast<double>(mentioned_count) / static_cast<double>(doc_chunks.size())) * 100.0;

    // 1. Generate Whisper Accuracy HTML report
    WhisperAccuracyVisualizer whisper_vis;
    WhisperAccuracyInput whisper_input;
    whisper_input.audio_filepath = input_sound_file.string();
    whisper_input.model_name = whisper_model;
    whisper_input.language = speech_cfg.language;
    whisper_input.total_duration_ms = segments.empty() ? 0 : segments.back().end_time_ms;
    whisper_input.processing_time_ms = static_cast<uint64_t>(step2_ms);
    whisper_input.speech_rate_wpm = static_cast<float>(voice_metrics.speech_rate_wpm);
    whisper_input.clarity_score = static_cast<float>(voice_metrics.clarity_score);
    whisper_input.overall_confidence = static_cast<float>(voice_metrics.clarity_score / 100.0);
    whisper_input.segments = segments;

    std::string whisper_html;
    if (whisper_vis.generate_html(whisper_input, whisper_html) == RST_OK) {
        std::filesystem::path whisper_html_path = std::filesystem::current_path() / "whisper_accuracy_report.html";
        std::ofstream whisper_html_file(whisper_html_path, std::ios::trunc);
        if (whisper_html_file.is_open()) {
            whisper_html_file << whisper_html;
            whisper_html_file.close();
            std::cout << "[STEP 5] Saved Whisper accuracy HTML report to: " << whisper_html_path.string() << "\n";
        }
    }

    // 2. Generate Dual-Column Text Comparison HTML report
    TextComparisonVisualizer comp_vis;
    TextComparisonInput comp_input;
    comp_input.document_title = pdf_file.filename().string();
    comp_input.reference_filepath = pdf_file.string();
    comp_input.audio_filepath = input_sound_file.string();
    comp_input.whisper_model = whisper_model;
    comp_input.llama_model = llama_path.filename().string();
    comp_input.overall_coverage_pct = coverage_pct;
    comp_input.total_ref_chunks = doc_chunks.size();
    comp_input.mentioned_chunks = mentioned_count;
    comp_input.not_clear_chunks = 0;
    comp_input.not_mentioned_chunks = doc_chunks.size() - mentioned_count;
    comp_input.threshold_mentioned = sim_threshold;

    for (size_t i = 0; i < matches.size(); ++i) {
        const auto& m = matches[i];
        TextComparisonInput::ReferenceItem ref_item;
        ref_item.id = i;
        ref_item.text = (i < doc_chunks.size()) ? doc_chunks[i].text : "";
        ref_item.importance_weight = (i < doc_chunks.size()) ? static_cast<float>(doc_chunks[i].importance_weight) : 1.0f;
        ref_item.coverage_status = m.is_mentioned ? coverage_level_e::MENTIONED : coverage_level_e::NOT_MENTIONED;
        ref_item.similarity_score = m.similarity_score;
        ref_item.matched_transcript_index = m.best_transcript_chunk_index;
        ref_item.word_alignment = m.word_alignment;
        ref_item.word_recall_score = m.word_recall_score;
        comp_input.reference_items.push_back(ref_item);
    }

    for (size_t j = 0; j < stitched_segments.size(); ++j) {
        const auto& seg = stitched_segments[j];
        TextComparisonInput::TranscriptItem ts_item;
        ts_item.id = j;
        ts_item.start_time_ms = seg.start_time_ms;
        ts_item.end_time_ms = seg.end_time_ms;
        ts_item.text = seg.text;
        ts_item.confidence_score = seg.confidence_score;
        
        ts_item.primary_matched_ref_index = -1;
        for (size_t i = 0; i < matches.size(); ++i) {
            if (matches[i].best_transcript_chunk_index == static_cast<int>(j) && matches[i].is_mentioned) {
                ts_item.primary_matched_ref_index = static_cast<int>(i);
                break;
            }
        }
        comp_input.transcript_items.push_back(ts_item);
    }

    std::string comp_html;
    if (comp_vis.generate_html(comp_input, comp_html) == RST_OK) {
        std::filesystem::path comp_html_path = std::filesystem::current_path() / "dual_column_comparison_report.html";
        std::ofstream comp_html_file(comp_html_path, std::ios::trunc);
        if (comp_html_file.is_open()) {
            comp_html_file << comp_html;
            comp_html_file.close();
            std::cout << "[STEP 5] Saved dual-column text comparison HTML report to: " << comp_html_path.string() << "\n";
        }
    }


    auto t5_end = std::chrono::high_resolution_clock::now();
    double step5_ms = std::chrono::duration<double, std::milli>(t5_end - t5_start).count();

    std::cout << "\n========================================================================\n";
    std::cout << "                         FINAL METRICS SUMMARY                          \n";
    std::cout << "========================================================================\n";
    std::cout << "  VOICE METRICS:\n";
    std::cout << "    - Speech Rate (WPM)  : " << std::fixed << std::setprecision(2) << voice_metrics.speech_rate_wpm << " WPM\n";
    std::cout << "    - Clarity Score      : " << voice_metrics.clarity_score << " / 100.0\n";
    std::cout << "    - Pacing Score       : " << voice_metrics.pacing_score << " / 100.0\n";
    std::cout << "    - Overall Quality    : " << voice_metrics.overall_quality_score << " / 100.0\n";
    std::cout << "------------------------------------------------------------------------\n";
    std::cout << "  COMPARISON / COVERAGE METRICS:\n";
    std::cout << "    - Total PDF Chunks   : " << doc_chunks.size() << "\n";
    std::cout << "    - Mentioned Chunks   : " << mentioned_count << "\n";
    std::cout << "    - Unmentioned Chunks : " << (doc_chunks.size() - mentioned_count) << "\n";
    std::cout << "    - Coverage Score     : " << std::fixed << std::setprecision(2) << coverage_pct << " %\n";
    std::cout << "------------------------------------------------------------------------\n";
    std::cout << "  PIPELINE EXECUTION TIMINGS:\n";
    std::cout << "    - Step 1 (OPUS -> WAV)      : " << std::setw(8) << step1_ms << " ms\n";
    std::cout << "    - Step 2 (Whisper STT)       : " << std::setw(8) << step2_ms << " ms\n";
    std::cout << "    - Step 3 (PDF Ingestion)     : " << std::setw(8) << step3_ms << " ms\n";
    std::cout << "    - Step 4 (Vector Match)      : " << std::setw(8) << step4_ms << " ms\n";
    std::cout << "    - Step 5 (Metrics Calc)      : " << std::setw(8) << step5_ms << " ms\n";
    std::cout << "    ---------------------------------------------------\n";
    std::cout << "    - Total Pipeline Duration    : " << (step1_ms + step2_ms + step3_ms + step4_ms + step5_ms) << " ms\n";
    std::cout << "========================================================================\n\n";

    std::cout << "[SUCCESS] ComparisonExample pipeline execution completed successfully.\n";
    return 0;
}
