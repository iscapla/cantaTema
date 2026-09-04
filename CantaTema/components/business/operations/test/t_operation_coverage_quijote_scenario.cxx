#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

#include "operations/operation_coverage.hpp"
#include "t_operation_coverage_helper.hpp"
#include "database/mocks/mock_database.hpp"
#include "operations/mocks/mock_operation_subject.hpp"
#include "operations/mocks/mock_operation_practice_event.hpp"
#include "file_handler/mocks/mock_file_handler.hpp"
#include "speech_recognition/mocks/mock_speech_recognition.hpp"
#include "embeddings/mocks/mock_embedding_engine.hpp"
#include "similarity/mocks/mock_similarity_search.hpp"
#include "primitives/tool_paths.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

class OperationCoverageQuijoteTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_db = std::make_shared<MockDatabase>();
        mock_subject_op = std::make_shared<MockOperationSubject>();
        mock_practice_op = std::make_shared<MockOperationPracticeEvent>();
        mock_file_handler = std::make_shared<MockFileHandler>();
        mock_speech = std::make_shared<MockSpeechRecognition>();
        mock_embedding = std::make_shared<MockEmbeddingEngine>();
        EXPECT_CALL(*mock_embedding, load_model(_)).WillRepeatedly(Return(true));
        mock_similarity = std::make_shared<MockSimilaritySearch>();

        auto mutable_user = std::make_shared<User>("quijote_user");
        mutable_user->set_useraccountid(100);
        user = mutable_user;

        // Resolve path to Don_Quijote_de_la_Mancha.txt in example_data
        std::filesystem::path base = ToolPath::get_base_path();
        ref_quijote_txt = (base / "example_data" / "Don_Quijote_de_la_Mancha.txt").string();
        audio_quijote_wav = (base / "example_data" / "Don_Quijote_de_la_Mancha.wav").string();

        // If file doesn't exist at ToolPath, create fallback fixture for robust unit testing
        if (!std::filesystem::exists(ref_quijote_txt)) {
            std::filesystem::path fallback_txt = std::filesystem::current_path() / "Don_Quijote_fallback.txt";
            std::ofstream ofs(fallback_txt);
            ofs << "En un lugar de la Mancha, de cuyo nombre no quiero acordarme, no ha mucho tiempo que vivía un hidalgo de los de lanza en astillero, adarga antigua, rocín flaco y galgo corredor. "
                << "Una olla de algo más vaca que carnero, salpicón las más noches, duelos y quebrantos los sábados, lantejas los viernes, algún palomino de añadidura los domingos, consumían las tres partes de su hacienda. "
                << "El resto della concluían sayo de velarte, calzas de velludo para las fiestas con sus pantuflos de lo mismo, los días de entre semana se honraba con su vellorí de lo más fino.";
            ofs.close();
            ref_quijote_txt = fallback_txt.string();
            created_fallback_ref = true;
        }

        if (!std::filesystem::exists(audio_quijote_wav)) {
            std::filesystem::path fallback_audio = std::filesystem::current_path() / "Don_Quijote_fallback.wav";
            std::ofstream ofs_audio(fallback_audio);
            ofs_audio << "dummy wav audio header and sample data for 33 second audio clip";
            ofs_audio.close();
            audio_quijote_wav = fallback_audio.string();
            created_fallback_audio = true;
        }

        coverage_op = make_test_coverage_op(
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
        if (created_fallback_ref && std::filesystem::exists(ref_quijote_txt)) {
            std::filesystem::remove(ref_quijote_txt);
        }
        if (created_fallback_audio && std::filesystem::exists(audio_quijote_wav)) {
            std::filesystem::remove(audio_quijote_wav);
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
    std::string ref_quijote_txt;
    std::string audio_quijote_wav;
    bool created_fallback_ref{false};
    bool created_fallback_audio{false};

    std::unique_ptr<OperationCoverage> coverage_op;
};

// 1. Scenario: 100% Full Match on Don Quijote Sentences
TEST_F(OperationCoverageQuijoteTest, QuijoteFullCoverage100Percent) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(201);
    practice->set_subject_id(50);
    practice->set_filepath(audio_quijote_wav);

    auto subject = std::make_shared<Subject>(50, "Don Quijote de la Mancha");
    subject->set_filepath(ref_quijote_txt);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 201, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 50, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(audio_quijote_wav)).WillOnce(Return(RST_OK));

    std::vector<TranscriptSegment> mock_segments;
    TranscriptSegment s1{0, 11000, "En un lugar de la Mancha, de cuyo nombre no quiero acordarme...", 0.96f};
    TranscriptSegment s2{11000, 22000, "Una olla de algo más vaca que carnero, salpicón las más noches...", 0.94f};
    TranscriptSegment s3{22000, 33000, "El resto della concluían sayo de velarte, calzas de velludo...", 0.95f};
    mock_segments.push_back(s1);
    mock_segments.push_back(s2);
    mock_segments.push_back(s3);

    EXPECT_CALL(*mock_speech, get_segments(_))
        .WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.8f, 0.6f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches;
    SimilarityResult sim1;
    sim1.pdf_chunk_index = 0;
    sim1.best_transcript_chunk_index = 0;
    sim1.similarity_score = 0.96f;
    sim1.is_mentioned = true;
    sim1.weighted_missed_score = 0.0f;

    SimilarityResult sim2;
    sim2.pdf_chunk_index = 1;
    sim2.best_transcript_chunk_index = 1;
    sim2.similarity_score = 0.92f;
    sim2.is_mentioned = true;
    sim2.weighted_missed_score = 0.0f;

    SimilarityResult sim3;
    sim3.pdf_chunk_index = 2;
    sim3.best_transcript_chunk_index = 2;
    sim3.similarity_score = 0.94f;
    sim3.is_mentioned = true;
    sim3.weighted_missed_score = 0.0f;

    mock_matches.push_back(sim1);
    mock_matches.push_back(sim2);
    mock_matches.push_back(sim3);

    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));

    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(201, _, ::testing::DoubleEq(100.0), _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(RST_OK));

    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _))
        .WillOnce(Return(RST_OK));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 201, "ggml-tiny.bin", "multilingual-e5-large", 0.75f, "es", exec_id);

    EXPECT_EQ(res, RST_OK);
    EXPECT_FALSE(exec_id.empty());
    EXPECT_TRUE(exec_id.rfind("exec-", 0) == 0);
}

