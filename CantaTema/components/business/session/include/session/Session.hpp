/**
 * @file Session.hpp
 * @brief Facade class representing the active user session and main business logic coordinator.
 */

#ifndef __SESSION_HPP
#define __SESSION_HPP

#include <memory>
#include <string>
#include <vector>

#include "operations/i_operation_user.hpp"
#include "operations/i_operation_category.hpp"
#include "operations/i_operation_subject.hpp"
#include "operations/i_operation_user_metrics.hpp"
#include "operations/i_operation_practice_event.hpp"
#include "operations/i_operation_coverage.hpp"
#include "models/manager_models.hpp"
#include "sound_system/i_sound_system.hpp"
#include "database/i_database.hpp"
#include "primitives/user_configuration.hpp"
#include "primitives/hardware_info.hpp"

namespace cantatema::infra {
class IGpuDetector;
}

/**
 * @class Session
 * @brief Main facade class coordinating operations, models, sound, and coverage pipelines for the logged-in user.
 */
class Session : public IOperationUser
{
public:
    /**
     * @brief Constructs a Session instance with injected operations and infrastructure dependencies.
     * @param _user_op Injected user operations handler.
     * @param _category_op Injected category operations handler.
     * @param _subject_op Injected subject operations handler.
     * @param _user_metrics_op Injected user metrics operations handler.
     * @param _practice_event_op Injected practice event operations handler.
     * @param _coverage_op Injected coverage operations handler (optional).
     * @param _db_op Injected database handler for coverage persistence (optional).
     * @param _sound_system_op Injected sound system handler (optional).
     * @param _models_manager_op Injected model manager handler (optional).
     * @param _gpu_detector_op Injected hardware/GPU detector handler (optional).
     */
    Session(
        std::shared_ptr<IOperationUser> &&_user_op,
        std::shared_ptr<IOperationCategory> &&_category_op,
        std::shared_ptr<IOperationSubject> &&_subject_op,
        std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op,
        std::shared_ptr<IOperationPracticeEvent> &&_practice_event_op,
        std::shared_ptr<IOperationCoverage> &&_coverage_op = nullptr,
        std::shared_ptr<IDatabase> &&_db_op = nullptr,
        std::shared_ptr<ISoundSystem> &&_sound_system_op = nullptr,
        std::shared_ptr<ManagerModels> &&_models_manager_op = nullptr,
        std::shared_ptr<cantatema::infra::IGpuDetector> &&_gpu_detector_op = nullptr
    );

    /**
     * @brief Default constructor for Session, initializing default concrete operation and infrastructure instances.
     */
    Session(void);

    /**
     * @brief Destructor for Session.
     */
    ~Session(void) override;

    /**
     * @brief Initializes or resets session state.
     * @return rst_code_e RST_OK on success.
     */
    rst_code_e initialize(void);

    //-------------------------------------------------------------------------------------
    // User Management & Authentication
    //-------------------------------------------------------------------------------------

    /**
     * @brief Registers a new user in the system.
     * @param name Username credential.
     * @param password Password credential.
     * @return rst_code_e RST_OK on success, USER_DUPLICATED or error code.
     */
    rst_code_e user_add(const std::string &name, const std::string &password) override;

    /**
     * @brief Retrieves the currently authenticated session user profile.
     * @param user Output reference to receive const User pointer.
     * @return rst_code_e RST_OK on success, or USER_NO_AUTH if unauthenticated.
     */
    rst_code_e user_get(std::shared_ptr<const User> &user) override;

    /**
     * @brief Retrieves a user profile by username.
     * @param user_name Username to query.
     * @param user Output reference to receive const User pointer.
     * @return rst_code_e RST_OK on success, USER_NO_AUTH or USER_NOT_FOUND.
     */
    rst_code_e user_get_by_name(std::string user_name, std::shared_ptr<const User> &user) override;

    /**
     * @brief Updates the currently authenticated user's account details.
     * @param user Updated User object.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e user_update(std::shared_ptr<const User> &user) override;

    /**
     * @brief Removes the currently authenticated user account and logs out.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e user_remove(void) override;

    /**
     * @brief Checks if a user is currently authenticated in this session.
     * @return true if authenticated, false otherwise.
     */
    bool user_is_authenticated(void) override;

