/**
 * @file cantatema_c_api.h
 * @brief C ABI export interface for CantaTema native core engine integration with Flutter (dart:ffi).
 */

#ifndef CANTATEMA_C_API_H
#define CANTATEMA_C_API_H

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CANTATEMA_EXPORTS)
        #define CANTATEMA_API __declspec(dllexport)
    #else
        #define CANTATEMA_API __declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CANTATEMA_API __attribute__((visibility("default")))
    #else
        #define CANTATEMA_API
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------------------
// 1. Engine Lifecycle & Session Management (Single Active Session)
//-----------------------------------------------------------------------------------------

/**
 * @brief Initializes SQLite persistence, directory paths, and boots the native engine.
 * @note Limits to maximum 1 active session when running embedded in Flutter.
 * @param storage_path Absolute root path where databases and data are stored (or NULL for default).
 * @param config_json Optional configuration parameters in JSON format.
 * @return int32_t Returns rst_code_e (RST_OK on success, negative or non-zero on failure).
 */
CANTATEMA_API int32_t canta_init_engine(const char* storage_path, const char* config_json);

/**
 * @brief Initializes logger in test mode (critical errors only, no console spew).
 */
CANTATEMA_API void canta_init_logger_for_test(void);

/**
 * @brief Enables or disables simulated audio recording for headless/test environments without capture hardware.
 * @param enabled 1 to enable mock recording, 0 to disable.
 */
CANTATEMA_API void canta_set_mock_recording_for_test(int enabled);

/**
 * @brief Resets and purges database state for testing harness.
 */
CANTATEMA_API void canta_purge_database_for_test(void);

/**
 * @brief Flushes transactions, releases audio/models, and shuts down the active engine session.
 */
CANTATEMA_API void canta_shutdown_engine(void);

/**
 * @brief Deallocates heap strings returned across the C ABI by the engine.
 * @param ptr Pointer previously returned by C++ methods (malloc-allocated UTF-8 string).
 */
CANTATEMA_API void canta_free_string(const char* ptr);

/**
 * @brief Returns the semver version string of the compiled CantaTema core engine.
 * @return const char* Heap-allocated UTF-8 string. Must be freed with canta_free_string.
 */
CANTATEMA_API const char* canta_get_engine_version(void);

/**
 * @brief Translates an rst_code_e integer code into a human-readable string.
 * @param error_code Integer code corresponding to rst_code_e.
 * @return const char* Heap-allocated string description (e.g. "USER_NO_AUTH"). Must be freed.
 */
CANTATEMA_API const char* canta_get_error_text(int32_t error_code);

//-----------------------------------------------------------------------------------------
// 2. User Authentication & Profile
//-----------------------------------------------------------------------------------------

/**
 * @brief Retrieves the active candidate user profile for the current session.
 * @return const char* JSON object string of UserProfile, or NULL if unauthenticated. Must be freed.
 */
CANTATEMA_API const char* canta_get_current_user_json(void);

/**
 * @brief Authenticates credentials and sets the active candidate session user.
 * @param email_or_name Candidate username or email address.
 * @param password Plaintext or hashed password.
 * @return const char* JSON string of authenticated UserProfile, or NULL on failure. Must be freed.
 */
CANTATEMA_API const char* canta_login_user_json(const char* email_or_name, const char* password);

/**
 * @brief Registers a new candidate profile and logs in the active session.
 * @param email_or_name Candidate username or email.
 * @param password Password.
 * @param display_name Full name or display name.
 * @return const char* Created UserProfile JSON string, or NULL on error. Must be freed.
 */
CANTATEMA_API const char* canta_register_user_json(const char* email_or_name, const char* password, const char* display_name);

/**
 * @brief Logs out the active candidate session user.
 * @return int32_t RST_OK on success.
 */
CANTATEMA_API int32_t canta_logout_user(void);

//-----------------------------------------------------------------------------------------
// 3. Topic & Category Management
//-----------------------------------------------------------------------------------------

/**
 * @brief Queries study categories for the active user with topic counts and mastery.
 * @return const char* JSON array string of TopicCategory objects. Must be freed.
 */
CANTATEMA_API const char* canta_get_categories_json(void);

/**
 * @brief Retrieves all study topics / subjects for the active user.
 * @return const char* JSON array string of Topic objects. Must be freed.
 */
