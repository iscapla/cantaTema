#include "operations/operation_coverage.hpp"

#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "configuration/configuration_system.hpp"
#include "models/manager_models.hpp"
#include "file_handler/text_handler.hpp"
#include "file_handler/text_chunk_extractor.hpp"
#include "speech_recognition/voice_quality_analyzer.hpp"
#include "primitives/utils_logger.hpp"

#include "database/db_coverage.hpp"
#include "file_handler/file_handler.hpp"
#include "speech_recognition/whisper_speech_recognition.hpp"
#include "embeddings/llama_embedding_engine.hpp"
#include "similarity/faiss_similarity_search.hpp"
#include "operations/operation_subject.hpp"
#include "operations/operation_practice_event.hpp"
#include "operations/operation_user_metrics.hpp"
#include "operations/operation_category.hpp"

OperationCoverage::OperationCoverage(
    std::shared_ptr<IDatabase> db,
    std::shared_ptr<IOperationSubject> subject_op,
    std::shared_ptr<IOperationPracticeEvent> practice_op,
    std::shared_ptr<IFileHandler> file_handler,
    std::shared_ptr<ISpeechRecognition> speech_recognition,
    std::shared_ptr<IEmbeddingEngine> embedding_engine,
    std::shared_ptr<ISimilaritySearch> similarity_search
)
    : m_db(std::move(db)),
      m_subject_op(std::move(subject_op)),
      m_practice_op(std::move(practice_op)),
      m_file_handler(std::move(file_handler)),
      m_speech_recognition(std::move(speech_recognition)),
      m_embedding_engine(std::move(embedding_engine)),
      m_similarity_search(std::move(similarity_search))
{
    if (m_db && m_subject_op && m_practice_op) {
        if (!m_file_handler) m_file_handler = std::make_shared<FileHandler>();
        if (!m_speech_recognition) m_speech_recognition = std::make_shared<WhisperSpeechRecognition>();
        if (!m_embedding_engine) m_embedding_engine = std::make_shared<LlamaEmbeddingEngine>();
        if (!m_similarity_search) m_similarity_search = std::make_shared<FaissSimilaritySearch>();
    }
}

std::string OperationCoverage::generate_execution_uuid() const
{
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(100000, 999999);

    std::ostringstream ss;
    ss << "exec-" << timestamp << "-" << dis(gen);
    return ss.str();
}

