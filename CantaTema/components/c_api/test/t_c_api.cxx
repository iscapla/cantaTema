/**
 * @file t_c_api.cxx
 * @brief Unit tests for C ABI export bridge (cantatema_bridge).
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "c_api/cantatema_c_api.h"
#include "primitives/definitions.hpp"
#include "primitives/utils_logger.hpp"

class CApiTest : public ::testing::Test {
protected:
    void SetUp() override {
        canta_init_logger_for_test();
        canta_shutdown_engine();
        canta_purge_database_for_test();
    }

    void TearDown() override {
        canta_shutdown_engine();
    }
};

TEST_F(CApiTest, UninitializedEngineReturnsAppropriateDefaults) {
    canta_shutdown_engine();

    EXPECT_EQ(canta_get_current_user_json(), nullptr);

    const char* cats_json = canta_get_categories_json();
    ASSERT_NE(cats_json, nullptr);
    EXPECT_STREQ(cats_json, "[]");
    canta_free_string(cats_json);

    const char* topics_json = canta_get_all_topics_json();
    ASSERT_NE(topics_json, nullptr);
    EXPECT_STREQ(topics_json, "[]");
    canta_free_string(topics_json);

    const char* cal_json = canta_get_calendar_events_json();
    ASSERT_NE(cal_json, nullptr);
    EXPECT_STREQ(cal_json, "[]");
    canta_free_string(cal_json);

    EXPECT_EQ(canta_start_recording_session("1"), static_cast<int32_t>(USER_NO_AUTH));
    EXPECT_FLOAT_EQ(canta_get_current_audio_amplitude(), 0.0f);
}

TEST_F(CApiTest, VersionAndErrorText) {
    const char* ver = canta_get_engine_version();
    ASSERT_NE(ver, nullptr);
    EXPECT_TRUE(std::string(ver).find("1.0.0") != std::string::npos);
    canta_free_string(ver);

    const char* ok_text = canta_get_error_text(0);
    ASSERT_NE(ok_text, nullptr);
    EXPECT_STREQ(ok_text, "RST_OK");
    canta_free_string(ok_text);

    const char* auth_text = canta_get_error_text(static_cast<int32_t>(USER_NO_AUTH));
    ASSERT_NE(auth_text, nullptr);
    EXPECT_STREQ(auth_text, "USER_NO_AUTH");
    canta_free_string(auth_text);
}

TEST_F(CApiTest, EngineLifecycleAndSingleSessionLimit) {
    EXPECT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));

    // Calling init again cleanly resets and enforces max 1 active session
    EXPECT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));

    canta_shutdown_engine();
    EXPECT_EQ(canta_get_current_user_json(), nullptr);
}

TEST_F(CApiTest, UserAuthenticationFlow) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));

    // Initial state: not authenticated
    EXPECT_EQ(canta_get_current_user_json(), nullptr);

    // Register user
    const char* reg_json = canta_register_user_json("candidate_juan", "pass123", "Juan Perez");
    ASSERT_NE(reg_json, nullptr);
    auto j_reg = nlohmann::json::parse(reg_json);
    EXPECT_EQ(j_reg["name"], "candidate_juan");
    EXPECT_EQ(j_reg["displayName"], "Juan Perez");
    canta_free_string(reg_json);

    // Current user should now be active
    const char* cur_json = canta_get_current_user_json();
    ASSERT_NE(cur_json, nullptr);
    auto j_cur = nlohmann::json::parse(cur_json);
    EXPECT_EQ(j_cur["name"], "candidate_juan");
    EXPECT_EQ(j_cur["firstName"], "Juan");
    EXPECT_EQ(j_cur["lastName"], "Perez");
    canta_free_string(cur_json);

    // Logout
    EXPECT_EQ(canta_logout_user(), static_cast<int32_t>(RST_OK));
    EXPECT_EQ(canta_get_current_user_json(), nullptr);

    // Login with wrong password fails
    EXPECT_EQ(canta_login_user_json("candidate_juan", "wrongpass"), nullptr);

    // Login with correct password succeeds
    const char* login_json = canta_login_user_json("candidate_juan", "pass123");
    ASSERT_NE(login_json, nullptr);
    auto j_login = nlohmann::json::parse(login_json);
    EXPECT_EQ(j_login["name"], "candidate_juan");
    EXPECT_EQ(j_login["firstName"], "Juan");
    EXPECT_EQ(j_login["lastName"], "Perez");
    canta_free_string(login_json);
}

TEST_F(CApiTest, TopicAndCategoryCRUD) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    const char* u = canta_register_user_json("maria_candidate", "pass123", "Maria Gomez");
    canta_free_string(u);

    // Categories initially empty
    const char* cats_init = canta_get_categories_json();
    EXPECT_STREQ(cats_init, "[]");
    canta_free_string(cats_init);

    // Create topic with auto-created category
    std::string payload = R"({
        "title": "Tema 1: La Corona",
        "categoryName": "Derecho Constitucional",
        "filePath": "corona.pdf",
        "tags": ["constitucional", "monarquia"],
        "language": "es"
    })";

    const char* created_json = canta_create_topic_json(payload.c_str());
    ASSERT_NE(created_json, nullptr);
    auto j_topic = nlohmann::json::parse(created_json);
    EXPECT_EQ(j_topic["title"], "Tema 1: La Corona");
    EXPECT_EQ(j_topic["language"], "es");
    std::string topic_id = j_topic["id"];
    std::string cat_id = j_topic["categoryId"];
    canta_free_string(created_json);

    // Categories should now have 1 item
    const char* cats = canta_get_categories_json();
    ASSERT_NE(cats, nullptr);
    auto j_cats = nlohmann::json::parse(cats);
    EXPECT_EQ(j_cats.size(), 1u);
    EXPECT_EQ(j_cats[0]["name"], "Derecho Constitucional");
    canta_free_string(cats);

    // Get all topics
    const char* all_topics = canta_get_all_topics_json();
    ASSERT_NE(all_topics, nullptr);
    auto j_all = nlohmann::json::parse(all_topics);
    EXPECT_EQ(j_all.size(), 1u);
    EXPECT_EQ(j_all[0]["title"], "Tema 1: La Corona");
    canta_free_string(all_topics);

    // Get topic by category
    const char* cat_topics = canta_get_topics_by_category_json(cat_id.c_str());
    ASSERT_NE(cat_topics, nullptr);
    auto j_cat_topics = nlohmann::json::parse(cat_topics);
    EXPECT_EQ(j_cat_topics.size(), 1u);
    canta_free_string(cat_topics);

    // Get topic by ID
    const char* topic_by_id = canta_get_topic_by_id_json(topic_id.c_str());
    ASSERT_NE(topic_by_id, nullptr);
    auto j_by_id = nlohmann::json::parse(topic_by_id);
    EXPECT_EQ(j_by_id["title"], "Tema 1: La Corona");
    canta_free_string(topic_by_id);

    // Update topic
    std::string update_payload = "{\"id\":\"" + topic_id + "\", \"title\":\"Tema 1: La Corona y Sucesion\"}";
    EXPECT_EQ(canta_update_topic_json(update_payload.c_str()), static_cast<int32_t>(RST_OK));

    // Verify update
    const char* updated_json = canta_get_topic_by_id_json(topic_id.c_str());
    ASSERT_NE(updated_json, nullptr);
    auto j_updated = nlohmann::json::parse(updated_json);
    EXPECT_EQ(j_updated["title"], "Tema 1: La Corona y Sucesion");
    canta_free_string(updated_json);

    // Delete topic
    EXPECT_EQ(canta_delete_topic(topic_id.c_str()), static_cast<int32_t>(RST_OK));
    EXPECT_EQ(canta_get_topic_by_id_json(topic_id.c_str()), nullptr);
}

TEST_F(CApiTest, AudioMeteringAndPlaybackControls) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    const char* u = canta_register_user_json("audio_user", "pass", "Audio User");
    canta_free_string(u);

    // Amplitude while idle is 0.0f
    EXPECT_FLOAT_EQ(canta_get_current_audio_amplitude(), 0.0f);

    // Pause / resume recording controls
    EXPECT_EQ(canta_pause_recording_session(), static_cast<int32_t>(RST_OK));
    EXPECT_FLOAT_EQ(canta_get_current_audio_amplitude(), 0.0f);
    EXPECT_EQ(canta_resume_recording_session(), static_cast<int32_t>(RST_OK));

    // STT live token streaming poll returns NULL initially
    EXPECT_EQ(canta_poll_live_transcription(), nullptr);

    // Playback on nonexistent session returns PRACTICE_EVENT_NOT_FOUND
    EXPECT_EQ(canta_start_playback("99999"), static_cast<int32_t>(PRACTICE_EVENT_NOT_FOUND));
    EXPECT_EQ(canta_stop_playback(), static_cast<int32_t>(RST_OK));

    // Stop recording when not recording returns null
    EXPECT_EQ(canta_stop_recording_session(), nullptr);
}

TEST_F(CApiTest, CalendarEventsAndSchedulerFlow) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    const char* u = canta_register_user_json("calendar_user", "pass", "Cal User");
    canta_free_string(u);

    // Create a topic
    std::string topic_payload = R"({"title":"Tema Penal 1","categoryName":"Derecho Penal"})";
    const char* top_json = canta_create_topic_json(topic_payload.c_str());
    ASSERT_NE(top_json, nullptr);
    auto j_top = nlohmann::json::parse(top_json);
    std::string topic_id = j_top["id"];
    canta_free_string(top_json);

    // Initially 0 calendar events
    const char* cal_init = canta_get_calendar_events_json();
    EXPECT_STREQ(cal_init, "[]");
    canta_free_string(cal_init);

    // Schedule a future planned cante
    std::string cal_payload = "{\"topicId\":\"" + topic_id + "\", \"date\":1750000000, \"description\":\"Repaso primer cante\"}";
    EXPECT_EQ(canta_add_calendar_event_json(cal_payload.c_str()), static_cast<int32_t>(RST_OK));

    // Query calendar events
    const char* cal_after = canta_get_calendar_events_json();
    ASSERT_NE(cal_after, nullptr);
    auto j_cal = nlohmann::json::parse(cal_after);
    EXPECT_EQ(j_cal.size(), 1u);
    EXPECT_EQ(j_cal[0]["status"], "planned");
    std::string event_id = j_cal[0]["id"];
    canta_free_string(cal_after);

    // Remove calendar event
    EXPECT_EQ(canta_remove_calendar_event(event_id.c_str()), static_cast<int32_t>(RST_OK));

    const char* cal_empty = canta_get_calendar_events_json();
    EXPECT_STREQ(cal_empty, "[]");
    canta_free_string(cal_empty);

    // Scheduler items
    const char* schedule_items = canta_get_schedule_items_json(-1, nullptr);
    ASSERT_NE(schedule_items, nullptr);
    canta_free_string(schedule_items);
}

TEST_F(CApiTest, LocalAIModelQueryingAndUpdating) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    const char* u = canta_register_user_json("model_user", "pass", "Model User");
    canta_free_string(u);

    const char* models_json = canta_get_ai_models_json();
    ASSERT_NE(models_json, nullptr);
    auto j_models = nlohmann::json::parse(models_json);
    EXPECT_TRUE(j_models.is_array());
    canta_free_string(models_json);

    // Update active model
    std::string update_payload = R"({"name":"ggml-base.bin","type":"whisper","isActive":true})";
    EXPECT_EQ(canta_update_ai_model_json(update_payload.c_str()), static_cast<int32_t>(RST_OK));
}

TEST_F(CApiTest, DecryptedAudioStreamingRangeParameters) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    const char* u = canta_register_user_json("stream_user", "pass", "Stream User");
    canta_free_string(u);

    // Null buffer argument check
    EXPECT_EQ(canta_read_audio_stream(1, 0, 100, nullptr, nullptr, nullptr), static_cast<int32_t>(DB_BAD_PARAM));

    uint8_t buffer[64];
    uint32_t bytes_read = 0;
    int32_t is_eof = 0;

    // Nonexistent session returns PRACTICE_EVENT_NOT_FOUND
    EXPECT_EQ(canta_read_audio_stream(9999, 0, 64, buffer, &bytes_read, &is_eof), static_cast<int32_t>(PRACTICE_EVENT_NOT_FOUND));
}

TEST_F(CApiTest, ScheduleItemsFullWorkflow) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    const char* u = canta_register_user_json("sched_user", "pass", "Sched User");
    canta_free_string(u);

    // Create topic
    std::string top_payload = R"({"title":"Tema Fiscal 1","categoryName":"Hacienda Publica"})";
    const char* top_json = canta_create_topic_json(top_payload.c_str());
    ASSERT_NE(top_json, nullptr);
    auto j_top = nlohmann::json::parse(top_json);
    std::string topic_id = j_top["id"];
    canta_free_string(top_json);

    // Create practice event
    std::string cal_payload = "{\"topicId\":\"" + topic_id + "\", \"date\":1760000000, \"description\":\"Examen parcial\"}";
    ASSERT_EQ(canta_add_calendar_event_json(cal_payload.c_str()), static_cast<int32_t>(RST_OK));

    const char* cal_json = canta_get_calendar_events_json();
    ASSERT_NE(cal_json, nullptr);
    auto j_cal = nlohmann::json::parse(cal_json);
    ASSERT_GE(j_cal.size(), 1u);
    std::string practice_id = j_cal[0]["id"];
    canta_free_string(cal_json);

    // Create schedule item (analysis task)
    std::string sched_payload = "{\"practiceId\":" + practice_id + "}";
    const char* item_json = canta_create_schedule_item_json(sched_payload.c_str());
    ASSERT_NE(item_json, nullptr);
    auto j_item = nlohmann::json::parse(item_json);
    std::string task_id = j_item["id"];
    EXPECT_FALSE(task_id.empty());
    EXPECT_EQ(j_item["status"], "waiting");
    canta_free_string(item_json);

    // Query schedule items with status filter
    const char* all_items = canta_get_schedule_items_json(-1, nullptr);
    ASSERT_NE(all_items, nullptr);
    auto j_all_items = nlohmann::json::parse(all_items);
    EXPECT_GE(j_all_items.size(), 1u);
    canta_free_string(all_items);

    // Query schedule items with search filter
    const char* filtered_items = canta_get_schedule_items_json(-1, "task-");
    ASSERT_NE(filtered_items, nullptr);
    canta_free_string(filtered_items);

    // Filter with non-matching status
    const char* status_filtered = canta_get_schedule_items_json(999, nullptr);
    ASSERT_NE(status_filtered, nullptr);
    canta_free_string(status_filtered);

    // Filter with non-matching search string
    const char* query_filtered = canta_get_schedule_items_json(-1, "non_matching_search_str_12345");
    ASSERT_NE(query_filtered, nullptr);
    canta_free_string(query_filtered);

    // Update schedule item status
    EXPECT_EQ(canta_update_schedule_item_status(task_id.c_str(), 3), static_cast<int32_t>(RST_OK));

    // Delete schedule item
    EXPECT_EQ(canta_delete_schedule_item(task_id.c_str()), static_cast<int32_t>(RST_OK));
}

TEST_F(CApiTest, SessionHistoryAndAnalysisFlow) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    const char* u = canta_register_user_json("history_user", "pass", "History User");
    canta_free_string(u);

    // Create topic with category
    std::string top_payload = R"({"title":"Tema Procesal 1","categoryName":"Derecho Procesal"})";
    const char* top_json = canta_create_topic_json(top_payload.c_str());
    ASSERT_NE(top_json, nullptr);
    auto j_top = nlohmann::json::parse(top_json);
    std::string topic_id = j_top["id"];
    canta_free_string(top_json);

    // Add recorded study session
    const char* study_json = canta_create_study_session_json(topic_id.c_str(), nullptr, 90);
    ASSERT_NE(study_json, nullptr);
    auto j_study = nlohmann::json::parse(study_json);
    std::string session_id = j_study["id"];
    canta_free_string(study_json);

    // Add calendar event
    std::string cal_payload = "{\"topicId\":\"" + topic_id + "\", \"date\":1765000000, \"description\":\"Simulacro oral\"}";
    ASSERT_EQ(canta_add_calendar_event_json(cal_payload.c_str()), static_cast<int32_t>(RST_OK));

    // Query sessions for topic
    const char* topic_sessions = canta_get_sessions_for_topic_json(topic_id.c_str());
    ASSERT_NE(topic_sessions, nullptr);
    auto j_top_sess = nlohmann::json::parse(topic_sessions);
    EXPECT_GE(j_top_sess.size(), 1u);
    canta_free_string(topic_sessions);

    // Query recent sessions
    const char* recent_sessions = canta_get_recent_sessions_json();
    ASSERT_NE(recent_sessions, nullptr);
    auto j_rec_sess = nlohmann::json::parse(recent_sessions);
    EXPECT_GE(j_rec_sess.size(), 1u);
    EXPECT_FALSE(j_rec_sess[0]["topicTitle"].get<std::string>().empty());
    canta_free_string(recent_sessions);

    // Query session by ID
    const char* sess_by_id = canta_get_session_by_id_json(session_id.c_str());
    ASSERT_NE(sess_by_id, nullptr);
    auto j_by_id = nlohmann::json::parse(sess_by_id);
    EXPECT_EQ(j_by_id["id"], session_id);
    EXPECT_FALSE(j_by_id["topicTitle"].get<std::string>().empty());
    canta_free_string(sess_by_id);

    // Test audio stream reading
    uint8_t stream_buf[128];
    uint32_t bytes_read = 0;
    int32_t is_eof = 0;
    uint32_t s_id_num = static_cast<uint32_t>(std::stoul(session_id));
    EXPECT_EQ(canta_read_audio_stream(s_id_num, 0, sizeof(stream_buf), stream_buf, &bytes_read, &is_eof), static_cast<int32_t>(RST_OK));
    EXPECT_GT(bytes_read, 0u);

    // Playback session
    canta_start_playback(session_id.c_str());
    EXPECT_EQ(canta_stop_playback(), static_cast<int32_t>(RST_OK));

    // Generate analysis for session
    const char* gen_report = canta_generate_session_analysis_json(session_id.c_str());
    if (gen_report) {
        canta_free_string(gen_report);
    }

    // Query analysis report for session
    const char* report = canta_get_session_analysis_report_json(session_id.c_str());
    if (report) {
        canta_free_string(report);
    }

    // Create topic without category to test fallback empty categoryName
    std::string no_cat_payload = R"({"title":"Tema Sin Categoria"})";
    const char* top_no_cat = canta_create_topic_json(no_cat_payload.c_str());
    ASSERT_NE(top_no_cat, nullptr);
    auto j_no_cat = nlohmann::json::parse(top_no_cat);
    std::string no_cat_id = j_no_cat["id"];
    canta_free_string(top_no_cat);

    const char* all_topics = canta_get_all_topics_json();
    ASSERT_NE(all_topics, nullptr);
    canta_free_string(all_topics);

    const char* topic_det = canta_get_topic_by_id_json(no_cat_id.c_str());
    if (topic_det) canta_free_string(topic_det);

    EXPECT_EQ(canta_delete_topic(no_cat_id.c_str()), static_cast<int32_t>(RST_OK));

    // Delete topic to test fallback empty topicTitle
    EXPECT_EQ(canta_delete_topic(topic_id.c_str()), static_cast<int32_t>(RST_OK));
    const char* recent_after_topic_del = canta_get_recent_sessions_json();
    ASSERT_NE(recent_after_topic_del, nullptr);
    canta_free_string(recent_after_topic_del);

    const char* sess_after_topic_del = canta_get_session_by_id_json(session_id.c_str());
    ASSERT_NE(sess_after_topic_del, nullptr);
    canta_free_string(sess_after_topic_del);

    const char* cal_after_topic_del = canta_get_calendar_events_json();
    ASSERT_NE(cal_after_topic_del, nullptr);
    canta_free_string(cal_after_topic_del);

    // Test unauthenticated calls to session analysis
    canta_logout_user();
    EXPECT_EQ(canta_generate_session_analysis_json(session_id.c_str()), nullptr);
    EXPECT_EQ(canta_get_session_analysis_report_json(session_id.c_str()), nullptr);
    canta_login_user_json("history_user", "pass");

    // Delete session
    EXPECT_EQ(canta_delete_session(session_id.c_str()), static_cast<int32_t>(RST_OK));
    EXPECT_EQ(canta_get_session_by_id_json(session_id.c_str()), nullptr);
}

TEST_F(CApiTest, RecordingFlowEdgeCasesAndControls) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    const char* u = canta_register_user_json("record_user", "pass", "Record User");
    canta_free_string(u);

    // Create topic
    std::string top_payload = R"({"title":"Tema Grabacion","categoryName":"Pruebas"})";
    const char* top_json = canta_create_topic_json(top_payload.c_str());
    ASSERT_NE(top_json, nullptr);
    auto j_top = nlohmann::json::parse(top_json);
    std::string topic_id = j_top["id"];
    canta_free_string(top_json);

    // Enable mock recording for test environment
    canta_set_mock_recording_for_test(1);

    // Start recording session
    int32_t rec_res = canta_start_recording_session(topic_id.c_str());
    EXPECT_EQ(rec_res, static_cast<int32_t>(RST_OK));

    // Amplitude metering while recording
    EXPECT_FLOAT_EQ(canta_get_current_audio_amplitude(), 0.42f);

    // Cannot start another recording while one is active
    EXPECT_EQ(canta_start_recording_session(topic_id.c_str()), static_cast<int32_t>(PRACTICE_EVENT_ERROR));

    // Pause and resume
    EXPECT_EQ(canta_pause_recording_session(), static_cast<int32_t>(RST_OK));
    EXPECT_FLOAT_EQ(canta_get_current_audio_amplitude(), 0.0f);
    EXPECT_EQ(canta_resume_recording_session(), static_cast<int32_t>(RST_OK));
    EXPECT_FLOAT_EQ(canta_get_current_audio_amplitude(), 0.42f);

    // Stop recording session
    const char* saved_sess = canta_stop_recording_session();
    ASSERT_NE(saved_sess, nullptr);
    auto j_saved = nlohmann::json::parse(saved_sess);
    EXPECT_FALSE(j_saved["id"].get<std::string>().empty());
    EXPECT_EQ(j_saved["topicId"], topic_id);
    canta_free_string(saved_sess);

    // Test starting recording again and then re-initializing engine
    EXPECT_EQ(canta_start_recording_session(topic_id.c_str()), static_cast<int32_t>(RST_OK));
    EXPECT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));

    // Re-login after init
    const char* re_login = canta_login_user_json("record_user", "pass");
    ASSERT_NE(re_login, nullptr);
    canta_free_string(re_login);

    // Test start recording and then shutdown engine
    EXPECT_EQ(canta_start_recording_session(topic_id.c_str()), static_cast<int32_t>(RST_OK));
    canta_shutdown_engine();

    // Disable mock recording
    canta_set_mock_recording_for_test(0);
}

TEST_F(CApiTest, NullAndInvalidInputEdgeCases) {
    ASSERT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    const char* u = canta_register_user_json("edge_user", "pass", "Edge User");
    canta_free_string(u);

    // Null string pointer deallocation safe check
    canta_free_string(nullptr);

    // Null parameters on user management
    EXPECT_EQ(canta_login_user_json(nullptr, nullptr), nullptr);
    EXPECT_EQ(canta_register_user_json(nullptr, nullptr, nullptr), nullptr);

    // Null parameters on topic management
    EXPECT_EQ(canta_create_topic_json(nullptr), nullptr);
    EXPECT_EQ(canta_update_topic_json(nullptr), static_cast<int32_t>(DB_BAD_PARAM));
    EXPECT_EQ(canta_delete_topic(nullptr), static_cast<int32_t>(DB_BAD_PARAM));
    EXPECT_EQ(canta_get_topic_by_id_json(nullptr), nullptr);
    const char* empty_cat_topics = canta_get_topics_by_category_json(nullptr);
    ASSERT_NE(empty_cat_topics, nullptr);
    EXPECT_STREQ(empty_cat_topics, "[]");
    canta_free_string(empty_cat_topics);

    // Null parameters on session history
    const char* empty_topic_sess = canta_get_sessions_for_topic_json(nullptr);
    ASSERT_NE(empty_topic_sess, nullptr);
    EXPECT_STREQ(empty_topic_sess, "[]");
    canta_free_string(empty_topic_sess);
    EXPECT_EQ(canta_get_session_by_id_json(nullptr), nullptr);
    EXPECT_EQ(canta_delete_session(nullptr), static_cast<int32_t>(DB_BAD_PARAM));

    // Null parameters on scheduling and analysis
    EXPECT_EQ(canta_create_schedule_item_json(nullptr), nullptr);
    EXPECT_EQ(canta_update_schedule_item_status(nullptr, 0), static_cast<int32_t>(DB_BAD_PARAM));
    EXPECT_EQ(canta_delete_schedule_item(nullptr), static_cast<int32_t>(DB_BAD_PARAM));
    EXPECT_EQ(canta_generate_session_analysis_json(nullptr), nullptr);
    EXPECT_EQ(canta_get_session_analysis_report_json(nullptr), nullptr);

    // Null parameters on calendar and models
    EXPECT_EQ(canta_add_calendar_event_json(nullptr), static_cast<int32_t>(DB_BAD_PARAM));
    EXPECT_EQ(canta_remove_calendar_event(nullptr), static_cast<int32_t>(DB_BAD_PARAM));
    EXPECT_EQ(canta_update_ai_model_json(nullptr), static_cast<int32_t>(DB_BAD_PARAM));

    // Malformed JSON strings while authenticated
    EXPECT_EQ(canta_create_topic_json("{bad_json"), nullptr);
    EXPECT_EQ(canta_update_topic_json("{bad_json"), static_cast<int32_t>(UNKNOWN));
    EXPECT_EQ(canta_create_schedule_item_json("{bad_json"), nullptr);
    EXPECT_EQ(canta_add_calendar_event_json("{bad_json"), static_cast<int32_t>(UNKNOWN));
    EXPECT_EQ(canta_update_ai_model_json("{bad_json"), static_cast<int32_t>(UNKNOWN));

    // Finally log out
    EXPECT_EQ(canta_logout_user(), static_cast<int32_t>(RST_OK));
}

TEST_F(CApiTest, ExhaustiveBranchCoverage) {
    // 1. Storage path parameter in init
    EXPECT_EQ(canta_init_engine("test_data_dir", "{}"), static_cast<int32_t>(RST_OK));

    // 2. Register candidate user
    const char* reg_json = canta_register_user_json("full_branch_user", "password123", "Full Branch");
    ASSERT_NE(reg_json, nullptr);
    canta_free_string(reg_json);

    // Duplicate registration returns nullptr
    EXPECT_EQ(canta_register_user_json("full_branch_user", "password123", "Full Branch"), nullptr);

    // Login with wrong password returns nullptr
    EXPECT_EQ(canta_login_user_json("full_branch_user", "wrong_pass"), nullptr);

    // Login with nonexistent user returns nullptr
    EXPECT_EQ(canta_login_user_json("nonexistent_user_xyz", "pass"), nullptr);

    // Valid login
    const char* login_json = canta_login_user_json("full_branch_user", "password123");
    ASSERT_NE(login_json, nullptr);
    canta_free_string(login_json);

    // 3. Create topic variations
    // Topic with empty title returns nullptr
    EXPECT_EQ(canta_create_topic_json(R"({"title":""})"), nullptr);

    // Initial topic creates category
    const char* t0_json = canta_create_topic_json(R"({"title":"Topic Initial","categoryName":"Derecho Civil"})");
    ASSERT_NE(t0_json, nullptr);
    auto j_t0 = nlohmann::json::parse(t0_json);
    std::string cat_id_str = j_t0["categoryId"];
    canta_free_string(t0_json);

    // Topic with integer categoryId
    std::string t1_payload = "{\"title\":\"Topic Int Cat\",\"categoryId\":" + cat_id_str + ",\"tags\":[\"t1\"]}";
    const char* t1_json = canta_create_topic_json(t1_payload.c_str());
    ASSERT_NE(t1_json, nullptr);
    auto j_t1 = nlohmann::json::parse(t1_json);
    std::string t1_id = j_t1["id"];
    canta_free_string(t1_json);

    // Topic with string categoryId
    std::string t2_payload = "{\"title\":\"Topic Str Cat\",\"categoryId\":\"" + cat_id_str + "\"}";
    const char* t2_json = canta_create_topic_json(t2_payload.c_str());
    ASSERT_NE(t2_json, nullptr);
    auto j_t2 = nlohmann::json::parse(t2_json);
    std::string t2_id = j_t2["id"];
    canta_free_string(t2_json);

    // Topic reusing existing category name
    const char* t3_json = canta_create_topic_json(R"({"title":"Topic Reused Cat","categoryName":"Derecho Civil"})");
    ASSERT_NE(t3_json, nullptr);
    canta_free_string(t3_json);

    // 4. Update topic variations
    // Update without id
    EXPECT_EQ(canta_update_topic_json(R"({"title":"No ID"})"), static_cast<int32_t>(DB_BAD_PARAM));

    // Update with nonexistent id
    EXPECT_EQ(canta_update_topic_json(R"({"id":99999,"title":"Nonexistent"})"), static_cast<int32_t>(SUBJECT_NOT_FOUND));

    // Update with integer id and string categoryId
    std::string upd1 = "{\"id\":" + t1_id + ",\"title\":\"Updated Int ID\",\"categoryId\":\"" + cat_id_str + "\",\"language\":\"en\"}";
    EXPECT_EQ(canta_update_topic_json(upd1.c_str()), static_cast<int32_t>(RST_OK));

    // Update with string id and integer categoryId
    std::string upd2 = "{\"id\":\"" + t2_id + "\",\"title\":\"Updated Str ID\",\"categoryId\":" + cat_id_str + "}";
    EXPECT_EQ(canta_update_topic_json(upd2.c_str()), static_cast<int32_t>(RST_OK));

    // 5. Query topic by non-numeric and nonexistent IDs
    EXPECT_EQ(canta_get_topic_by_id_json("not_a_number"), nullptr);
    EXPECT_EQ(canta_get_topic_by_id_json("99999"), nullptr);

    // Query topics by non-numeric category ID
    const char* bad_cat = canta_get_topics_by_category_json("not_a_number");
    ASSERT_NE(bad_cat, nullptr);
    EXPECT_STREQ(bad_cat, "[]");
    canta_free_string(bad_cat);

    // Delete topic with non-numeric ID
    EXPECT_EQ(canta_delete_topic("not_a_number"), static_cast<int32_t>(UNKNOWN));

    // 6. Calendar & Sessions edge cases
    // Calendar event with string topicId
    std::string cal_str = "{\"topicId\":\"" + t1_id + "\",\"date\":1770000000}";
    EXPECT_EQ(canta_add_calendar_event_json(cal_str.c_str()), static_cast<int32_t>(RST_OK));

    // Calendar event with integer topicId
    std::string cal_int = "{\"topicId\":" + t2_id + ",\"date\":1770001000}";
    EXPECT_EQ(canta_add_calendar_event_json(cal_int.c_str()), static_cast<int32_t>(RST_OK));

    // Non-numeric event ID removal
    EXPECT_EQ(canta_remove_calendar_event("not_a_number"), static_cast<int32_t>(UNKNOWN));

    // Sessions for non-numeric topic ID
    const char* non_num_sess = canta_get_sessions_for_topic_json("not_a_number");
    ASSERT_NE(non_num_sess, nullptr);
    EXPECT_STREQ(non_num_sess, "[]");
    canta_free_string(non_num_sess);

    // Session by non-numeric ID
    EXPECT_EQ(canta_get_session_by_id_json("not_a_number"), nullptr);
    EXPECT_EQ(canta_get_session_by_id_json("99999"), nullptr);

    // Delete session non-numeric and nonexistent
    EXPECT_EQ(canta_delete_session("not_a_number"), static_cast<int32_t>(UNKNOWN));
    EXPECT_EQ(canta_delete_session("99999"), static_cast<int32_t>(PRACTICE_EVENT_NOT_FOUND));

    // 7. Schedule items edge cases
    // Schedule item without practiceId
    EXPECT_EQ(canta_create_schedule_item_json(R"({})"), nullptr);
    EXPECT_EQ(canta_create_schedule_item_json(R"({"practiceId":0})"), nullptr);
    EXPECT_EQ(canta_create_schedule_item_json(R"({"practiceId":99999})"), nullptr);

    // Nonexistent schedule item update & delete
    EXPECT_EQ(canta_update_schedule_item_status("nonexistent_task_xyz", 3), static_cast<int32_t>(TASK_NOT_FOUND));
    EXPECT_EQ(canta_delete_schedule_item("nonexistent_task_xyz"), static_cast<int32_t>(TASK_NOT_FOUND));

    // 8. Analysis generation edge cases
    EXPECT_EQ(canta_generate_session_analysis_json("not_a_number"), nullptr);
    EXPECT_EQ(canta_generate_session_analysis_json("99999"), nullptr);
    EXPECT_EQ(canta_get_session_analysis_report_json("not_a_number"), nullptr);
    EXPECT_EQ(canta_get_session_analysis_report_json("99999"), nullptr);

    // 9. AI models updates
    // Missing/empty name returns DB_BAD_PARAM
    EXPECT_EQ(canta_update_ai_model_json(R"({"name":""})"), static_cast<int32_t>(DB_BAD_PARAM));
    // Llama model type
    EXPECT_EQ(canta_update_ai_model_json(R"({"name":"llama-embed.gguf","type":"llama","isActive":true})"), static_cast<int32_t>(RST_OK));

    // 10. Start playback with null or invalid session
    EXPECT_EQ(canta_start_playback(nullptr), static_cast<int32_t>(DB_BAD_PARAM));
    EXPECT_EQ(canta_start_playback("not_a_number"), static_cast<int32_t>(UNKNOWN));

    // Playback on planned event with empty filepath returns FILE_NOT_FOUND
    const char* cal_for_pb = canta_get_calendar_events_json();
    if (cal_for_pb) {
        auto j_cal_pb = nlohmann::json::parse(cal_for_pb);
        if (!j_cal_pb.empty()) {
            std::string pb_event_id = j_cal_pb[0]["id"];
            EXPECT_EQ(canta_start_playback(pb_event_id.c_str()), static_cast<int32_t>(FILE_NOT_FOUND));
            // Also test analysis report query when no execution_id exists
            EXPECT_EQ(canta_get_session_analysis_report_json(pb_event_id.c_str()), nullptr);
            EXPECT_EQ(canta_generate_session_analysis_json(pb_event_id.c_str()), nullptr);
            // Also test audio read range when empty filepath
            uint8_t dummy_buf[16];
            uint32_t br = 0;
            int32_t is_eof = 0;
            unsigned int p_id = static_cast<unsigned int>(std::stoul(pb_event_id));
            EXPECT_EQ(canta_read_audio_stream(p_id, 0, 16, dummy_buf, &br, &is_eof), static_cast<int32_t>(FILE_NOT_FOUND));
        }
        canta_free_string(cal_for_pb);
    }

    // Schedule items filtering with status and query mismatches
    const char* no_match_status = canta_get_schedule_items_json(999, nullptr);
    if (no_match_status) canta_free_string(no_match_status);
    const char* no_match_query = canta_get_schedule_items_json(-1, "nonexistent_task_query_xyz");
    if (no_match_query) canta_free_string(no_match_query);

    // 11. Purge database when engine is initialized
    canta_purge_database_for_test();

    // 12. Unauthenticated calls on remaining endpoints
    canta_shutdown_engine();
    EXPECT_EQ(canta_init_engine(nullptr, nullptr), static_cast<int32_t>(RST_OK));
    EXPECT_EQ(canta_delete_session("1"), static_cast<int32_t>(USER_NO_AUTH));
    EXPECT_EQ(canta_update_schedule_item_status("1", 3), static_cast<int32_t>(USER_NO_AUTH));
    EXPECT_EQ(canta_delete_schedule_item("1"), static_cast<int32_t>(USER_NO_AUTH));
    EXPECT_EQ(canta_remove_calendar_event("1"), static_cast<int32_t>(USER_NO_AUTH));
    EXPECT_EQ(canta_start_playback("1"), static_cast<int32_t>(USER_NO_AUTH));
    uint8_t dummy_buf2[16];
    uint32_t br2 = 0;
    int32_t eof2 = 0;
    EXPECT_EQ(canta_read_audio_stream(1, 0, 16, dummy_buf2, &br2, &eof2), static_cast<int32_t>(USER_NO_AUTH));

    const char* noauth_cats = canta_get_categories_json();
    ASSERT_NE(noauth_cats, nullptr);
    EXPECT_STREQ(noauth_cats, "[]");
    canta_free_string(noauth_cats);

    const char* noauth_tops = canta_get_all_topics_json();
    ASSERT_NE(noauth_tops, nullptr);
    EXPECT_STREQ(noauth_tops, "[]");
    canta_free_string(noauth_tops);

    const char* noauth_cat_tops = canta_get_topics_by_category_json("1");
    ASSERT_NE(noauth_cat_tops, nullptr);
    EXPECT_STREQ(noauth_cat_tops, "[]");
    canta_free_string(noauth_cat_tops);

    EXPECT_EQ(canta_get_topic_by_id_json("1"), nullptr);
    EXPECT_EQ(canta_create_topic_json("{}"), nullptr);
    EXPECT_EQ(canta_update_topic_json("{}"), static_cast<int32_t>(USER_NO_AUTH));
    EXPECT_EQ(canta_delete_topic("1"), static_cast<int32_t>(USER_NO_AUTH));
    EXPECT_EQ(canta_start_recording_session("1"), static_cast<int32_t>(USER_NO_AUTH));
    EXPECT_EQ(canta_stop_recording_session(), nullptr);
    EXPECT_EQ(canta_create_study_session_json("1", nullptr, 60), nullptr);

    const char* noauth_sess = canta_get_sessions_for_topic_json("1");
    ASSERT_NE(noauth_sess, nullptr);
    EXPECT_STREQ(noauth_sess, "[]");
    canta_free_string(noauth_sess);

    const char* noauth_recent = canta_get_recent_sessions_json();
    ASSERT_NE(noauth_recent, nullptr);
    EXPECT_STREQ(noauth_recent, "[]");
    canta_free_string(noauth_recent);

    EXPECT_EQ(canta_get_session_by_id_json("1"), nullptr);

    const char* noauth_sched = canta_get_schedule_items_json(-1, nullptr);
    ASSERT_NE(noauth_sched, nullptr);
    EXPECT_STREQ(noauth_sched, "[]");
    canta_free_string(noauth_sched);

    EXPECT_EQ(canta_create_schedule_item_json("{}"), nullptr);

    const char* noauth_cal = canta_get_calendar_events_json();
    ASSERT_NE(noauth_cal, nullptr);
    EXPECT_STREQ(noauth_cal, "[]");
    canta_free_string(noauth_cal);

    EXPECT_EQ(canta_add_calendar_event_json("{}"), static_cast<int32_t>(USER_NO_AUTH));

    const char* noauth_models = canta_get_ai_models_json();
    ASSERT_NE(noauth_models, nullptr);
    EXPECT_STRNE(noauth_models, "[]");
    canta_free_string(noauth_models);

    EXPECT_EQ(canta_update_ai_model_json("{}"), static_cast<int32_t>(USER_NO_AUTH));

    // Query topics for non-existent numeric category ID
    const char* non_exist_cat_topics = canta_get_topics_by_category_json("99999");
    ASSERT_NE(non_exist_cat_topics, nullptr);
    EXPECT_STREQ(non_exist_cat_topics, "[]");
    canta_free_string(non_exist_cat_topics);

    // Register user with single word display name to cover no-space branch
    const char* u_single = canta_register_user_json("candidate_single", "pass123", "SingleName");
    ASSERT_NE(u_single, nullptr);
    canta_free_string(u_single);

    // Create study session with explicit existing audio path
    std::string sample_opus_path;
    std::vector<std::filesystem::path> candidates = {
        "CantaTema/example_data/subject_es_1_p_1.opus",
        "../CantaTema/example_data/subject_es_1_p_1.opus",
        "../../CantaTema/example_data/subject_es_1_p_1.opus",
        "../../../CantaTema/example_data/subject_es_1_p_1.opus"
    };
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) {
            sample_opus_path = c.string();
            break;
        }
    }
    if (!sample_opus_path.empty()) {
        const char* study_with_path = canta_create_study_session_json("1", sample_opus_path.c_str(), 60);
        if (study_with_path) canta_free_string(study_with_path);
    }

    // Update schedule item with non-cancel status code (returns RST_OK)
    EXPECT_EQ(canta_update_schedule_item_status("any", 1), static_cast<int32_t>(RST_OK));

    // Cover real recording failure path when mock is disabled
    canta_set_mock_recording_for_test(0);
    canta_start_recording_session("1");
    EXPECT_EQ(canta_stop_recording_session(), nullptr);

    // Cover stop recording session
    canta_set_mock_recording_for_test(1);
    canta_start_recording_session("1");
    canta_stop_recording_session();
    canta_set_mock_recording_for_test(0);

    // 13. Purge database when engine is shut down
    canta_shutdown_engine();
    canta_purge_database_for_test();
}

int main(int argc, char **argv) {
    util_logger_init_for_test();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


