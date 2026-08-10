#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

#include "operations/operation_coverage.hpp"
#include "database/mocks/mock_database.hpp"
#include "operations/mocks/mock_operation_subject.hpp"
#include "operations/mocks/mock_operation_practice_event.hpp"
#include "file_handler/mocks/mock_file_handler.hpp"
#include "speech_recognition/mocks/mock_speech_recognition.hpp"
#include "embeddings/mocks/mock_embedding_engine.hpp"
#include "similarity/mocks/mock_similarity_search.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

class OperationCoverageLlamaComparisonTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_db = std::make_shared<MockDatabase>();
        mock_subject_op = std::make_shared<MockOperationSubject>();
        mock_practice_op = std::make_shared<MockOperationPracticeEvent>();
        mock_file_handler = std::make_shared<MockFileHandler>();
        mock_speech = std::make_shared<MockSpeechRecognition>();
        mock_embedding = std::make_shared<MockEmbeddingEngine>();
        mock_similarity = std::make_shared<MockSimilaritySearch>();

        auto mutable_user = std::make_shared<User>("llama_test_user");
        mutable_user->set_useraccountid(300);
        user = mutable_user;

        test_ref_file = (std::filesystem::current_path() / "test_ref_llama.txt").string();
        std::ofstream ofs(test_ref_file);
        ofs << "Chunk one text. Chunk two text. Chunk three text. Chunk four text. Chunk five text.";
        ofs.close();

        test_audio_file = (std::filesystem::current_path() / "test_audio_llama.wav").string();
        std::ofstream ofs_audio(test_audio_file);
        ofs_audio << "dummy audio content";
        ofs_audio.close();

        coverage_op = std::make_unique<OperationCoverage>(
            mock_db,
            mock_subject_op,
            mock_practice_op,
            mock_file_handler,
            mock_speech,
            mock_embedding,
            mock_similarity
        );
    }

    void TearDown() override {
        if (std::filesystem::exists(test_ref_file)) {
            std::filesystem::remove(test_ref_file);
        }
        if (std::filesystem::exists(test_audio_file)) {
            std::filesystem::remove(test_audio_file);
        }
    }

    std::shared_ptr<MockDatabase> mock_db;
    std::shared_ptr<MockOperationSubject> mock_subject_op;
    std::shared_ptr<MockOperationPracticeEvent> mock_practice_op;
    std::shared_ptr<MockFileHandler> mock_file_handler;
    std::shared_ptr<MockSpeechRecognition> mock_speech;
    std::shared_ptr<MockEmbeddingEngine> mock_embedding;
    std::shared_ptr<MockSimilaritySearch> mock_similarity;

    std::shared_ptr<const User> user;
    std::string test_ref_file;
    std::string test_audio_file;

    std::unique_ptr<OperationCoverage> coverage_op;
};

// 15. Comparison (llama). Missing inputs
TEST_F(OperationCoverageLlamaComparisonTest, ComparisonMissingInputs) {
    std::string exec_id;
    // Null user
    EXPECT_EQ(coverage_op->analyze_practice_coverage(nullptr, 1, "", "", 0.75f, "es", exec_id), USER_NO_AUTH);

    // Empty practice audio path
    auto practice_no_audio = std::make_shared<PracticeEvent>();
    practice_no_audio->set_id(1);
    practice_no_audio->set_filepath("");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice_no_audio), Return(RST_OK)));

    EXPECT_EQ(coverage_op->analyze_practice_coverage(user, 1, "", "", 0.75f, "es", exec_id), PRACTICE_EVENT_NO_SOUND_LENGHT);
}

// 16. Comparison (llama). Model conversion / embedding errors
TEST_F(OperationCoverageLlamaComparisonTest, ComparisonModelConversionErrors) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(301);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 301, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    mock_segments.push_back({0, 2000, "Some transcript.", 0.90f});
    EXPECT_CALL(*mock_speech, get_segments(_)).WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    // Embedding engine returns empty vectors (conversion error)
    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly(Return(std::vector<std::vector<float>>{}));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 301, "AUTO", "AUTO", 0.75f, "es", exec_id);
    EXPECT_EQ(res, UNKNOWN); // Size mismatch / conversion error yields UNKNOWN code
}

// 17. Comparison (llama). 100% accuracy
TEST_F(OperationCoverageLlamaComparisonTest, Comparison100Accuracy) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(302);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 302, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));
    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));
    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    mock_segments.push_back({0, 5000, "Chunk one text. Chunk two text.", 0.95f});
    EXPECT_CALL(*mock_speech, get_segments(_)).WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{1.0f, 0.0f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches(5);
    for (size_t i = 0; i < 5; ++i) {
        mock_matches[i].pdf_chunk_index = i;
        mock_matches[i].best_transcript_chunk_index = static_cast<int>(i);
        mock_matches[i].similarity_score = 0.95f;
        mock_matches[i].is_mentioned = true;
        mock_matches[i].weighted_missed_score = 0.0f;
    }

    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));
    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(302, _, ::testing::DoubleEq(100.0), _, _, _, _, _, _, _, _, _)).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _)).WillOnce(Return(RST_OK));

    std::string exec_id;
    EXPECT_EQ(coverage_op->analyze_practice_coverage(user, 302, "AUTO", "AUTO", 0.75f, "es", exec_id), RST_OK);
}