    /**
     * @brief Authenticates user credentials and loads session context.
     * @param name Username credential.
     * @param password Password credential.
     * @return rst_code_e RST_OK on success, USER_NO_AUTH on bad password, or USER_NOT_FOUND.
     */
    rst_code_e user_identify(const std::string &name, const std::string &password) override;

    /**
     * @brief Logs out the currently authenticated session user.
     * @return rst_code_e RST_OK on success.
     */
    rst_code_e user_logout(void);

    //-------------------------------------------------------------------------------------
    // Category Management
    //-------------------------------------------------------------------------------------

    /**
     * @brief Creates a new study category for the authenticated user.
     * @param name Category name.
     * @return rst_code_e RST_OK on success, USER_NO_AUTH or error code.
     */
    rst_code_e category_add(const std::string &name);

    /**
     * @brief Renames an existing category belonging to the authenticated user.
     * @param category_id Unique ID of category.
     * @param new_name New category name.
     * @return rst_code_e RST_OK on success, CATEGORY_NOT_FOUND, or error code.
     */
    rst_code_e category_update(const unsigned int category_id, const std::string &new_name);

    /**
     * @brief Removes a study category by ID.
     * @param category_id Unique ID of category to delete.
     * @return rst_code_e RST_OK on success, CATEGORY_NOT_FOUND, or error code.
     */
    rst_code_e category_remove(const unsigned int category_id);

    /**
     * @brief Retrieves a category by ID if it belongs to the authenticated user.
     * @param category_id Unique ID of category to query.
     * @param category Output reference receiving Category pointer.
     * @return rst_code_e RST_OK on success, CATEGORY_NOT_FOUND, or USER_NO_AUTH.
     */
    rst_code_e category_get_by_id(const unsigned int category_id, std::shared_ptr<Category> &category);

    /**
     * @brief Retrieves all study categories owned by the authenticated user.
     * @param categories Output vector receiving user categories.
     * @return rst_code_e RST_OK on success, or USER_NO_AUTH.
     */
    rst_code_e category_get_by_user(std::vector<std::shared_ptr<const Category>> &categories);

    //-------------------------------------------------------------------------------------
    // Subject Management
    //-------------------------------------------------------------------------------------

    /**
     * @brief Adds a new study subject with reference material attachment.
     * @param name Subject title.
     * @param category_id Associated category ID (0 for uncategorized).
     * @param file_path Filepath to reference PDF/text document.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_add(const std::string &name, unsigned int category_id, const std::string &file_path);

    /**
     * @brief Updates subject attributes and attached reference document.
     * @param id Subject ID.
     * @param new_name New subject title.
     * @param new_category_id New category ID.
     * @param file_path_new New reference document filepath (empty to keep unchanged).
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_update(unsigned int id, const std::string &new_name, const unsigned int new_category_id, const std::string &file_path_new);

    /**
     * @brief Deletes a subject record by ID.
     * @param id Subject ID.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e subject_remove(unsigned int id);

    /**
     * @brief Retrieves subject details by subject ID.
     * @param subject_id Subject ID.
     * @param subject Output reference receiving Subject pointer.
     * @return rst_code_e RST_OK on success, SUBJECT_NOT_FOUND, or USER_NO_AUTH.
     */
    rst_code_e subject_get_by_id(unsigned int subject_id, std::shared_ptr<Subject> &subject);

