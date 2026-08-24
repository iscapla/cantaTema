#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "session/Session.hpp"
#include "database/db_main.hpp"
#include "primitives/user.hpp"
#include "primitives/category.hpp"
#include "primitives/subject.hpp"
#include "primitives/practice_event.hpp"
#include "primitives/user_metrics.hpp"
#include "operations/operation_user.hpp"
#include "operations/operation_category.hpp"
#include "operations/operation_user_metrics.hpp"
#include "operations/mocks/mock_operation_subject.hpp"
#include "operations/mocks/mock_operation_practice_event.hpp"
#include "operations/mocks/mock_operation_coverage.hpp"
#include "operations/mocks/mock_operation_analysis_scheduler.hpp"
#include "database/mocks/mock_database.hpp"
#include "sound_system/i_sound_system.hpp"
#include "speech_recognition/mocks/mock_gpu_detector.hpp"
#include <memory>
#include <vector>
#include <filesystem>
#include <fstream>

using ::testing::Return;
using ::testing::StrictMock;
using ::testing::DoAll;
using ::testing::SetArgReferee;
using ::testing::_;
using cantatema::infra::MockGpuDetector;

class MockSoundSystem : public ISoundSystem {
public:
    MOCK_METHOD(std::vector<SoundSystemDeviceInfo>, getCaptureDevices, (), (override));
    MOCK_METHOD(bool, startRecording, (const SoundFileHandler& fileHandler, int deviceIndex), (override));
    MOCK_METHOD(void, stopRecording, (), (override));
    MOCK_METHOD(bool, isRecording, (), (const, override));
    MOCK_METHOD(unsigned long long, get_recording_timestamp, (), (override));
    MOCK_METHOD(bool, play, (const SoundFileHandler& fileHandler, PlaybackCallback callback), (override));
    MOCK_METHOD(void, stopPlaying, (), (override));
    MOCK_METHOD(bool, isPlaying, (), (const, override));
    MOCK_METHOD(unsigned long long, get_playing_timestamp, (), (override));
};

class SessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure database starts from a purged state
        DB_Main::getInstance()->purge();
    }

    void TearDown() override {
        // No-op
    }
};

TEST_F(SessionTest, ConstructorExceptions) {
    EXPECT_THROW({
        Session s(nullptr, nullptr, nullptr, nullptr, nullptr);
    }, std::runtime_error);
}