CANTATEMA_API const char* canta_get_all_topics_json(void);

/**
 * @brief Retrieves topics belonging to a specific category.
 * @param category_id Unique category identifier string or integer representation.
 * @return const char* JSON array string of Topic objects. Must be freed.
 */
CANTATEMA_API const char* canta_get_topics_by_category_json(const char* category_id);

/**
 * @brief Fetches a single study topic by ID including reference document path.
 * @param topic_id Unique topic identifier.
 * @return const char* Topic JSON object string, or NULL if not found. Must be freed.
 */
CANTATEMA_API const char* canta_get_topic_by_id_json(const char* topic_id);

/**
 * @brief Creates a new study topic / subject under a category.
 * @param topic_payload_json JSON object containing title, categoryName/categoryId, filePath, tags, language.
 * @return const char* Created Topic JSON object string. Must be freed.
 */
CANTATEMA_API const char* canta_create_topic_json(const char* topic_payload_json);

/**
 * @brief Updates topic metadata, syllabus path, or tags.
 * @param topic_payload_json JSON object containing topic id and updated fields.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_update_topic_json(const char* topic_payload_json);

/**
 * @brief Deletes a study topic by ID.
 * @param topic_id Unique topic identifier.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_delete_topic(const char* topic_id);

//-----------------------------------------------------------------------------------------
// 4. Audio Capture, DSP Metering & Recording Studio
//-----------------------------------------------------------------------------------------

/**
 * @brief Arms microphone capture for reciting a topic and creates a live session.
 * @param topic_id Unique identifier of the recited topic.
 * @return int32_t rst_code_e (RST_OK on success, PRACTICE_EVENT_ERROR if busy).
 */
CANTATEMA_API int32_t canta_start_recording_session(const char* topic_id);

/**
 * @brief Pauses audio recording buffer capture.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_pause_recording_session(void);

/**
 * @brief Resumes paused audio capture.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_resume_recording_session(void);

/**
 * @brief Stops capture, encrypts/flushes Opus audio, and persists the recorded session.
 * @return const char* JSON string of finalized StudySession. Must be freed.
 */
CANTATEMA_API const char* canta_stop_recording_session(void);

/**
 * @brief Retrieves current audio RMS amplitude level for live waveform rendering.
 * @return float Amplitude normalized in range [0.0, 1.0] (0.0 if idle or paused).
 */
CANTATEMA_API float canta_get_current_audio_amplitude(void);

/**
 * @brief Polls newly transcribed speech text tokens since last query.
 * @return const char* UTF-8 text string of new tokens, or NULL if none. Must be freed.
 */
CANTATEMA_API const char* canta_poll_live_transcription(void);

//-----------------------------------------------------------------------------------------
// 5. Cante Session History
//-----------------------------------------------------------------------------------------

/**
 * @brief Creates or imports a study session record associated with a topic.
 * @param topic_id Unique topic identifier.
 * @param audio_path Path to audio file. If NULL or empty, a placeholder audio record is generated.
 * @param duration_seconds Duration of recording in seconds (defaults to 60 if 0).
 * @return const char* StudySession JSON object string on success, or NULL on error. Must be freed.
 */
CANTATEMA_API const char* canta_create_study_session_json(const char* topic_id, const char* audio_path, uint32_t duration_seconds);

/**
 * @brief Queries recorded recitation history for a topic, sorted by date descending.
 * @param topic_id Unique topic identifier.
 * @return const char* JSON array string of StudySession objects. Must be freed.
 */
CANTATEMA_API const char* canta_get_sessions_for_topic_json(const char* topic_id);

/**
 * @brief Queries global chronological feed of recorded sessions for active user.
 * @return const char* JSON array string of StudySession objects. Must be freed.
 */
CANTATEMA_API const char* canta_get_recent_sessions_json(void);

/**
 * @brief Retrieves metadata for a specific practice session by ID.
 * @param session_id Session / practice event identifier.
 * @return const char* StudySession JSON object string, or NULL if not found. Must be freed.
 */
CANTATEMA_API const char* canta_get_session_by_id_json(const char* session_id);

