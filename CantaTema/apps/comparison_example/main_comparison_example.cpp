#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <memory>
#include <sstream>

#include "sound_system/sound_converter.hpp"
#include "speech_recognition/whisper_speech_recognition.hpp"
#include "speech_recognition/voice_quality_analyzer.hpp"
#include "file_handler/text_handler.hpp"
#include "file_handler/text_chunk_extractor.hpp"
#include "embeddings/llama_embedding_engine.hpp"
#include "similarity/faiss_similarity_search.hpp"
#include "models/manager_models.hpp"
#include "configuration/configuration_system.hpp"
#include "primitives/tool_paths.hpp"
#include "primitives/utils_logger.hpp"

namespace {

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
        // "ggml-large-v3-turbo.bin",
        // "ggml-large-v3.bin",
        // "ggml-small.bin",
        // "ggml-base.bin",
        "ggml-tiny.bin"
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
    std::cout << "========================================================================\n";
    std::cout << "                     CantaTema - ComparisonExample                      \n";
    std::cout << "        Stand-Alone Developer Audio-to-PDF Analysis Pipeline           \n";
    std::cout << "========================================================================\n\n";

    // -------------------------------------------------------------------------
    // Resolve sample files from example_data
    // -------------------------------------------------------------------------
    std::filesystem::path opus_file = locate_example_file("subject_es_1_p_1.opus");
    std::filesystem::path pdf_file = locate_example_file("subject_es_1.pdf");

    std::cout << "[INIT] Target OPUS Audio : " << opus_file.string() << "\n";
    std::cout << "[INIT] Target PDF Reference: " << pdf_file.string() << "\n\n";

    if (!std::filesystem::exists(opus_file)) {
        std::cerr << "[ERROR] OPUS file not found at: " << opus_file.string() << "\n";
        return 1;
    }
    if (!std::filesystem::exists(pdf_file)) {
        std::cerr << "[ERROR] PDF file not found at: " << pdf_file.string() << "\n";
        return 1;
    }