// 2. Scenario: Partial Coverage (Missing Sentence #2 "Una olla de algo más vaca que carnero...")
TEST_F(OperationCoverageQuijoteTest, QuijotePartialCoverageMissingSentence2) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(202);
    practice->set_subject_id(50);
    practice->set_filepath(audio_quijote_wav);

    auto subject = std::make_shared<Subject>(50, "Don Quijote de la Mancha");
    subject->set_filepath(ref_quijote_txt);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 202, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 50, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(audio_quijote_wav)).WillOnce(Return(RST_OK));

    // Student only spoke Sentence 1 and Sentence 3
    std::vector<TranscriptSegment> mock_segments;
    TranscriptSegment s1{0, 15000, "En un lugar de la Mancha, de cuyo nombre no quiero acordarme...", 0.95f};
    TranscriptSegment s3{15000, 30000, "El resto della concluían sayo de velarte, calzas de velludo...", 0.91f};
    mock_segments.push_back(s1);
    mock_segments.push_back(s3);

    EXPECT_CALL(*mock_speech, get_segments(_))
        .WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.7f, 0.7f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    // Sentence 2 is unmatched (is_mentioned = false)
    std::vector<SimilarityResult> mock_matches;
    SimilarityResult sim1;
    sim1.pdf_chunk_index = 0;
    sim1.best_transcript_chunk_index = 0;
    sim1.similarity_score = 0.95f;
    sim1.is_mentioned = true;
    sim1.weighted_missed_score = 0.0f;

    SimilarityResult sim2;
    sim2.pdf_chunk_index = 1;
    sim2.best_transcript_chunk_index = -1;
    sim2.similarity_score = 0.22f;
    sim2.is_mentioned = false;
    sim2.weighted_missed_score = 0.78f;

    SimilarityResult sim3;
    sim3.pdf_chunk_index = 2;
    sim3.best_transcript_chunk_index = 1;
    sim3.similarity_score = 0.91f;
    sim3.is_mentioned = true;
    sim3.weighted_missed_score = 0.0f;

    mock_matches.push_back(sim1);
    mock_matches.push_back(sim2);
    mock_matches.push_back(sim3);

    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));

    // Coverage is 2 out of 3 sentences (66.666...%)
    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(202, _, ::testing::DoubleNear(66.666, 0.5), _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(RST_OK));

    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _))
        .WillOnce(Return(RST_OK));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 202, "AUTO", "AUTO", 0.75f, "es", exec_id);

    EXPECT_EQ(res, RST_OK);
    EXPECT_FALSE(exec_id.empty());
}