TEST_F(SessionTest, CompleteUserSessionFlow) {
    Session session;

    // 1. Initial State: Not Authenticated
    EXPECT_FALSE(session.user_is_authenticated());

    std::shared_ptr<const User> current_user;
    EXPECT_EQ(session.user_get(current_user), USER_NO_AUTH);

    // 2. Add and Identify User
    std::string username = "session_flow_user";
    std::string password = "SecretPassword123";
    EXPECT_EQ(session.user_add(username, password), RST_OK);

    EXPECT_EQ(session.user_identify(username, password), RST_OK);
    EXPECT_TRUE(session.user_is_authenticated());

    EXPECT_EQ(session.user_get(current_user), RST_OK);
    ASSERT_NE(current_user, nullptr);
    EXPECT_EQ(current_user->get_name(), username);

    // Get user by name
    std::shared_ptr<const User> fetched_user;
    EXPECT_EQ(session.user_get_by_name(username, fetched_user), RST_OK);
    ASSERT_NE(fetched_user, nullptr);

    // Update user
    std::shared_ptr<User> modifiable_user = std::make_shared<User>(*current_user);
    modifiable_user->set_passwordkey("new_hashed_secret");
    std::shared_ptr<const User> const_modifiable = modifiable_user;
    EXPECT_EQ(session.user_update(const_modifiable), RST_OK);

    // 3. Category Operations
    EXPECT_EQ(session.category_add("Mathematics"), RST_OK);
    
    std::vector<std::shared_ptr<const Category>> categories;
    EXPECT_EQ(session.category_get_by_user(categories), RST_OK);
    ASSERT_EQ(categories.size(), 1u);
    unsigned int cat_id = categories[0]->get_id();
    EXPECT_EQ(categories[0]->get_name(), "Mathematics");

    std::shared_ptr<Category> fetched_cat;
    EXPECT_EQ(session.category_get_by_id(cat_id, fetched_cat), RST_OK);
    ASSERT_NE(fetched_cat, nullptr);
    EXPECT_EQ(fetched_cat->get_name(), "Mathematics");

    EXPECT_EQ(session.category_update(cat_id, "Advanced Mathematics"), RST_OK);

    // 4. Subject Operations
    std::string dummy_file = "dummy_subject.pdf";
    {
        std::ofstream ofs(dummy_file);
        ofs << "dummy PDF content";
    }
    EXPECT_EQ(session.subject_add("Algebra", cat_id, dummy_file), RST_OK);

    std::vector<std::shared_ptr<Subject>> subjects;
    EXPECT_EQ(session.subject_get_by_user(subjects), RST_OK);
    ASSERT_EQ(subjects.size(), 1u);
    unsigned int sub_id = subjects[0]->get_id();
    EXPECT_EQ(subjects[0]->get_name(), "Algebra");

    std::shared_ptr<Subject> fetched_subject;
    EXPECT_EQ(session.subject_get_by_id(sub_id, fetched_subject), RST_OK);
    EXPECT_NE(fetched_subject, nullptr);

    std::vector<std::shared_ptr<Subject>> cat_subjects;
    EXPECT_EQ(session.subject_get_by_category(cat_id, cat_subjects), RST_OK);
    EXPECT_EQ(cat_subjects.size(), 1u);

    EXPECT_EQ(session.subject_update(sub_id, "Linear Algebra", cat_id, fetched_subject->get_filepath()), RST_OK);

    // Subject Language Management
    EXPECT_EQ(session.set_subject_language(sub_id, "es"), RST_OK);
    EXPECT_EQ(session.subject_get_by_id(sub_id, fetched_subject), RST_OK);
    EXPECT_EQ(fetched_subject->get_language(), "es");

    std::string fetched_lang;
    EXPECT_EQ(session.get_subject_language(sub_id, fetched_lang), RST_OK);
    EXPECT_EQ(fetched_lang, "es");

    // 5. User Metrics
    std::shared_ptr<const UserMetrics> metrics;
    EXPECT_EQ(session.user_metrics_get(metrics), RST_OK);
    EXPECT_NE(metrics, nullptr);
    EXPECT_EQ(session.user_metrics_can_accept_file_size(1024), RST_OK);

    // 6. Practice Event Operations
    PracticeEvent practice;
    practice.set_subject_id(sub_id);
    EXPECT_EQ(session.practice_event_add_planned(practice), RST_OK);
    unsigned int practice_id = practice.get_id();
    EXPECT_NE(practice_id, 0u);

    std::shared_ptr<PracticeEvent> fetched_practice;
    EXPECT_EQ(session.practice_event_get_by_id(practice_id, fetched_practice), RST_OK);
    EXPECT_NE(fetched_practice, nullptr);

    std::vector<std::shared_ptr<PracticeEvent>> subject_practices;
    EXPECT_EQ(session.practice_event_get_by_subject(sub_id, subject_practices), RST_OK);
    EXPECT_GE(subject_practices.size(), 1u);

    std::vector<std::shared_ptr<PracticeEvent>> user_practices;
    EXPECT_EQ(session.practice_event_get_by_user(user_practices), RST_OK);
    EXPECT_GE(user_practices.size(), 1u);

    practice.set_description("Updated description");
    EXPECT_EQ(session.practice_event_update(practice), RST_OK);

    std::string sound_file = "dummy_practice.wav";
    {
        std::ofstream outfile(sound_file, std::ios::binary);
        outfile.write("RIFF", 4);
        int32_t chunk_size = 36 + 44100 * 2;
        outfile.write(reinterpret_cast<const char*>(&chunk_size), 4);
        outfile.write("WAVE", 4);
        outfile.write("fmt ", 4);
        int32_t sub_chunk1_size = 16;
        outfile.write(reinterpret_cast<const char*>(&sub_chunk1_size), 4);
        int16_t audio_format = 1;
        outfile.write(reinterpret_cast<const char*>(&audio_format), 2);
        int16_t num_channels = 1;
        outfile.write(reinterpret_cast<const char*>(&num_channels), 2);
        int32_t sample_rate = 44100;
        outfile.write(reinterpret_cast<const char*>(&sample_rate), 4);
        int32_t byte_rate = 44100 * 2;
        outfile.write(reinterpret_cast<const char*>(&byte_rate), 4);
        int16_t block_align = 2;
        outfile.write(reinterpret_cast<const char*>(&block_align), 2);
        int16_t bits_per_sample = 16;
        outfile.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
        outfile.write("data", 4);
        int32_t sub_chunk2_size = 44100 * 2;
        outfile.write(reinterpret_cast<const char*>(&sub_chunk2_size), 4);
        std::vector<char> data(sub_chunk2_size, 0);
        outfile.write(data.data(), sub_chunk2_size);
    }
    PracticeEvent recorded_practice;
    recorded_practice.set_subject_id(sub_id);
    EXPECT_EQ(session.practice_event_add_recorded(sound_file, recorded_practice), RST_OK);

    // 7. User Configuration operations
    UserConfiguration config;
    config.comparison.similarity_threshold = 0.85f;
    config.whisper.model_name = "small";
    EXPECT_EQ(session.set_user_config(config), RST_OK);
    EXPECT_EQ(session.get_user_config().comparison.similarity_threshold, 0.85f);
    EXPECT_EQ(session.load_user_config(), RST_OK);
    EXPECT_EQ(session.save_user_config(), RST_OK);

    // 8. User Logout and Re-login
    EXPECT_EQ(session.user_logout(), RST_OK);
    EXPECT_FALSE(session.user_is_authenticated());
    EXPECT_EQ(session.user_get(current_user), USER_NO_AUTH);

    EXPECT_EQ(session.user_identify(username, "new_hashed_secret"), RST_OK);
    EXPECT_TRUE(session.user_is_authenticated());

    // 9. Practice event bulk removal
    PracticeEvent p_extra;
    p_extra.set_subject_id(sub_id);
    EXPECT_EQ(session.practice_event_add_planned(p_extra), RST_OK);
    EXPECT_EQ(session.practice_event_remove_by_subject(sub_id), RST_OK);

    PracticeEvent p_extra2;
    p_extra2.set_subject_id(sub_id);
    EXPECT_EQ(session.practice_event_add_planned(p_extra2), RST_OK);
    EXPECT_EQ(session.practice_event_remove_all_by_user(), RST_OK);

    // 10. Cleanup operations
    EXPECT_EQ(session.subject_remove(sub_id), RST_OK);
    EXPECT_EQ(session.category_remove(cat_id), RST_OK);
    EXPECT_EQ(session.user_remove(), RST_OK);

    std::filesystem::remove(dummy_file);
    std::filesystem::remove(sound_file);
}

