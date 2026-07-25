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

class OperationCoveragePipelineFailuresTest : public ::testing::Test {
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

        test_ref_file = (std::filesystem::current_path() / "test_ref_fail.txt").string();
        std::ofstream ofs(test_ref_file);
        ofs << "Sentence one for testing pipeline failures. Sentence two for validation.";
        ofs.close();

        test_audio_file = (std::filesystem::current_path() / "test_audio_fail.opus").string();
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

TEST_F(OperationCoveragePipelineFailuresTest, SpeechRecognitionInitFailure) {
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

    EXPECT_CALL(*mock_speech, initialize(_))
        .WillOnce(Return(MODELS_FILE_NOT_FOUND));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "tiny", "e5", 0.8f, "es", exec_id);
    EXPECT_EQ(res, MODELS_FILE_NOT_FOUND);
}

TEST_F(OperationCoveragePipelineFailuresTest, SpeechRecognitionSubmitTaskFailure) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(1);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    EXPECT_CALL(*mock_speech, initialize(_)).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(UNKNOWN));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "tiny", "e5", 0.8f, "es", exec_id);
    EXPECT_EQ(res, UNKNOWN);
}

TEST_F(OperationCoveragePipelineFailuresTest, EmbeddingSizeMismatchReturnsUnknown) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(1);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    EXPECT_CALL(*mock_speech, initialize(_)).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    TranscriptSegment seg;
    seg.text = "Transcript line";
    mock_segments.push_back(seg);

    EXPECT_CALL(*mock_speech, get_segments(_))
        .WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    // Embedding engine returns empty vector (size mismatch)
    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_))
        .WillRepeatedly(Return(std::vector<std::vector<float>>{}));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "tiny", "e5", 0.8f, "es", exec_id);
    EXPECT_EQ(res, UNKNOWN);
}

TEST_F(OperationCoveragePipelineFailuresTest, DatabaseSaveFailurePropagatesError) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(1);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(test_ref_file);

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    EXPECT_CALL(*mock_speech, initialize(_)).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(test_audio_file)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    TranscriptSegment seg;
    seg.text = "Sentence one for testing pipeline failures.";
    mock_segments.push_back(seg);

    EXPECT_CALL(*mock_speech, get_segments(_))
        .WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_))
        .WillRepeatedly([](const std::vector<std::string>& texts) {
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.1f, 0.2f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches;
    SimilarityResult sim;
    sim.is_mentioned = true;
    mock_matches.push_back(sim);

    EXPECT_CALL(*mock_similarity, search_pdf_matches(_, _, _)).WillOnce(Return(mock_matches));

    // Database save fails
    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(1, _, _, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UNKNOWN));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "AUTO", "AUTO", 0.75f, "es", exec_id);
    EXPECT_EQ(res, UNKNOWN);
    EXPECT_TRUE(exec_id.empty());
}