// 3. Scenario: Quality Score & Timing Calculations for ~33 Second Don Quijote Audio
TEST_F(OperationCoverageQuijoteTest, QuijoteVoiceQualityAndPacingMetrics) {
    auto practice = std::make_shared<PracticeEvent>();
    practice->set_id(203);
    practice->set_subject_id(50);
    practice->set_filepath(audio_quijote_wav);

    auto subject = std::make_shared<Subject>(50, "Don Quijote de la Mancha");
    subject->set_filepath(ref_quijote_txt);
    subject->set_language("es");

    EXPECT_CALL(*mock_practice_op, practice_event_get_by_id(user, 203, _))
        .WillOnce(DoAll(SetArgReferee<2>(practice), Return(RST_OK)));

    EXPECT_CALL(*mock_subject_op, subject_get_by_id(user, 50, _))
        .WillOnce(DoAll(SetArgReferee<2>(subject), Return(RST_OK)));

    EXPECT_CALL(*mock_speech, initialize(::testing::An<const ISpeechRecognition::speech_recognition_config_t&>())).WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_speech, submit_task(audio_quijote_wav)).WillOnce(Return(RST_OK));

    // High clarity, steady pace audio across 33 seconds (~33000 ms)
    std::vector<TranscriptSegment> mock_segments;
    TranscriptSegment s1{0, 11000, "En un lugar de la Mancha, de cuyo nombre no quiero acordarme...", 0.98f};
    TranscriptSegment s2{11000, 22000, "Una olla de algo más vaca que carnero, salpicón las más noches...", 0.97f};
    TranscriptSegment s3{22000, 33000, "El resto della concluían sayo de velarte, calzas de velludo...", 0.99f};
    mock_segments.push_back(s1);
    mock_segments.push_back(s2);
    mock_segments.push_back(s3);

    EXPECT_CALL(*mock_speech, get_segments(_))
        .WillOnce(DoAll(SetArgReferee<0>(mock_segments), Return(RST_OK)));

    EXPECT_CALL(*mock_embedding, generate_embeddings_batch(_, _))
        .WillRepeatedly([](const std::vector<std::string>& texts, EmbeddingRole role) {
            (void)role;
            return std::vector<std::vector<float>>(texts.size(), std::vector<float>{0.9f, 0.9f});
        });

    EXPECT_CALL(*mock_similarity, reset()).Times(1);
    EXPECT_CALL(*mock_similarity, index_transcript_embeddings(_)).WillOnce(Return(true));

    std::vector<SimilarityResult> mock_matches;
    SimilarityResult sim1;
    sim1.pdf_chunk_index = 0;
    sim1.best_transcript_chunk_index = 0;
    sim1.similarity_score = 0.95f;
    sim1.is_mentioned = true;
    sim1.weighted_missed_score = 0.0f;

    SimilarityResult sim2;
    sim2.pdf_chunk_index = 1;
    sim2.best_transcript_chunk_index = 1;
    sim2.similarity_score = 0.95f;
    sim2.is_mentioned = true;
    sim2.weighted_missed_score = 0.0f;

    SimilarityResult sim3;
    sim3.pdf_chunk_index = 2;
    sim3.best_transcript_chunk_index = 2;
    sim3.similarity_score = 0.95f;
    sim3.is_mentioned = true;
    sim3.weighted_missed_score = 0.0f;

    mock_matches.push_back(sim1);
    mock_matches.push_back(sim2);
    mock_matches.push_back(sim3);

    EXPECT_CALL(*mock_similarity, search_pdf_matches_advanced(_, _, _, _, _)).WillOnce(Return(mock_matches));

    // Verify speed score > 0 and clarity score > 0 saved to database
    EXPECT_CALL(*mock_db, save_coverage_analysis_execution(
        203, _, _,
        ::testing::Gt(0.0), // speed score
        ::testing::Gt(0.0), // clarity score
        _, _, _, _, _, _, _))
        .WillOnce(Return(RST_OK));

    EXPECT_CALL(*mock_practice_op, practice_event_update(user, _))
        .WillOnce(Return(RST_OK));

    std::string exec_id;
    rst_code_e res = coverage_op->analyze_practice_coverage(user, 203, "AUTO", "AUTO", 0.75f, "es", exec_id);

    EXPECT_EQ(res, RST_OK);
    EXPECT_FALSE(exec_id.empty());
}