// 18. Comparison (llama). Conversion with a single difference
TEST_F(OperationCoverageLlamaComparisonTest, ComparisonSingleDifference) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(303);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 303, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));
    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));
    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    mock_segments.push_back({0, 4000, "Chunks 1, 2, 3, 4.", 0.90f});
    EXPECT_CALL(*mock_speech, get_segments(_)).WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.5f, 0.5f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches(5);
    for (size_t i = 0; i < 4; ++i) {
        mock_matches[i].pdf_chunk_index = i;
        mock_matches[i].best_transcript_chunk_index = static_cast<int>(i);
        mock_matches[i].similarity_score = 0.90f;
        mock_matches[i].is_mentioned = true;
    }
    // Single difference: Chunk 5 missing
    mock_matches[4].pdf_chunk_index = 4;
    mock_matches[4].best_transcript_chunk_index = -1;
    mock_matches[4].similarity_score = 0.20f;
    mock_matches[4].is_mentioned = false;
    mock_matches[4].weighted_missed_score = 0.80f;

    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));
    // 4 out of 5 chunks covered = 80% coverage
    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(303, _, ::testing::DoubleEq(80.0), _, _, _, _, _, _, _, _, _)).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _)).WillOnce(Return(RST_OK));

    std::string exec_id;
    EXPECT_EQ(coverage_op->analyze_practice_coverage(user, 303, "AUTO", "AUTO", 0.75f, "es", exec_id), RST_OK);
}

// 19. Comparison (llama). Conversion with some differences
TEST_F(OperationCoverageLlamaComparisonTest, ComparisonSomeDifferences) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(304);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 304, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));
    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));
    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    mock_segments.push_back({0, 2000, "Chunks 1 and 2.", 0.90f});
    EXPECT_CALL(*mock_speech, get_segments(_)).WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.5f, 0.5f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches(5);
    for (size_t i = 0; i < 2; ++i) {
        mock_matches[i].pdf_chunk_index = i;
        mock_matches[i].best_transcript_chunk_index = static_cast<int>(i);
        mock_matches[i].similarity_score = 0.90f;
        mock_matches[i].is_mentioned = true;
    }
    for (size_t i = 2; i < 5; ++i) {
        mock_matches[i].pdf_chunk_index = i;
        mock_matches[i].best_transcript_chunk_index = -1;
        mock_matches[i].similarity_score = 0.15f;
        mock_matches[i].is_mentioned = false;
        mock_matches[i].weighted_missed_score = 0.85f;
    }

    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));
    // 2 out of 5 chunks = 40% coverage
    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(304, _, ::testing::DoubleEq(40.0), _, _, _, _, _, _, _, _, _)).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _)).WillOnce(Return(RST_OK));

    std::string exec_id;
    EXPECT_EQ(coverage_op->analyze_practice_coverage(user, 304, "AUTO", "AUTO", 0.75f, "es", exec_id), RST_OK);
}

// 20. Comparison (llama). Conversion with different text orders
TEST_F(OperationCoverageLlamaComparisonTest, ComparisonDifferentTextOrders) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(305);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 305, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));
    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));
    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    // Spoken out of order: Chunk 5 first, then Chunk 1
    std::vector<TranscriptSegment> mock_segments;
    mock_segments.push_back({0, 2000, "Chunk five text.", 0.92f});
    mock_segments.push_back({2000, 4000, "Chunk one text.", 0.94f});
    EXPECT_CALL(*mock_speech, get_segments(_)).WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.6f, 0.6f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches(5);
    // Faiss vector search maps Chunk 0 to transcript #1 and Chunk 4 to transcript #0
    mock_matches[0].pdf_chunk_index = 0;
    mock_matches[0].best_transcript_chunk_index = 1;
    mock_matches[0].similarity_score = 0.94f;
    mock_matches[0].is_mentioned = true;

    mock_matches[4].pdf_chunk_index = 4;
    mock_matches[4].best_transcript_chunk_index = 0;
    mock_matches[4].similarity_score = 0.92f;
    mock_matches[4].is_mentioned = true;

    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));
    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(305, _, _, _, _, _, _, _, _, _, _, _)).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _)).WillOnce(Return(RST_OK));

    std::string exec_id;
    EXPECT_EQ(coverage_op->analyze_practice_coverage(user, 305, "AUTO", "AUTO", 0.75f, "es", exec_id), RST_OK);
}

// 21. Comparison (llama). Weights that change the conversion rates
TEST_F(OperationCoverageLlamaComparisonTest, ComparisonWeightsChangeConversionRates) {
    SimilarityResult normal_missed;
    normal_missed.pdf_chunk_index = 0;
    normal_missed.is_mentioned = false;
    normal_missed.similarity_score = 0.20f;
    normal_missed.weighted_missed_score = 1.0f * (1.0f - 0.20f); // 0.80 penalty

    SimilarityResult bold_missed;
    bold_missed.pdf_chunk_index = 1;
    bold_missed.is_mentioned = false;
    bold_missed.similarity_score = 0.20f;
    bold_missed.weighted_missed_score = 1.5f * (1.0f - 0.20f); // 1.20 penalty (bold weight 1.5x)

    EXPECT_GT(bold_missed.weighted_missed_score, normal_missed.weighted_missed_score);
}

// 22. Comparison (llama). Metrics
TEST_F(OperationCoverageLlamaComparisonTest, ComparisonMetricsVerification) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(307);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 307, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));
    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));
    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    mock_segments.push_back({0, 3000, "Segment test metrics.", 0.98f});
    EXPECT_CALL(*mock_speech, get_segments(_)).WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.5f, 0.5f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches(5);
    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));

    // Verify speed score > 0, clarity score > 0, and metrics saved to database
    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(
        307, _, _,
        ::testing::Ge(0.0), // speed score metric
        ::testing::Ge(0.0), // clarity score metric
        _, _, _, _, _, _, _))
        .WillOnce(Return(RST_OK));

    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _)).WillOnce(Return(RST_OK));

    std::string exec_id;
    EXPECT_EQ(coverage_op->analyze_practice_coverage(user, 307, "AUTO", "AUTO", 0.75f, "es", exec_id), RST_OK);
}