TEST_F(SessionTest, UnauthenticatedAccessReturnsNoAuth) {
    Session session;
    EXPECT_FALSE(session.user_is_authenticated());

    std::shared_ptr<const User> u;
    EXPECT_EQ(session.user_get(u), USER_NO_AUTH);
    EXPECT_EQ(session.user_get_by_name("some_name", u), USER_NO_AUTH);
    EXPECT_EQ(session.user_update(u), USER_NO_AUTH);
    EXPECT_EQ(session.user_remove(), USER_NO_AUTH);

    EXPECT_EQ(session.category_add("Name"), USER_NO_AUTH);
    EXPECT_EQ(session.category_update(1, "Name"), USER_NO_AUTH);
    EXPECT_EQ(session.category_remove(1), USER_NO_AUTH);
    std::shared_ptr<Category> cat;
    EXPECT_EQ(session.category_get_by_id(1, cat), USER_NO_AUTH);
    std::vector<std::shared_ptr<const Category>> cats;
    EXPECT_EQ(session.category_get_by_user(cats), USER_NO_AUTH);

    EXPECT_EQ(session.subject_add("Name", 1, "path.pdf"), USER_NO_AUTH);
    EXPECT_EQ(session.subject_update(1, "Name", 1, "path.pdf"), USER_NO_AUTH);
    EXPECT_EQ(session.subject_remove(1), USER_NO_AUTH);
    std::vector<std::shared_ptr<Subject>> subs;
    EXPECT_EQ(session.subject_get_by_user(subs), USER_NO_AUTH);
    std::shared_ptr<Subject> sub;
    EXPECT_EQ(session.subject_get_by_id(1, sub), USER_NO_AUTH);
    EXPECT_EQ(session.subject_get_by_category(1, subs), USER_NO_AUTH);
    EXPECT_EQ(session.set_subject_language(1, "es"), USER_NO_AUTH);
    std::string lang;
    EXPECT_EQ(session.get_subject_language(1, lang), USER_NO_AUTH);

    std::shared_ptr<const UserMetrics> metrics;
    EXPECT_EQ(session.user_metrics_get(metrics), USER_NO_AUTH);
    EXPECT_EQ(session.user_metrics_can_accept_file_size(100), USER_NO_AUTH);

    PracticeEvent pe;
    EXPECT_EQ(session.practice_event_add_planned(pe), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_add_recorded("path.wav", pe), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_update(pe), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_remove(1), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_remove_by_subject(1), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_remove_all_by_user(), USER_NO_AUTH);
    std::shared_ptr<PracticeEvent> fpe;
    EXPECT_EQ(session.practice_event_get_by_id(1, fpe), USER_NO_AUTH);
    std::vector<std::shared_ptr<PracticeEvent>> pes;
    EXPECT_EQ(session.practice_event_get_by_subject(1, pes), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_get_by_user(pes), USER_NO_AUTH);

    std::string exec_id, list_json, report_json, config_json;
    EXPECT_EQ(session.analyze_practice_coverage(1, exec_id), USER_NO_AUTH);
    UserConfiguration ucfg;
    EXPECT_EQ(session.analyze_practice_coverage(1, ucfg, exec_id), USER_NO_AUTH);
    EXPECT_EQ(session.get_analysis_executions_for_practice(1, list_json), USER_NO_AUTH);
    EXPECT_EQ(session.get_analysis_execution_details("exec-123", report_json, config_json), USER_NO_AUTH);
    EXPECT_EQ(session.get_analysis_execution_details_by_practice(1, report_json, config_json), USER_NO_AUTH);

    EXPECT_EQ(session.load_user_config(), USER_NO_AUTH);
    EXPECT_EQ(session.save_user_config(), USER_NO_AUTH);
}

