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
#include "configuration/configuration_system.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

class OperationCoverageValidationTest : public ::testing::Test {
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

        test_ref_file = (std::filesystem::current_path() / "test_ref_valid.txt").string();
        std::ofstream ofs(test_ref_file);
        ofs << "This is sentence one about topics. This is sentence two covering concepts.";
        ofs.close();

        test_audio_file = (std::filesystem::current_path() / "test_audio_valid.opus").string();
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

TEST_F(OperationCoverageValidationTest, NullUserReturnsNoAuth) {
    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(nullptr, 1, "", "", 0.0f, "", exec_id);
    EXPECT_EQ(res, USER_NO_AUTH);
}

TEST_F(OperationCoverageValidationTest, NullDependenciesReturnsUnknown) {
    OperationCoverage op_null(nullptr, mock_subject_op, mock_practice_op, mock_file_handler, mock_speech, mock_embedding, mock_similarity);
    std::string exec_id;
    rst_code_e res = op_null.analyze_practice_coverage(user, 1, "", "", 0.0f, "", exec_id);
    EXPECT_EQ(res, UNKNOWN);
}

TEST_F(OperationCoverageValidationTest, PracticeEventNotFound) {
    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(Return(PRACTICE_EVENT_NOT_FOUND));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "", "", 0.0f, "", exec_id);
    EXPECT_EQ(res, PRACTICE_EVENT_NOT_FOUND);
}

TEST_F(OperationCoverageValidationTest, EmptyAudioPathReturnsError) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(1);
    practice->set_filepath(""); // empty audio path

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "", "", 0.0f, "", exec_id);
    EXPECT_EQ(res, PRACTICE_EVENT_NO_SOUND_LENGHT);
}

TEST_F(OperationCoverageValidationTest, SubjectNotFound) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(1);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(Return(SUBJECT_NOT_FOUND));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "", "", 0.0f, "", exec_id);
    EXPECT_EQ(res, SUBJECT_NOT_FOUND);
}

TEST_F(OperationCoverageValidationTest, EmptyReferenceFilePathReturnsFileNotFound) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(1);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(""); // Empty reference path

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "", "", 0.0f, "", exec_id);
    EXPECT_EQ(res, FILE_NOT_FOUND);
}

TEST_F(OperationCoverageValidationTest, ReferenceFileEmptyReturnsEmptyOrInvalid) {
    std::string empty_ref = (std::filesystem::current_path() / "empty_ref.txt").string();
    std::ofstream ofs(empty_ref);
    ofs.close(); // zero-byte file

    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(1);
    practice->set_subject_id(10);
    practice->set_filepath(test_audio_file);

    auto subject = std::make_shared<Subject>(10, "Subject Test");
    subject->set_filepath(empty_ref);

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 1, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 10, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 1, "", "", 0.0f, "", exec_id);
    EXPECT_EQ(res, FILE_EMPTY_OR_INVALID);

    if (std::filesystem::exists(empty_ref)) {
        std::filesystem::remove(empty_ref);
    }
}