rst_code_e OperationCoverage::analyze_practice_coverage(
    const std::shared_ptr<const User>& user,
    int practice_id,
    const std::string& whisper_model,
    const std::string& llama_model,
    float similarity_threshold,
    const std::string& language,
    std::string& out_analysis_execution_id
)
{
    if (!user) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - User parameter is null");
        return USER_NO_AUTH;
    }

    if (!m_db || !m_subject_op || !m_practice_op || !m_speech_recognition || !m_embedding_engine || !m_similarity_search) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Injected dependencies missing");
        return UNKNOWN;
    }

    // 1. Fetch practice event
    std::shared_ptr<PracticeEvent> practice_event;
    rst_code_e res = m_practice_op->practice_event_get_by_id(user, static_cast<unsigned int>(practice_id), practice_event);
    if (res != RST_OK || !practice_event) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Failed to retrieve practice event ID: {}", practice_id);
        return (res != RST_OK) ? res : PRACTICE_EVENT_NOT_FOUND;
    }

    std::string audio_path = practice_event->get_filepath();
    if (audio_path.empty()) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Practice event has empty audio path");
        return PRACTICE_EVENT_NO_SOUND_LENGHT;
    }

    // 2. Fetch subject
    std::shared_ptr<Subject> subject;
    res = m_subject_op->subject_get_by_id(user, practice_event->get_subject_id(), subject);
    if (res != RST_OK || !subject) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Failed to retrieve parent subject ID: {}", practice_event->get_subject_id());
        return (res != RST_OK) ? res : SUBJECT_NOT_FOUND;
    }

    std::string reference_path = subject->get_filepath();
    if (reference_path.empty()) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Subject reference file path is empty");
        return FILE_NOT_FOUND;
    }

    // 3. Inspect PDF/TXT page count and extract reference chunks
    TextFileHandler text_handler(reference_path);
    text_handler.parse();

    unsigned int max_page_limit = ConfigurationSystem::getInstance().get_max_pdf_page_count();
    if (text_handler.get_number_of_pages() > static_cast<int>(max_page_limit)) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Reference document page count ({}) exceeds limit ({})",
                                  text_handler.get_number_of_pages(), max_page_limit);
        return FILE_EXCEEDS_PAGE_LIMIT;
    }

    TextChunkExtractor chunk_extractor;
    std::vector<DocumentChunk> doc_chunks;
    res = chunk_extractor.extract_chunks(text_handler, doc_chunks);
    if (res != RST_OK || doc_chunks.empty()) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Extraction failed or reference file empty");
        return (res != RST_OK) ? res : FILE_EMPTY_OR_INVALID;
    }

    // 4. Resolve parameters
    ManagerModels model_mgr;
    std::string resolved_whisper_model = whisper_model;
    if (resolved_whisper_model.empty() || resolved_whisper_model == "AUTO" || resolved_whisper_model == "auto") {
        resolved_whisper_model = model_mgr.auto_select_whisper_model();
    }

    std::string resolved_llama_model = llama_model;
    if (resolved_llama_model.empty() || resolved_llama_model == "AUTO" || resolved_llama_model == "auto") {
        resolved_llama_model = model_mgr.auto_select_llama_model();
    }

    std::string resolved_language = language;
    if (resolved_language.empty()) {
        resolved_language = subject->get_language();
        if (resolved_language.empty()) {
            resolved_language = "es";
        }
    }

    float resolved_threshold = similarity_threshold;
    if (resolved_threshold <= 0.0f) {
        resolved_threshold = ConfigurationSystem::getInstance().get_coverage_similarity_threshold();
    }

    // 5. Speech recognition transcription
    ISpeechRecognition::speech_recognition_config_t speech_config;
    speech_config.model_name = resolved_whisper_model;
    speech_config.language = resolved_language;
    speech_config.use_gpu = ConfigurationSystem::getInstance().get_whisper_use_gpu();
    
    res = m_speech_recognition->initialize(speech_config);
    if (res != RST_OK) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Speech recognition initialization failed");
        return res;
    }

    res = m_speech_recognition->submit_task(audio_path);
    if (res != RST_OK) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Speech recognition task submission failed");
        return res;
    }

    std::vector<TranscriptSegment> segments;
    res = m_speech_recognition->get_segments(segments);
    if (res != RST_OK) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Failed to retrieve transcript segments");
        return res;
    }

    // 6. Vector embeddings
    std::vector<std::string> pdf_texts;
    std::vector<float> pdf_weights;
    pdf_texts.reserve(doc_chunks.size());
    pdf_weights.reserve(doc_chunks.size());
    for (const auto& chunk : doc_chunks) {
        pdf_texts.push_back(chunk.text);
        pdf_weights.push_back(static_cast<float>(chunk.importance_weight));
    }

    std::vector<std::string> transcript_texts;
    transcript_texts.reserve(segments.size());
    for (const auto& seg : segments) {
        transcript_texts.push_back(seg.text);
    }

    auto pdf_embeddings = m_embedding_engine->generate_embeddings_batch(pdf_texts);
    auto transcript_embeddings = m_embedding_engine->generate_embeddings_batch(transcript_texts);

    if (pdf_embeddings.size() != doc_chunks.size() || transcript_embeddings.size() != segments.size()) {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Embedding generation size mismatch");
        return UNKNOWN;
    }

    // 7. Vector Similarity Search
    m_similarity_search->reset();
    m_similarity_search->index_transcript_embeddings(transcript_embeddings);

    SimilaritySearchOptions search_options;
    search_options.similarity_threshold = resolved_threshold;
    search_options.numeric_boost = ConfigurationSystem::getInstance().get_coverage_numeric_boost();
    search_options.numeric_mismatch_penalty = ConfigurationSystem::getInstance().get_coverage_numeric_mismatch_penalty();
    search_options.temporal_penalty_weight = ConfigurationSystem::getInstance().get_coverage_temporal_penalty_weight();

    auto matches = m_similarity_search->search_pdf_matches_advanced(
        pdf_embeddings,
        pdf_texts,
        transcript_texts,
        pdf_weights,
        search_options
    );

    size_t mentioned_count = 0;
    size_t warning_count = 0;
    for (const auto& match : matches) {
        if (match.is_mentioned) {
            mentioned_count++;
        }
        if (match.has_numeric_warning) {
            warning_count++;
        }
    }

    double coverage_pct = doc_chunks.empty() ? 0.0 : (static_cast<double>(mentioned_count) / static_cast<double>(doc_chunks.size())) * 100.0;

    // 8. Voice Quality Evaluation
    VoiceQualityMetrics voice_metrics = VoiceQualityAnalyzer::analyze(segments);

    // 9. Build JSON config snapshot and detailed report
    std::ostringstream config_ss;
    config_ss << "{"
              << "\"whisper_model\":\"" << resolved_whisper_model << "\","
              << "\"llama_model\":\"" << resolved_llama_model << "\","
              << "\"language\":\"" << resolved_language << "\","
              << "\"similarity_threshold\":" << resolved_threshold << ","
              << "\"numeric_boost\":" << search_options.numeric_boost << ","
              << "\"numeric_mismatch_penalty\":" << search_options.numeric_mismatch_penalty << ","
              << "\"temporal_penalty_weight\":" << search_options.temporal_penalty_weight
              << "}";
    std::string config_snapshot_json = config_ss.str();

    std::ostringstream report_ss;
    report_ss << std::boolalpha;
    report_ss << "{"
              << "\"practice_id\":" << practice_id << ","
              << "\"coverage_percentage\":" << std::fixed << std::setprecision(2) << coverage_pct << ","
              << "\"voice_quality\":{"
              << "\"speech_rate_wpm\":" << voice_metrics.speech_rate_wpm << ","
              << "\"clarity_score\":" << voice_metrics.clarity_score << ","
              << "\"pacing_score\":" << voice_metrics.pacing_score << ","
              << "\"overall_score\":" << voice_metrics.overall_quality_score
              << "},"
              << "\"total_pdf_chunks\":" << doc_chunks.size() << ","
              << "\"mentioned_chunks\":" << mentioned_count << ","
              << "\"numeric_warning_chunks\":" << warning_count << ","
              << "\"chunk_matches\":[";

    for (size_t i = 0; i < matches.size(); ++i) {
        const auto& m = matches[i];
        if (i > 0) report_ss << ",";
        report_ss << "{"
                  << "\"pdf_chunk_index\":" << m.pdf_chunk_index << ","
                  << "\"best_transcript_chunk_index\":" << m.best_transcript_chunk_index << ","
                  << "\"candidate_transcript_chunk_index\":" << m.candidate_transcript_chunk_index << ","
                  << "\"similarity_score\":" << std::setprecision(4) << m.similarity_score << ","
                  << "\"is_mentioned\":" << m.is_mentioned << ","
                  << "\"has_numeric_warning\":" << m.has_numeric_warning
                  << "}";
    }

    report_ss << "]}";
    std::string report_json = report_ss.str();

    // 10. Persist execution record in SQLite
    std::string execution_id = generate_execution_uuid();
    res = m_db->save_coverage_analysis_execution(
        practice_id,
        execution_id,
        coverage_pct,
        voice_metrics.speech_rate_wpm,
        voice_metrics.clarity_score,
        voice_metrics.pacing_score,
        resolved_whisper_model,
        resolved_llama_model,
        resolved_language,
        resolved_threshold,
        config_snapshot_json,
        report_json
    );

    if (res == RST_OK) {
        out_analysis_execution_id = execution_id;
        practice_event->set_analysis_execution_id(execution_id);
        if (m_practice_op) {
            m_practice_op->practice_event_update(user, *practice_event);
        }
        if (logger) logger->info("OperationCoverage::analyze_practice_coverage - Analysis execution saved successfully with ID: {}", execution_id);
    } else {
        if (logger) logger->error("OperationCoverage::analyze_practice_coverage - Failed to persist analysis execution to DB");
    }

    return res;
}

rst_code_e OperationCoverage::analyze_practice_coverage(
    const std::shared_ptr<const User>& user,
    int practice_id,
    const UserConfiguration& config,
    std::string& out_analysis_execution_id
) {
    return analyze_practice_coverage(
        user,
        practice_id,
        config.whisper.model_name,
        config.comparison.embedding_model_name,
        config.comparison.similarity_threshold,
        config.whisper.language,
        out_analysis_execution_id
    );
}