    // Prepare RAII temporary WAV file guard
    std::filesystem::path temp_wav_path = std::filesystem::temp_directory_path() /
        ("comparison_example_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".wav");
    TempFileGuard wav_guard(temp_wav_path);

    // =========================================================================
    // STEP 1: Convert OPUS file into WAV
    // =========================================================================
    std::cout << "========================================================================\n";
    std::cout << "  [STEP 1/5] Converting OPUS File into WAV Audio                       \n";
    std::cout << "========================================================================\n";
    auto t1_start = std::chrono::high_resolution_clock::now();

    bool conv_ok = SoundConverter::convert_opus_to_wav(opus_file.string(), temp_wav_path.string());
    auto t1_end = std::chrono::high_resolution_clock::now();
    double step1_ms = std::chrono::duration<double, std::milli>(t1_end - t1_start).count();

    if (!conv_ok || !std::filesystem::exists(temp_wav_path)) {
        std::cerr << "[ERROR] Failed to convert OPUS audio to WAV.\n";
        return 1;
    }

    auto wav_size_bytes = std::filesystem::file_size(temp_wav_path);
    std::cout << "[STEP 1] OPUS -> WAV conversion complete in " << std::fixed << std::setprecision(2) << step1_ms << " ms.\n";
    std::cout << "[STEP 1] Generated Temporary WAV: " << temp_wav_path.string() << " (" << wav_size_bytes << " bytes)\n\n";

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

    WhisperSpeechRecognition speech_rec;
    rst_code_e rst = speech_rec.initialize(speech_cfg);
    if (rst != RST_OK) {
        std::cerr << "[ERROR] Failed to initialize Whisper speech recognition: " << get_rst_txt(rst) << "\n";
        return 1;
    }

    rst = speech_rec.submit_task(temp_wav_path.string());
    if (rst != RST_OK) {
        std::cerr << "[ERROR] Speech transcription task failed: " << get_rst_txt(rst) << "\n";
        return 1;
    }

    std::vector<TranscriptSegment> segments;
    speech_rec.get_segments(segments);

    auto t2_end = std::chrono::high_resolution_clock::now();
    double step2_ms = std::chrono::duration<double, std::milli>(t2_end - t2_start).count();

    std::cout << "[STEP 2] Transcription finished in " << step2_ms << " ms. Total segments: " << segments.size() << "\n";
    std::cout << "------------------------------------------------------------------------\n";
    for (size_t i = 0; i < segments.size(); ++i) {
        const auto& seg = segments[i];
        std::cout << "  Seg #" << std::setw(2) << (i + 1) << " ["
                  << std::setw(6) << seg.start_time_ms << "ms - "
                  << std::setw(6) << seg.end_time_ms << "ms] "
                  << "(Conf: " << std::fixed << std::setprecision(2) << seg.confidence_score << "): "
                  << seg.text << "\n";
    }
    std::cout << "------------------------------------------------------------------------\n\n";

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
    std::cout << "------------------------------------------------------------------------\n";
    for (size_t i = 0; i < std::min<size_t>(doc_chunks.size(), 10); ++i) {
        const auto& chunk = doc_chunks[i];
        std::cout << "  Chunk #" << std::setw(2) << (i + 1)
                  << " [Weight: " << std::fixed << std::setprecision(2) << chunk.importance_weight
                  << " | B:" << chunk.is_bold << " I:" << chunk.is_italic << " U:" << chunk.is_underlined << " C:" << chunk.has_bg_color << "]: "
                  << (chunk.text.length() > 70 ? chunk.text.substr(0, 67) + "..." : chunk.text) << "\n";
    }
    if (doc_chunks.size() > 10) {
        std::cout << "  ... (" << (doc_chunks.size() - 10) << " more chunks extracted)\n";
    }
    std::cout << "------------------------------------------------------------------------\n\n";

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

    std::vector<std::string> pdf_texts;
    std::vector<float> pdf_weights;
    pdf_texts.reserve(doc_chunks.size());
    pdf_weights.reserve(doc_chunks.size());
    for (const auto& c : doc_chunks) {
        pdf_texts.push_back(c.text);
        pdf_weights.push_back(static_cast<float>(c.importance_weight));
    }

    std::vector<std::string> transcript_texts;
    transcript_texts.reserve(segments.size());
    for (const auto& s : segments) {
        transcript_texts.push_back(s.text);
    }

    std::cout << "[STEP 4] Generating vector embeddings for " << pdf_texts.size() << " PDF chunks & "
              << transcript_texts.size() << " transcript segments...\n";

    auto pdf_embeddings = embedding_engine.generate_embeddings_batch(pdf_texts);
    auto transcript_embeddings = embedding_engine.generate_embeddings_batch(transcript_texts);

    FaissSimilaritySearch similarity_search;
    similarity_search.index_transcript_embeddings(transcript_embeddings);

    float sim_threshold = ConfigurationSystem::getInstance().get_coverage_similarity_threshold();
    auto matches = similarity_search.search_pdf_matches(pdf_embeddings, pdf_weights, sim_threshold);

    auto t4_end = std::chrono::high_resolution_clock::now();
    double step4_ms = std::chrono::duration<double, std::milli>(t4_end - t4_start).count();

    size_t mentioned_count = 0;
    for (const auto& m : matches) {
        if (m.is_mentioned) mentioned_count++;
    }

    std::cout << "[STEP 4] Similarity search complete in " << step4_ms << " ms.\n";
    std::cout << "[STEP 4] Mentioned PDF Chunks: " << mentioned_count << " / " << matches.size()
              << " (Threshold: " << std::fixed << std::setprecision(2) << sim_threshold << ")\n";
    std::cout << "------------------------------------------------------------------------\n";
    for (size_t i = 0; i < matches.size(); ++i) {
        const auto& m = matches[i];
        std::cout << "  PDF Chunk #" << std::setw(2) << (i + 1)
                  << " -> Best Match Segment #" << std::setw(2) << (m.best_transcript_chunk_index >= 0 ? m.best_transcript_chunk_index + 1 : -1)
                  << " | Sim Score: " << std::fixed << std::setprecision(4) << m.similarity_score
                  << " | Mentioned: " << (m.is_mentioned ? "YES" : " NO")
                  << " | Missed Score: " << std::setprecision(2) << m.weighted_missed_score << "\n";
    }
    std::cout << "------------------------------------------------------------------------\n\n";

    // =========================================================================
    // STEP 5: Generate Metrics (Voice Metrics & Comparison Metrics)
    // =========================================================================
    std::cout << "========================================================================\n";
    std::cout << "  [STEP 5/5] Generating Voice & Coverage Metrics                        \n";
    std::cout << "========================================================================\n";
    auto t5_start = std::chrono::high_resolution_clock::now();

    VoiceQualityMetrics voice_metrics = VoiceQualityAnalyzer::analyze(segments);
    double coverage_pct = doc_chunks.empty() ? 0.0 : (static_cast<double>(mentioned_count) / static_cast<double>(doc_chunks.size())) * 100.0;

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