TEST_F(SessionTest, EdgeAndErrorCases) {
    Session session;
    std::string name = "session_edge_user";
    std::string pass = "pass123";
    ASSERT_EQ(session.user_add(name, pass), RST_OK);
    ASSERT_EQ(session.user_identify(name, pass), RST_OK);

    // 1. Category update/remove/get by id not found
    std::shared_ptr<Category> cat;
    EXPECT_EQ(session.category_get_by_id(9999, cat), CATEGORY_NOT_FOUND);
    EXPECT_EQ(session.category_update(9999, "New Name"), CATEGORY_NOT_FOUND);
    EXPECT_EQ(session.category_remove(9999), CATEGORY_NOT_FOUND);

    // 2. Subject update / language get/set not found
    EXPECT_EQ(session.subject_update(9999, "New Name", 1, "some_path.pdf"), SUBJECT_NOT_FOUND);
    EXPECT_EQ(session.set_subject_language(9999, "es"), SUBJECT_NOT_FOUND);
    std::string lang;
    EXPECT_EQ(session.get_subject_language(9999, lang), SUBJECT_NOT_FOUND);

    // 3. Subject get by category not found
    std::vector<std::shared_ptr<Subject>> subjects;
    EXPECT_EQ(session.subject_get_by_category(9999, subjects), CATEGORY_NOT_FOUND);

    // 4. Practice event get for historic executions not found
    std::string list_json, report_json, config_json;
    EXPECT_EQ(session.get_analysis_executions_for_practice(9999, list_json), PRACTICE_EVENT_NOT_FOUND);
    EXPECT_EQ(session.get_analysis_execution_details_by_practice(9999, report_json, config_json), PRACTICE_EVENT_NOT_FOUND);

    // Clean up user
    EXPECT_EQ(session.user_remove(), RST_OK);
}