    /**
     * @brief Retrieves all subjects belonging to a specific category.
     * @param category_id Category ID.
     * @param subjects Output vector receiving matching Subject objects.
     * @return rst_code_e RST_OK on success, CATEGORY_NOT_FOUND, or error code.
     */
    rst_code_e subject_get_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects);

    /**
     * @brief Retrieves all subjects owned by the authenticated user.
     * @param subjects Output vector receiving all user Subject objects.
     * @return rst_code_e RST_OK on success, or USER_NO_AUTH.
     */
    rst_code_e subject_get_by_user(std::vector<std::shared_ptr<Subject>> &subjects);

    /**
     * @brief Sets language code (e.g. 'es', 'en') for a subject.
     * @param subject_id Subject ID.
     * @param language ISO language code.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e set_subject_language(int subject_id, const std::string &language);

    /**
     * @brief Gets configured language code for a subject.
     * @param subject_id Subject ID.
     * @param language Output string receiving language ISO code.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e get_subject_language(int subject_id, std::string &language);

    //-------------------------------------------------------------------------------------
    // User Metrics
    //-------------------------------------------------------------------------------------

    /**
     * @brief Retrieves current storage metrics and practice counts for authenticated user.
     * @param user_metrics Output reference receiving UserMetrics object.
     * @return rst_code_e RST_OK on success, or USER_NO_AUTH.
     */
    rst_code_e user_metrics_get(std::shared_ptr<const UserMetrics> &user_metrics);

    /**
     * @brief Validates if the authenticated user's storage quota can accept an incoming file.
     * @param size_in_kb Size of incoming file in Kilobytes.
     * @return rst_code_e RST_OK if acceptable, or USER_STORAGE_LIMIT_EXCEEDED.
     */
    rst_code_e user_metrics_can_accept_file_size(unsigned int size_in_kb);

    //-------------------------------------------------------------------------------------
    // Practice Event Operations
    //-------------------------------------------------------------------------------------

    /**
     * @brief Schedules a planned practice session for a subject.
     * @param practice PracticeEvent reference with subject and date configured.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e practice_event_add_planned(PracticeEvent &practice);

    /**
     * @brief Adds a completed audio-recorded practice event.
     * @param source_file Path to recorded audio file.
     * @param practice PracticeEvent reference.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e practice_event_add_recorded(const std::string &source_file, PracticeEvent &practice);

    /**
     * @brief Updates practice event metadata or completion status.
     * @param practice PracticeEvent containing updated parameters.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e practice_event_update(const PracticeEvent &practice);

    /**
     * @brief Deletes a practice event by ID.
     * @param id Practice event ID.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e practice_event_remove(unsigned int id);

    /**
     * @brief Removes all practice events associated with a subject.
     * @param subject_id Unique subject ID.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e practice_event_remove_by_subject(unsigned int subject_id);

    /**
     * @brief Removes all practice events for the authenticated user.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e practice_event_remove_all_by_user(void);

    /**
     * @brief Retrieves a practice event by ID.
     * @param id Practice event ID.
     * @param practice Output reference receiving PracticeEvent object.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e practice_event_get_by_id(unsigned int id, std::shared_ptr<PracticeEvent> &practice);

    /**
     * @brief Retrieves all practice events associated with a subject.
     * @param subject_id Subject ID.
     * @param practices Output vector receiving practice events.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e practice_event_get_by_subject(unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &practices);

    /**
     * @brief Retrieves all practice events for the authenticated user.
     * @param practices Output vector receiving practice events.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e practice_event_get_by_user(std::vector<std::shared_ptr<PracticeEvent>> &practices);

    //-------------------------------------------------------------------------------------
    // Audio Hardware, Recording & Playback (ISoundSystem)
    //-------------------------------------------------------------------------------------

    /**
     * @brief Enumerates available audio capture input devices.
     * @param devices Output vector receiving list of audio devices.
     * @return rst_code_e RST_OK on success.
     */
    rst_code_e audio_get_capture_devices(std::vector<ISoundSystem::SoundSystemDeviceInfo> &devices);

    /**
     * @brief Starts audio recording to the specified file.
     * @param output_file_path Target audio destination path.
     * @param device_index Audio capture device index (-1 for default).
     * @return rst_code_e RST_OK on success, or AUDIO_ERROR.
     */
    rst_code_e audio_start_recording(const std::string &output_file_path, int device_index = -1);

    /**
     * @brief Stops the active audio recording session and flushes output.
     * @return rst_code_e RST_OK on success.
     */
    rst_code_e audio_stop_recording(void);

    /**
     * @brief Checks if audio recording is currently in progress.
     * @return true if actively recording, false otherwise.
     */
    bool audio_is_recording(void) const;

    /**
     * @brief Gets current audio recording duration in milliseconds.
     * @return unsigned long long Milliseconds recorded.
     */
    unsigned long long audio_get_recording_timestamp(void);

    /**
     * @brief Starts playback of an audio file.
     * @param sound_file_path Source audio filepath.
     * @param callback Optional playback progress/status callback.
     * @return rst_code_e RST_OK on success, or AUDIO_ERROR.
     */
    rst_code_e audio_play(const std::string &sound_file_path, ISoundSystem::PlaybackCallback callback = nullptr);

    /**
     * @brief Stops currently active audio playback stream.
     * @return rst_code_e RST_OK on success.
     */
    rst_code_e audio_stop_playing(void);

    /**
     * @brief Checks if audio playback is currently active.
     * @return true if playing, false otherwise.
     */
    bool audio_is_playing(void) const;

    /**
     * @brief Gets current audio playback position in milliseconds.
     * @return unsigned long long Milliseconds played.
     */
    unsigned long long audio_get_playing_timestamp(void);

    //-------------------------------------------------------------------------------------
    // AI Speech & Embedding Models Management (ManagerModels)
    //-------------------------------------------------------------------------------------

    /**
     * @brief Retrieves list of all Whisper and Llama models.
     * @param check_network If true, queries remote Hugging Face repository.
     * @param models Output vector receiving ModelInfo records.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e models_get_all(bool check_network, std::vector<ManagerModels::ModelInfo> &models) const;

    /**
     * @brief Retrieves list of Whisper speech recognition models.
     * @param check_network If true, checks remote network availability.
     * @param models Output vector receiving Whisper ModelInfo records.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e models_get_whisper(bool check_network, std::vector<ManagerModels::ModelInfo> &models) const;

    /**
     * @brief Retrieves list of Llama text embedding models.
     * @param check_network If true, checks remote network availability.
     * @param models Output vector receiving Llama ModelInfo records.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e models_get_llama(bool check_network, std::vector<ManagerModels::ModelInfo> &models) const;

    /**
     * @brief Checks if a Whisper model is present locally on disk.
     * @param model_name Model identifier name.
     * @return rst_code_e RST_OK if present, or MODEL_NOT_FOUND.
     */
    rst_code_e models_is_whisper_available(const std::string &model_name) const;

    /**
     * @brief Checks if a Llama model is present locally on disk.
     * @param model_name Model identifier name.
     * @return rst_code_e RST_OK if present, or MODEL_NOT_FOUND.
     */
    rst_code_e models_is_llama_available(const std::string &model_name) const;

    /**
     * @brief Downloads a Whisper model from Hugging Face.
     * @param model_name Name of Whisper model to download.
     * @param callback Optional progress callback handler.
     * @return rst_code_e RST_OK on successful download, or error code.
     */
    rst_code_e models_download_whisper(const std::string &model_name, DownloadProgressCallback callback = nullptr) const;

    /**
     * @brief Downloads a Llama embedding model from Hugging Face.
     * @param model_name Name of Llama model to download.
     * @param callback Optional progress callback handler.
     * @return rst_code_e RST_OK on successful download, or error code.
     */
    rst_code_e models_download_llama(const std::string &model_name, DownloadProgressCallback callback = nullptr) const;

    /**
     * @brief Downloads a model (Whisper or Llama) by type and name.
     * @param type ModelType enum (Whisper or Llama).
     * @param model_name Name of model.
     * @param callback Optional progress callback handler.
     * @return rst_code_e RST_OK on successful download, or error code.
     */
    rst_code_e models_download(ModelType type, const std::string &model_name, DownloadProgressCallback callback = nullptr) const;

    /**
     * @brief Deletes a local Whisper model file from disk.
     * @param model_name Model name to remove.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e models_remove_whisper(const std::string &model_name) const;

    /**
     * @brief Deletes a local Llama model file from disk.
     * @param model_name Model name to remove.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e models_remove_llama(const std::string &model_name) const;

    /**
     * @brief Auto-selects the best available Whisper model locally.
     * @return std::string Model name identifier.
     */
    std::string models_auto_select_whisper(void) const;

    /**
     * @brief Auto-selects the best available Llama model locally.
     * @return std::string Model name identifier.
     */
    std::string models_auto_select_llama(void) const;

    //-------------------------------------------------------------------------------------
    // Coverage Analysis & Execution Reports
    //-------------------------------------------------------------------------------------

    /**
     * @brief Executes audio-to-PDF coverage analysis with explicit model and parameter overrides.
     * @param practice_id Practice event ID.
     * @param out_execution_id Output string receiving generated execution GUID.
     * @param whisper_model Model override for Whisper (optional).
     * @param llama_model Model override for Llama embeddings (optional).
     * @param similarity_threshold Cosine similarity threshold override (optional).
     * @param language Spoken language code override (optional).
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e analyze_practice_coverage(
        int practice_id,
        std::string &out_execution_id,
        const std::string &whisper_model = "",
        const std::string &llama_model = "",
        float similarity_threshold = 0.0f,
        const std::string &language = ""
    );

    /**
     * @brief Executes audio-to-PDF coverage analysis using UserConfiguration parameters.
     * @param practice_id Practice event ID.
     * @param config Configuration parameters struct.
     * @param out_execution_id Output string receiving generated execution GUID.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e analyze_practice_coverage(
        int practice_id,
        const UserConfiguration &config,
        std::string &out_execution_id
    );

    /**
     * @brief Retrieves JSON list of all analysis executions for a practice session.
     * @param practice_id Practice event ID.
     * @param executions_list_json Output JSON array string.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e get_analysis_executions_for_practice(int practice_id, std::string &executions_list_json);

    /**
     * @brief Retrieves detailed report and configuration snapshot for an execution ID.
     * @param execution_id Unique GUID string of execution analysis.
     * @param report_json Output string receiving full report JSON.
     * @param config_json Output string receiving configuration snapshot JSON.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e get_analysis_execution_details(const std::string &execution_id, std::string &report_json, std::string &config_json);

    /**
     * @brief Retrieves report and configuration snapshot directly for a practice event's linked execution.
     * @param practice_id Practice event ID.
     * @param report_json Output string receiving full report JSON.
     * @param config_json Output string receiving configuration snapshot JSON.
     * @return rst_code_e RST_OK on success, PRACTICE_EVENT_NOT_FOUND, or error code.
     */
    rst_code_e get_analysis_execution_details_by_practice(int practice_id, std::string &report_json, std::string &config_json);

    //-------------------------------------------------------------------------------------
    // User Configuration Management
    //-------------------------------------------------------------------------------------

    /**
     * @brief Gets reference to active UserConfiguration for this session.
     * @return UserConfiguration& Mutable reference.
     */
    UserConfiguration& get_user_config(void);

    /**
     * @brief Gets const reference to active UserConfiguration for this session.
     * @return const UserConfiguration& Const reference.
     */
    const UserConfiguration& get_user_config(void) const;

    /**
     * @brief Sets and persists UserConfiguration for the authenticated user.
     * @param config New configuration parameters.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e set_user_config(const UserConfiguration &config);

    /**
     * @brief Loads persisted UserConfiguration from database for the authenticated user.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e load_user_config(void);

    /**
     * @brief Saves current UserConfiguration to database for the authenticated user.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e save_user_config(void);

    //-------------------------------------------------------------------------------------
    // Hardware (CPU & GPU) Detection
    //-------------------------------------------------------------------------------------

    /**
     * @brief Probes and retrieves detected CPU and GPU hardware information.
     * @param info Output reference receiving detected HardwareInfo.
     * @return rst_code_e RST_OK on success.
     */
    rst_code_e get_hardware_info(cantatema::HardwareInfo &info) const;

    /**
     * @brief Probes and returns detected CPU and GPU hardware information.
     * @return cantatema::HardwareInfo Containing detected CPU and GPU details.
     */
    cantatema::HardwareInfo get_hardware_info(void) const;

private:
    std::shared_ptr<IOperationUser> user_op{nullptr};
    std::shared_ptr<IOperationCategory> category_op{nullptr};
    std::shared_ptr<IOperationSubject> subject_op{nullptr};
    std::shared_ptr<IOperationUserMetrics> user_metrics_op{nullptr};
    std::shared_ptr<IOperationPracticeEvent> practice_event_op{nullptr};
    std::shared_ptr<IOperationCoverage> coverage_op{nullptr};
    std::shared_ptr<IDatabase> db_coverage_op{nullptr};
    std::shared_ptr<ISoundSystem> sound_op{nullptr};
    std::shared_ptr<ManagerModels> models_manager_op{nullptr};
    std::shared_ptr<cantatema::infra::IGpuDetector> gpu_detector_op{nullptr};

    //-------------------------------------------------------------------------------------

    std::shared_ptr<const User> session_user{nullptr};
    UserConfiguration session_user_config;
};

#endif //__SESSION_HPP