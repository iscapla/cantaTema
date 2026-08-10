#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <fstream>
#include <filesystem>

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

class OperationCoverageScoringAndReportsTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_db = std::make_shared<MockDatabase>();
        mock_subject_op = std::make_shared<MockOperationSubject>();
        mock_practice_op = std::make_shared<MockOperationPracticeEvent>();
        mock_file_handler = std::make_shared<MockFileHandler>();
        mock_speech = std::make_shared<MockSpeechRecognition>();
        mock_embedding = std::make_shared<MockEmbeddingEngine>();
        mock_similarity = std::make_shared<MockSimilaritySearch>();

        auto mutable_user = std::make_shared<User>("test_user");
        mutable_user->set_useraccountid(1);
        user = mutable_user;

        test_ref_file = (std::filesystem::current_path() / "test_ref_report.txt").string();
        std::ofstream ofs(test_ref_file);
        ofs << "Paragraph one concept topic. Paragraph two advanced concept.";
        ofs.close();

        test_audio_file = (std::filesystem::current_path() / "test_audio_report.opus").string();
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

TEST_F(OperationCoverageScoringAndReportsTest, FullPipeline100PercentCoverage) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(1);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    TranscriptSegment seg;
    seg.start_time_ms = 0;
    seg.end_time_ms = 3000;
    seg.text = "Paragraph one concept topic.";
    seg.confidence_score = 0.98f;
    mock_segments.push_back(seg);

    EXPECT_CALL(*mock_speech, get_segments(_))
        .WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.5f, 0.5f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches;
    SimilarityResult sim1;
    sim1.is_mentioned = true;
    sim1.similarity_score = 0.92f;
    mock_matches.push_back(sim1);

    SimilarityResult sim2;
    sim2.is_mentioned = true;
    sim2.similarity_score = 0.88f;
    mock_matches.push_back(sim2);

    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));

    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(1, _, 100.0, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(RST_OK));

    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _))
        .WillOnce(Return(RST_OK));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "AUTO", "AUTO", 0.75f, "es", exec_id);

    EXPECT_EQ(res, RST_OK);
    EXPECT_FALSE(exec_id.empty());
    EXPECT_TRUE(exec_id.rfind("exec-", 0) == 0); // starts with "exec-"
}

TEST_F(OperationCoverageScoringAndReportsTest, FullPipelineZeroPercentCoverage) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(1);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    TranscriptSegment seg;
    seg.start_time_ms = 0;
    seg.end_time_ms = 2000;
    seg.text = "Unrelated speech text.";
    seg.confidence_score = 0.50f;
    mock_segments.push_back(seg);

    EXPECT_CALL(*mock_speech, get_segments(_))
        .WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.1f, 0.1f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches;
    SimilarityResult sim1;
    sim1.is_mentioned = false;
    sim1.similarity_score = 0.20f;
    mock_matches.push_back(sim1);

    SimilarityResult sim2;
    sim2.is_mentioned = false;
    sim2.similarity_score = 0.15f;
    mock_matches.push_back(sim2);

    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));

    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(1, _, 0.0, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(RST_OK));

    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _))
        .WillOnce(Return(RST_OK));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "", "", 0.85f, "", exec_id);

    EXPECT_EQ(res, RST_OK);
    EXPECT_FALSE(exec_id.empty());
}