TEST_F(SessionTest, ModelsManagementQueries) {
    Session session;

    std::vector<ManagerModels::ModelInfo> all_models;
    EXPECT_EQ(session.models_get_all(false, all_models), RST_OK);

    std::vector<ManagerModels::ModelInfo> whisper_models;
    EXPECT_EQ(session.models_get_whisper(false, whisper_models), RST_OK);

    std::vector<ManagerModels::ModelInfo> llama_models;
    EXPECT_EQ(session.models_get_llama(false, llama_models), RST_OK);

    EXPECT_NE(session.models_is_whisper_available("non_existent_whisper_model"), RST_OK);
    EXPECT_NE(session.models_is_llama_available("non_existent_llama_model"), RST_OK);

    std::string auto_whisper = session.models_auto_select_whisper();
    EXPECT_FALSE(auto_whisper.empty());

    std::string auto_llama = session.models_auto_select_llama();
    EXPECT_FALSE(auto_llama.empty());
}

TEST_F(SessionTest, AudioOperationsWithMockSoundSystem) {
    auto user_metrics_op = std::make_shared<OperationUserMetrics>();
    auto category_op = std::make_shared<OperationCategory>();
    auto user_op = std::make_shared<OperationUser>(user_metrics_op);
    auto mock_sub_op = std::make_shared<MockOperationSubject>();
    auto mock_practice_op = std::make_shared<MockOperationPracticeEvent>();
    auto mock_coverage_op = std::make_shared<MockOperationCoverage>();
    auto mock_db_op = std::make_shared<MockDatabase>();
    auto mock_sound = std::make_shared<MockSoundSystem>();
    auto models_mgr = std::make_shared<ManagerModels>();

    std::vector<ISoundSystem::SoundSystemDeviceInfo> fake_devices = {
        {0, "Default Mic", true},
        {1, "Secondary Mic", false}
    };

    EXPECT_CALL(*mock_sound, getCaptureDevices())
        .WillOnce(Return(fake_devices));
    EXPECT_CALL(*mock_sound, startRecording(_, 0))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_sound, isRecording())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_sound, get_recording_timestamp())
        .WillOnce(Return(1234ULL));
    EXPECT_CALL(*mock_sound, stopRecording())
        .Times(1);

    EXPECT_CALL(*mock_sound, play(_, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_sound, isPlaying())
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_sound, get_playing_timestamp())
        .WillOnce(Return(5678ULL));
    EXPECT_CALL(*mock_sound, stopPlaying())
        .Times(1);

    Session session(
        std::move(user_op),
        std::move(category_op),
        std::move(mock_sub_op),
        std::move(user_metrics_op),
        std::move(mock_practice_op),
        std::move(mock_coverage_op),
        std::move(mock_db_op),
        std::move(mock_sound),
        std::move(models_mgr)
    );

    std::vector<ISoundSystem::SoundSystemDeviceInfo> devices;
    EXPECT_EQ(session.audio_get_capture_devices(devices), RST_OK);
    ASSERT_EQ(devices.size(), 2u);
    EXPECT_EQ(devices[0].name, "Default Mic");

    EXPECT_EQ(session.audio_start_recording("test.opus", 0), RST_OK);
    EXPECT_TRUE(session.audio_is_recording());
    EXPECT_EQ(session.audio_get_recording_timestamp(), 1234ULL);
    EXPECT_EQ(session.audio_stop_recording(), RST_OK);

    EXPECT_EQ(session.audio_play("test.opus"), RST_OK);
    EXPECT_TRUE(session.audio_is_playing());
    EXPECT_EQ(session.audio_get_playing_timestamp(), 5678ULL);
    EXPECT_EQ(session.audio_stop_playing(), RST_OK);
}

TEST_F(SessionTest, InjectedCoverageAndHistoryQueries) {
    auto user_metrics_op = std::make_shared<OperationUserMetrics>();
    auto category_op = std::make_shared<OperationCategory>();
    auto user_op = std::make_shared<OperationUser>(user_metrics_op);
    auto mock_sub_op = std::make_shared<MockOperationSubject>();
    auto mock_practice_op = std::make_shared<MockOperationPracticeEvent>();
    auto mock_coverage_op = std::make_shared<MockOperationCoverage>();
    auto mock_db_op = std::make_shared<MockDatabase>();

    // Test constructor with injected mocks and concrete operations
    Session session(
        std::move(user_op),
        std::move(category_op),
        std::move(mock_sub_op),
        std::move(user_metrics_op),
        std::move(mock_practice_op),
        std::move(mock_coverage_op),
        std::move(mock_db_op)
    );
    EXPECT_FALSE(session.user_is_authenticated());
}