/**
 * @brief Deletes a practice session record and its recorded audio file.
 * @param session_id Session identifier to remove.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_delete_session(const char* session_id);

//-----------------------------------------------------------------------------------------
// 6. Speech Analysis & Syllabus Evaluation
//-----------------------------------------------------------------------------------------

/**
 * @brief Runs coverage pipeline (STT transcription, embeddings, similarity matching, scoring).
 * @param session_id Session / practice event identifier to analyze.
 * @return const char* JSON string of comprehensive AnalysisReport. Must be freed.
 */
CANTATEMA_API const char* canta_generate_session_analysis_json(const char* session_id);

/**
 * @brief Retrieves previously computed analysis report for a session from cache/DB.
 * @param session_id Session identifier.
 * @return const char* AnalysisReport JSON string, or NULL if unanalyzed. Must be freed.
 */
CANTATEMA_API const char* canta_get_session_analysis_report_json(const char* session_id);

//-----------------------------------------------------------------------------------------
// 7. Task Scheduler & Agenda Calendar
//-----------------------------------------------------------------------------------------

/**
 * @brief Retrieves study schedule items / tasks with optional status and query filter.
 * @param status_filter Filter code (-1 = all, 0 = waiting, 1 = executing, 2 = done, 3 = cancelled).
 * @param search_query Optional search filter text (or NULL).
 * @return const char* JSON array string of ScheduleItem objects. Must be freed.
 */
CANTATEMA_API const char* canta_get_schedule_items_json(int32_t status_filter, const char* search_query);

/**
 * @brief Adds a planned practice session to the task scheduler.
 * @param schedule_payload_json JSON object {"topicName":"...","topicId":...,"date":"..."}.
 * @return const char* Created ScheduleItem JSON object string. Must be freed.
 */
CANTATEMA_API const char* canta_create_schedule_item_json(const char* schedule_payload_json);

/**
 * @brief Updates status of a scheduled item.
 * @param item_id Item identifier string.
 * @param new_status Target status integer code.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_update_schedule_item_status(const char* item_id, int32_t new_status);

/**
 * @brief Deletes a scheduled task or planned practice.
 * @param item_id Item identifier string.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_delete_schedule_item(const char* item_id);

/**
 * @brief Retrieves calendar events (past recorded cantes and future planned sessions).
 * @return const char* JSON array string of CanteEvent objects. Must be freed.
 */
CANTATEMA_API const char* canta_get_calendar_events_json(void);

/**
 * @brief Schedules a future cante date on the study calendar.
 * @param event_payload_json Serialized CanteEvent JSON string.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_add_calendar_event_json(const char* event_payload_json);

/**
 * @brief Cancels an agenda event by ID.
 * @param event_id Calendar event identifier.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_remove_calendar_event(const char* event_id);

//-----------------------------------------------------------------------------------------
// 8. Local AI Model Management
//-----------------------------------------------------------------------------------------

/**
 * @brief Lists local and download-ready Whisper STT and Llama embedding models.
 * @return const char* JSON array string of AiModelItem objects. Must be freed.
 */
CANTATEMA_API const char* canta_get_ai_models_json(void);

/**
 * @brief Updates default active AI model configuration.
 * @param model_payload_json Serialized AiModelItem JSON object.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_update_ai_model_json(const char* model_payload_json);

//-----------------------------------------------------------------------------------------
// 9. Audio Playback & Streaming Range
//-----------------------------------------------------------------------------------------

/**
 * @brief Starts playback of a recorded session.
 * @param session_id Session / practice event identifier.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_start_playback(const char* session_id);

/**
 * @brief Stops active audio playback stream.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_stop_playback(void);

/**
 * @brief Streams decrypted audio byte range for a practice event with zero temp files.
 * @param session_id Practice event ID.
 * @param offset Byte offset in the file.
 * @param length Number of bytes to read.
 * @param out_buffer Pre-allocated output buffer receiving decrypted audio bytes.
 * @param out_bytes_read Number of bytes successfully read into out_buffer.
 * @param out_is_eof Output flag set to 1 if EOF reached, 0 otherwise.
 * @return int32_t rst_code_e (RST_OK on success).
 */
CANTATEMA_API int32_t canta_read_audio_stream(
    uint32_t session_id,
    uint64_t offset,
    uint32_t length,
    uint8_t* out_buffer,
    uint32_t* out_bytes_read,
    int32_t* out_is_eof
);

#ifdef __cplusplus
}
#endif

#endif // CANTATEMA_C_API_H