TEST_F(SessionTest, GetHardwareInfoDefault) {
    Session session;
    cantatema::HardwareInfo hw = session.get_hardware_info();
    EXPECT_FALSE(hw.cpu.name.empty());
    EXPECT_GT(hw.cpu.core_count, 0u);

    cantatema::HardwareInfo hw_ref;
    EXPECT_EQ(session.get_hardware_info(hw_ref), RST_OK);
    EXPECT_EQ(hw_ref.cpu.name, hw.cpu.name);
}

TEST_F(SessionTest, GetHardwareInfoInjectedMock) {
    auto user_metrics_op = std::make_shared<OperationUserMetrics>();
    auto category_op = std::make_shared<OperationCategory>();
    auto user_op = std::make_shared<OperationUser>(user_metrics_op);
    auto mock_sub_op = std::make_shared<MockOperationSubject>();
    auto mock_practice_op = std::make_shared<MockOperationPracticeEvent>();
    auto mock_coverage_op = std::make_shared<MockOperationCoverage>();
    auto mock_db_op = std::make_shared<MockDatabase>();
    auto mock_sound = std::make_shared<MockSoundSystem>();
    auto models_mgr = std::make_shared<ManagerModels>();
    auto mock_gpu_detector = std::make_shared<StrictMock<MockGpuDetector>>();

    cantatema::HardwareInfo mock_hw;
    mock_hw.cpu.name = "Mock CPU Ryzen 9";
    mock_hw.cpu.architecture = "x86_64";
    mock_hw.cpu.core_count = 16;
    mock_hw.has_cuda = true;
    mock_hw.has_any_gpu = true;
    mock_hw.use_gpu = true;
    mock_hw.selected_backend = "CUDA";

    cantatema::GpuInfo gpu;
    gpu.name = "CUDA0";
    gpu.description = "NVIDIA GeForce RTX 4090";
    gpu.backend_name = "CUDA";
    gpu.is_gpu = true;
    gpu.memory_total_mb = 24576;
    mock_hw.gpus.push_back(gpu);

    EXPECT_CALL(*mock_gpu_detector, detect_hardware())
        .WillRepeatedly(Return(mock_hw));

    Session session(
        std::move(user_op),
        std::move(category_op),
        std::move(mock_sub_op),
        std::move(user_metrics_op),
        std::move(mock_practice_op),
        std::move(mock_coverage_op),
        std::move(mock_db_op),
        std::move(mock_sound),
        std::move(models_mgr),
        std::move(mock_gpu_detector)
    );

    cantatema::HardwareInfo result = session.get_hardware_info();
    EXPECT_EQ(result.cpu.name, "Mock CPU Ryzen 9");
    EXPECT_EQ(result.cpu.core_count, 16u);
    EXPECT_TRUE(result.has_cuda);
    EXPECT_TRUE(result.use_gpu);
    ASSERT_EQ(result.gpus.size(), 1u);
    EXPECT_EQ(result.gpus[0].description, "NVIDIA GeForce RTX 4090");
}

TEST_F(SessionTest, SessionTaskSchedulerFlow) {
    auto user_metrics_op = std::make_shared<OperationUserMetrics>();
    auto category_op = std::make_shared<OperationCategory>();
    auto user_op = std::make_shared<OperationUser>(user_metrics_op);
    auto mock_sub_op = std::make_shared<MockOperationSubject>();
    auto mock_practice_op = std::make_shared<MockOperationPracticeEvent>();
    auto mock_coverage_op = std::make_shared<MockOperationCoverage>();
    auto mock_db_op = std::make_shared<MockDatabase>();
    auto mock_sound = std::make_shared<MockSoundSystem>();
    auto models_mgr = std::make_shared<ManagerModels>();
    auto mock_gpu_detector = std::make_shared<StrictMock<MockGpuDetector>>();
    auto mock_scheduler = std::make_shared<StrictMock<MockOperationAnalysisScheduler>>();
    auto mock_scheduler_ptr = mock_scheduler.get();

    EXPECT_CALL(*mock_scheduler_ptr, start_scheduler())
        .WillOnce(Return(RST_OK));
    EXPECT_CALL(*mock_scheduler_ptr, stop_scheduler())
        .WillOnce(Return(RST_OK));

    Session session(
        std::move(user_op),
        std::move(category_op),
        std::move(mock_sub_op),
        std::move(user_metrics_op),
        std::move(mock_practice_op),
        std::move(mock_coverage_op),
        std::move(mock_db_op),
        std::move(mock_sound),
        std::move(models_mgr),
        std::move(mock_gpu_detector),
        std::move(mock_scheduler)
    );

    // Unauthenticated calls should fail
    std::string task_id;
    EXPECT_EQ(session.analysis_task_submit(1, task_id), USER_NO_AUTH);
    EXPECT_EQ(session.analysis_task_cancel("t1"), USER_NO_AUTH);
    AnalysisTask task_status;
    EXPECT_EQ(session.analysis_task_get_status("t1", task_status), USER_NO_AUTH);
    std::vector<AnalysisTask> tasks_list;
    EXPECT_EQ(session.analysis_task_get_user_tasks(tasks_list), USER_NO_AUTH);
    EXPECT_EQ(session.analysis_task_get_all_tasks(tasks_list), USER_NO_AUTH);

    // Identify user
    EXPECT_EQ(session.user_add("sched_user", "password123"), RST_OK);
    EXPECT_EQ(session.user_identify("sched_user", "password123"), RST_OK);

    // 1. Submit task
    EXPECT_CALL(*mock_scheduler_ptr, submit_task(_, 1, _, _))
        .WillOnce(DoAll(testing::SetArgReferee<3>("task-uuid-abc"), Return(RST_OK)));
    EXPECT_EQ(session.analysis_task_submit(1, task_id), RST_OK);
    EXPECT_EQ(task_id, "task-uuid-abc");

    // 2. Cancel task
    EXPECT_CALL(*mock_scheduler_ptr, cancel_task(_, "task-uuid-abc"))
        .WillOnce(Return(RST_OK));
    EXPECT_EQ(session.analysis_task_cancel("task-uuid-abc"), RST_OK);

    // 3. Get status
    AnalysisTask sample_task("task-uuid-abc", 1, 1);
    sample_task.set_status(AnalysisTaskStatus::COMPLETED);
    EXPECT_CALL(*mock_scheduler_ptr, get_task_status(_, "task-uuid-abc", _))
        .WillOnce(DoAll(testing::SetArgReferee<2>(sample_task), Return(RST_OK)));
    EXPECT_EQ(session.analysis_task_get_status("task-uuid-abc", task_status), RST_OK);
    EXPECT_EQ(task_status.get_status(), AnalysisTaskStatus::COMPLETED);

    // 4. Get user tasks
    std::vector<AnalysisTask> expected_tasks = { sample_task };
    EXPECT_CALL(*mock_scheduler_ptr, get_user_tasks(_, _))
        .WillOnce(DoAll(testing::SetArgReferee<1>(expected_tasks), Return(RST_OK)));
    EXPECT_EQ(session.analysis_task_get_user_tasks(tasks_list), RST_OK);
    EXPECT_EQ(tasks_list.size(), 1u);

    // 5. Get all tasks (admin)
    EXPECT_CALL(*mock_scheduler_ptr, get_all_tasks(_, _))
        .WillOnce(DoAll(testing::SetArgReferee<1>(expected_tasks), Return(RST_OK)));
    EXPECT_EQ(session.analysis_task_get_all_tasks(tasks_list), RST_OK);
    EXPECT_EQ(tasks_list.size(), 1u);

    // 6. Max parallel tasks setter/getter
    EXPECT_CALL(*mock_scheduler_ptr, set_max_parallel_tasks(4))
        .Times(1);
    session.analysis_task_set_max_parallel(4);

    EXPECT_CALL(*mock_scheduler_ptr, get_max_parallel_tasks())
        .WillOnce(Return(4));
    EXPECT_EQ(session.analysis_task_get_max_parallel(), 4u);
}


