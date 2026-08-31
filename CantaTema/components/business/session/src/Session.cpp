/**
 * @file Session.cpp
 * @brief Implementation of the Session facade class.
 */

#include "session/Session.hpp"

#include "configuration/configuration_system.hpp"

#include "operations/operation_user.hpp"
#include "operations/operation_category.hpp"
#include "operations/operation_tag.hpp"
#include "operations/operation_subject.hpp"
#include "operations/operation_user_metrics.hpp"
#include "operations/operation_practice_event.hpp"
#include "operations/operation_coverage.hpp"
#include "operations/operation_analysis_scheduler.hpp"
#include "database/db_coverage.hpp"
#include "sound_system/sound_system.hpp"
#include "file_handler/file_handler.hpp"
#include "models/manager_models.hpp"
#include "speech_recognition/gpu_detector.hpp"

Session::Session(
    std::shared_ptr<IOperationUser> &&_user_op,
    std::shared_ptr<IOperationCategory> &&_category_op,
    std::shared_ptr<IOperationSubject> &&_subject_op,
    std::shared_ptr<IOperationUserMetrics> &&_user_metrics_op,
    std::shared_ptr<IOperationPracticeEvent> &&_practice_event_op,
    std::shared_ptr<IOperationCoverage> &&_coverage_op,
    std::shared_ptr<IDatabase> &&_db_op,
    std::shared_ptr<ISoundSystem> &&_sound_system_op,
    std::shared_ptr<ManagerModels> &&_models_manager_op,
    std::shared_ptr<cantatema::infra::IGpuDetector> &&_gpu_detector_op,
    std::shared_ptr<IOperationAnalysisScheduler> &&_scheduler_op,
    std::shared_ptr<IOperationTag> &&_tag_op
) : user_op(std::move(_user_op)),
    category_op(std::move(_category_op)),
    tag_op(std::move(_tag_op)),
    subject_op(std::move(_subject_op)),
    user_metrics_op(std::move(_user_metrics_op)),
    practice_event_op(std::move(_practice_event_op)),
    coverage_op(std::move(_coverage_op)),
    db_coverage_op(std::move(_db_op)),
    sound_op(std::move(_sound_system_op)),
    models_manager_op(std::move(_models_manager_op)),
    gpu_detector_op(std::move(_gpu_detector_op)),
    scheduler_op(std::move(_scheduler_op))
{
    if (user_op == nullptr || category_op == nullptr || subject_op == nullptr || user_metrics_op == nullptr || practice_event_op == nullptr)
    {
        throw std::runtime_error("Operation session received wrong operation instances.");
    }
    if (tag_op == nullptr)
    {
        tag_op = std::make_shared<OperationTag>();
    }
    if (db_coverage_op == nullptr)
    {
        db_coverage_op = std::make_shared<DB_Coverage>();
    }
    if (coverage_op == nullptr)
    {
        coverage_op = std::make_shared<OperationCoverage>(db_coverage_op, subject_op, practice_event_op);
    }
    if (sound_op == nullptr)
    {
        sound_op = std::make_shared<SoundSystem>(ISoundSystem::SoundSystemConfig{});
    }
    if (models_manager_op == nullptr)
    {
        models_manager_op = std::make_shared<ManagerModels>();
    }
    if (gpu_detector_op == nullptr)
    {
        gpu_detector_op = std::make_shared<cantatema::infra::GpuDetector>();
    }
    if (scheduler_op == nullptr)
    {
        scheduler_op = std::make_shared<OperationAnalysisScheduler>(db_coverage_op, coverage_op, practice_event_op, user_op);
    }
    initialize();
}

Session::Session(void)
{
    user_metrics_op = std::make_shared<OperationUserMetrics>();
    category_op = std::make_shared<OperationCategory>();
    tag_op = std::make_shared<OperationTag>();
    user_op = std::make_shared<OperationUser>(std::shared_ptr<IOperationUserMetrics>(user_metrics_op));
    subject_op = std::make_shared<OperationSubject>(std::shared_ptr<IOperationUserMetrics>(user_metrics_op), std::shared_ptr<IOperationCategory>(category_op));
    practice_event_op = std::make_shared<OperationPracticeEvent>(std::shared_ptr<IOperationUserMetrics>(user_metrics_op), std::shared_ptr<IOperationSubject>(subject_op));
    db_coverage_op = std::make_shared<DB_Coverage>();
    coverage_op = std::make_shared<OperationCoverage>(db_coverage_op, subject_op, practice_event_op);
    sound_op = std::make_shared<SoundSystem>(ISoundSystem::SoundSystemConfig{});
    models_manager_op = std::make_shared<ManagerModels>();
    gpu_detector_op = std::make_shared<cantatema::infra::GpuDetector>();
    scheduler_op = std::make_shared<OperationAnalysisScheduler>(db_coverage_op, coverage_op, practice_event_op, user_op);

    if (user_op == nullptr || category_op == nullptr || tag_op == nullptr || subject_op == nullptr || user_metrics_op == nullptr || practice_event_op == nullptr || coverage_op == nullptr || db_coverage_op == nullptr || sound_op == nullptr || models_manager_op == nullptr || gpu_detector_op == nullptr || scheduler_op == nullptr)
    {
        throw std::runtime_error("Operation session received wrong operation instances. (2)");
    }
    initialize();
}

Session::~Session(void)
{
    if (scheduler_op) {
        scheduler_op->stop_scheduler();
    }
    session_user = nullptr;
}

rst_code_e Session::initialize(void)
{
    session_user = nullptr;
    session_user_config.set_default_values();
    if (scheduler_op) {
        scheduler_op->start_scheduler();
    }
    return RST_OK;
}

//-------------------------------------------------------------------------------------
// User Management & Authentication
//-------------------------------------------------------------------------------------

rst_code_e Session::user_add(const std::string &name, const std::string &password)
{
    return user_op->user_add(name, password);
}

rst_code_e Session::user_get(std::shared_ptr<const User> &user)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    user = std::const_pointer_cast<const User>(session_user);
    return RST_OK;
}

rst_code_e Session::user_get_by_name(std::string user_name, std::shared_ptr<const User> &user)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return user_op->user_get_by_name(user_name, user);
}

rst_code_e Session::user_update(std::shared_ptr<const User> &user)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    if (session_user->get_name() != user->get_name())
        return USER_ERROR;

    return user_op->user_update(user);
}

rst_code_e Session::user_remove(void)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    rst_code_e rst = user_op->user_remove();
    if (rst)
        return rst;

    // Clear local user variable after removal
    session_user = nullptr;
    session_user_config.set_default_values();

    return RST_OK;
}

bool Session::user_is_authenticated(void)
{
    if (session_user == nullptr)
        return false;

    return user_op->user_is_authenticated();
}

rst_code_e Session::user_identify(const std::string &name, const std::string &password)
{
    // Remove session information
    session_user = nullptr;

    rst_code_e rst = user_op->user_identify(name, password);
    if (rst)
        return rst;

    rst = user_op->user_get(session_user);
    if (rst)
        return rst;

    load_user_config();
    return RST_OK;
}

rst_code_e Session::user_logout(void)
{
    session_user = nullptr;
    session_user_config.set_default_values();
    return RST_OK;
}

//-------------------------------------------------------------------------------------
// Category Management
//-------------------------------------------------------------------------------------

rst_code_e Session::category_add(const std::string &name)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    Category category(0, name);
    category.set_user_id(session_user->get_useraccountid());

    return category_op->category_add(session_user, category);
}

rst_code_e Session::category_update(const unsigned int category_id, const std::string &new_name)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Category>> categories;
    rst_code_e rst = category_op->category_get_all_by_user(session_user, categories);
    if (rst != RST_OK)
        return rst;

    for (auto &cat : categories)
    {
        if (cat->get_id() == category_id)
        {
            cat->set_name(new_name);
            return category_op->category_update(session_user, *cat);
        }
    }
    return CATEGORY_NOT_FOUND;
}

rst_code_e Session::category_remove(const unsigned int category_id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Category>> categories;
    rst_code_e rst = category_op->category_get_all_by_user(session_user, categories);
    if (rst != RST_OK)
        return rst;

    for (auto &cat : categories)
    {
        if (cat->get_id() == category_id)
        {
            return category_op->category_remove(category_id);
        }
    }
    return CATEGORY_NOT_FOUND;
}

rst_code_e Session::category_get_by_id(const unsigned int category_id, std::shared_ptr<Category> &category)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<Category> cat;
    rst_code_e rst = category_op->category_get_by_id(category_id, cat);
    if (rst != RST_OK)
        return rst;

    if (!cat || cat->get_user_id() != session_user->get_useraccountid())
        return CATEGORY_NOT_FOUND;

    category = cat;
    return RST_OK;
}

rst_code_e Session::category_get_by_user(std::vector<std::shared_ptr<const Category>> &categories)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Category>> user_categories;
    rst_code_e rst = category_op->category_get_all_by_user(session_user, user_categories);
    if (rst != RST_OK)
        return rst;

    categories.clear();
    for (const auto &cat : user_categories)
    {
        categories.push_back(cat);
    }

    return RST_OK;
}

//-------------------------------------------------------------------------------------
// Tag Management
//-------------------------------------------------------------------------------------

rst_code_e Session::tag_add(const std::string &name)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    Tag tag(0, name);
    return tag_op->tag_add(session_user, tag);
}

rst_code_e Session::tag_update(unsigned int tag_id, const std::string &new_name)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    Tag tag(tag_id, new_name);
    return tag_op->tag_update(session_user, tag);
}

rst_code_e Session::tag_remove(unsigned int tag_id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return tag_op->tag_remove(session_user, tag_id);
}

rst_code_e Session::tag_get_by_id(unsigned int tag_id, std::shared_ptr<Tag> &tag)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return tag_op->tag_get_by_id(session_user, tag_id, tag);
}

rst_code_e Session::tag_get_by_name(const std::string &name, std::shared_ptr<Tag> &tag)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return tag_op->tag_get_by_name(session_user, name, tag);
}

rst_code_e Session::tag_get_by_user(std::vector<std::shared_ptr<Tag>> &tags)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return tag_op->tag_get_all_by_user(session_user, tags);
}

rst_code_e Session::subject_add_tag(unsigned int subject_id, unsigned int tag_id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return tag_op->subject_add_tag(session_user, subject_id, tag_id);
}

rst_code_e Session::subject_remove_tag(unsigned int subject_id, unsigned int tag_id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return tag_op->subject_remove_tag(session_user, subject_id, tag_id);
}

rst_code_e Session::subject_get_tags(unsigned int subject_id, std::vector<std::shared_ptr<Tag>> &tags)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return tag_op->subject_get_tags(session_user, subject_id, tags);
}

rst_code_e Session::subject_get_by_tag(unsigned int tag_id, std::vector<std::shared_ptr<Subject>> &subjects)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return tag_op->subject_get_all_by_tag(session_user, tag_id, subjects);
}

//-------------------------------------------------------------------------------------
// Subject Management
//-------------------------------------------------------------------------------------

rst_code_e Session::subject_add(const std::string &name, unsigned int category_id, const std::string &file_path)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    Subject subject(0, name);
    subject.set_user_id(session_user->get_useraccountid());
    subject.set_category_id(category_id);

    return subject_op->subject_add(session_user, file_path, subject);
}

rst_code_e Session::subject_update(unsigned int id, const std::string &new_name, const unsigned int new_category_id, const std::string &file_path_new)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<Subject> subject;
    rst_code_e rst = subject_op->subject_get_by_id(session_user, id, subject);
    if (rst != RST_OK)
        return rst;

    subject->set_name(new_name);
    subject->set_category_id(new_category_id);
    subject->set_filepath(file_path_new);

    return subject_op->subject_update(session_user, *subject);
}

rst_code_e Session::subject_remove(unsigned int id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return subject_op->subject_remove(session_user, id);
}

rst_code_e Session::subject_get_by_id(unsigned int subject_id, std::shared_ptr<Subject> &subject)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return subject_op->subject_get_by_id(session_user, subject_id, subject);
}

rst_code_e Session::subject_get_by_category(unsigned int category_id, std::vector<std::shared_ptr<Subject>> &subjects)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Category>> categories;
    rst_code_e rst = category_op->category_get_all_by_user(session_user, categories);
    if (rst != RST_OK)
        return rst;

    bool found = false;
    for (const auto &cat : categories)
    {
        if (cat->get_id() == category_id)
        {
            found = true;
            break;
        }
    }

    if (!found)
        return CATEGORY_NOT_FOUND;

    std::vector<std::shared_ptr<Subject>> user_subjects;
    rst = subject_op->subject_get_all_by_category(session_user, category_id, user_subjects);
    if (rst != RST_OK)
        return rst;

    subjects.clear();
    for (const auto &sub : user_subjects)
    {
        subjects.push_back(sub);
    }

    return RST_OK;
}

rst_code_e Session::subject_get_by_user(std::vector<std::shared_ptr<Subject>> &subjects)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::vector<std::shared_ptr<Subject>> user_subjects;
    rst_code_e rst = subject_op->subject_get_all_by_user(session_user, user_subjects);
    if (rst != RST_OK)
        return rst;

    subjects.clear();
    for (const auto &sub : user_subjects)
    {
        subjects.push_back(sub);
    }

    return RST_OK;
}

rst_code_e Session::set_subject_language(int subject_id, const std::string &language)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<Subject> subject;
    rst_code_e rst = subject_op->subject_get_by_id(session_user, subject_id, subject);
    if (rst != RST_OK)
        return rst;

    subject->set_language(language);
    return subject_op->subject_update(session_user, *subject);
}

rst_code_e Session::get_subject_language(int subject_id, std::string &language)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<Subject> subject;
    rst_code_e rst = subject_op->subject_get_by_id(session_user, subject_id, subject);
    if (rst != RST_OK)
        return rst;

    language = subject->get_language();
    return RST_OK;
}

//-------------------------------------------------------------------------------------
// User Metrics
//-------------------------------------------------------------------------------------

rst_code_e Session::user_metrics_get(std::shared_ptr<const UserMetrics> &user_metrics)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<UserMetrics> temp_metrics;
    rst_code_e rst = user_metrics_op->user_metrics_get(session_user, temp_metrics);
    user_metrics = temp_metrics;

    return rst;
}

rst_code_e Session::user_metrics_can_accept_file_size(unsigned int size_in_kb)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return user_metrics_op->user_metrics_can_accept_file_size(session_user, size_in_kb);
}

//-------------------------------------------------------------------------------------
// Practice Event Operations
//-------------------------------------------------------------------------------------

rst_code_e Session::practice_event_add_planned(PracticeEvent &practice)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_add_planned(session_user, practice);
}

rst_code_e Session::practice_event_add_recorded(const std::string &source_file, PracticeEvent &practice)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_add_recorded(session_user, source_file, practice);
}

rst_code_e Session::practice_event_update(const PracticeEvent &practice)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_update(session_user, practice);
}

rst_code_e Session::practice_event_remove(unsigned int id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_remove(session_user, id);
}

rst_code_e Session::practice_event_remove_by_subject(unsigned int subject_id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_remove_by_subject_id(session_user, subject_id);
}

rst_code_e Session::practice_event_remove_all_by_user(void)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_remove_by_user_id(session_user);
}

rst_code_e Session::practice_event_get_by_id(unsigned int id, std::shared_ptr<PracticeEvent> &practice)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_get_by_id(session_user, id, practice);
}

rst_code_e Session::practice_event_get_by_subject(unsigned int subject_id, std::vector<std::shared_ptr<PracticeEvent>> &practices)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_get_all_by_subject(session_user, subject_id, practices);
}

rst_code_e Session::practice_event_get_by_user(std::vector<std::shared_ptr<PracticeEvent>> &practices)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    return practice_event_op->practice_event_get_all_by_user(session_user, practices);
}

//-------------------------------------------------------------------------------------
// Audio Hardware, Recording & Playback (ISoundSystem)
//-------------------------------------------------------------------------------------

rst_code_e Session::audio_get_capture_devices(std::vector<ISoundSystem::SoundSystemDeviceInfo> &devices)
{
    if (!sound_op)
        return UNKNOWN;
    devices = sound_op->getCaptureDevices();
    return RST_OK;
}

rst_code_e Session::audio_start_recording(const std::string &output_file_path, int device_index)
{
    if (!sound_op)
        return UNKNOWN;
    SoundFileHandler handler(output_file_path);
    if (!sound_op->startRecording(handler, device_index))
        return UNKNOWN;
    return RST_OK;
}

rst_code_e Session::audio_stop_recording(void)
{
    if (!sound_op)
        return UNKNOWN;
    sound_op->stopRecording();
    return RST_OK;
}

bool Session::audio_is_recording(void) const
{
    if (!sound_op)
        return false;
    return sound_op->isRecording();
}

unsigned long long Session::audio_get_recording_timestamp(void)
{
    if (!sound_op)
        return 0;
    return sound_op->get_recording_timestamp();
}

rst_code_e Session::audio_play(const std::string &sound_file_path, ISoundSystem::PlaybackCallback callback)
{
    if (!sound_op)
        return UNKNOWN;
    SoundFileHandler handler(sound_file_path);
    if (!sound_op->play(handler, callback))
        return UNKNOWN;
    return RST_OK;
}

rst_code_e Session::audio_stop_playing(void)
{
    if (!sound_op)
        return UNKNOWN;
    sound_op->stopPlaying();
    return RST_OK;
}

bool Session::audio_is_playing(void) const
{
    if (!sound_op)
        return false;
    return sound_op->isPlaying();
}

unsigned long long Session::audio_get_playing_timestamp(void)
{
    if (!sound_op)
        return 0;
    return sound_op->get_playing_timestamp();
}

rst_code_e Session::audio_stream_read_range(
    unsigned int practice_id,
    uint64_t offset,
    size_t length,
    std::vector<uint8_t>& out_buffer,
    bool& out_is_eof
)
{
    if (!user_is_authenticated())
        return USER_NO_AUTH;
    if (!practice_event_op || !sound_op)
        return UNKNOWN;

    std::shared_ptr<PracticeEvent> practice;
    rst_code_e rst = practice_event_op->practice_event_get_by_id(session_user, practice_id, practice);
    if (rst != RST_OK || !practice)
        return rst != RST_OK ? rst : PRACTICE_EVENT_NOT_FOUND;

    std::string filepath = practice->get_filepath();
    if (filepath.empty())
        return FILE_NOT_FOUND;

    SoundFileHandler sound_handler(filepath);
    return sound_op->read_decrypted_audio_range(sound_handler, offset, length, out_buffer, out_is_eof);
}

rst_code_e Session::document_stream_read_range(
    unsigned int subject_id,
    uint64_t offset,
    size_t length,
    std::vector<uint8_t>& out_buffer,
    bool& out_is_eof
)
{
    if (!user_is_authenticated())
        return USER_NO_AUTH;
    if (!subject_op)
        return UNKNOWN;

    std::shared_ptr<Subject> subject;
    rst_code_e rst = subject_op->subject_get_by_id(session_user, subject_id, subject);
    if (rst != RST_OK || !subject)
        return rst != RST_OK ? rst : SUBJECT_NOT_FOUND;

    std::string filepath = subject->get_filepath();
    if (filepath.empty())
        return FILE_NOT_FOUND;

    FileHandler file_handler(filepath, 0);
    return file_handler.read_range(offset, length, out_buffer, out_is_eof);
}

//-------------------------------------------------------------------------------------
// AI Speech & Embedding Models Management (ManagerModels)
//-------------------------------------------------------------------------------------

rst_code_e Session::models_get_all(bool check_network, std::vector<ManagerModels::ModelInfo> &models) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->get_available_models(check_network, models);
}

rst_code_e Session::models_get_whisper(bool check_network, std::vector<ManagerModels::ModelInfo> &models) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->get_whisper_models(check_network, models);
}

rst_code_e Session::models_get_llama(bool check_network, std::vector<ManagerModels::ModelInfo> &models) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->get_llama_models(check_network, models);
}

rst_code_e Session::models_is_whisper_available(const std::string &model_name) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->local_is_whisper_model_available(model_name);
}

rst_code_e Session::models_is_llama_available(const std::string &model_name) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->local_is_llama_model_available(model_name);
}

rst_code_e Session::models_download_whisper(const std::string &model_name, DownloadProgressCallback callback) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->network_download_model(ModelType::Whisper, model_name, callback);
}

rst_code_e Session::models_download_llama(const std::string &model_name, DownloadProgressCallback callback) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->network_download_model(ModelType::Llama, model_name, callback);
}

rst_code_e Session::models_download(ModelType type, const std::string &model_name, DownloadProgressCallback callback) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->network_download_model(type, model_name, callback);
}

rst_code_e Session::models_remove_whisper(const std::string &model_name) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->local_remove_whisper_model(model_name);
}

rst_code_e Session::models_remove_llama(const std::string &model_name) const
{
    if (!models_manager_op)
        return UNKNOWN;
    return models_manager_op->local_remove_llama_model(model_name);
}

std::string Session::models_auto_select_whisper(void) const
{
    if (!models_manager_op)
        return "";
    return models_manager_op->auto_select_whisper_model();
}

std::string Session::models_auto_select_llama(void) const
{
    if (!models_manager_op)
        return "";
    return models_manager_op->auto_select_llama_model();
}

//-------------------------------------------------------------------------------------
// Coverage Analysis & Execution Reports
//-------------------------------------------------------------------------------------

rst_code_e Session::analyze_practice_coverage(
    int practice_id,
    std::string &out_execution_id,
    const std::string &whisper_model,
    const std::string &llama_model,
    float similarity_threshold,
    const std::string &language
)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    if (!coverage_op)
        return UNKNOWN;

    return coverage_op->analyze_practice_coverage(
        session_user,
        practice_id,
        whisper_model,
        llama_model,
        similarity_threshold,
        language,
        out_execution_id
    );
}

rst_code_e Session::analyze_practice_coverage(int practice_id, const UserConfiguration &config, std::string &out_execution_id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    if (!coverage_op)
        return UNKNOWN;

    return coverage_op->analyze_practice_coverage(session_user, practice_id, config, out_execution_id);
}

rst_code_e Session::get_analysis_executions_for_practice(int practice_id, std::string &executions_list_json)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<PracticeEvent> practice;
    rst_code_e rst = practice_event_op->practice_event_get_by_id(session_user, practice_id, practice);
    if (rst != RST_OK)
        return rst;

    if (!db_coverage_op)
        return UNKNOWN;

    return db_coverage_op->get_analysis_executions_for_practice(practice_id, executions_list_json);
}

rst_code_e Session::get_analysis_execution_details(const std::string &execution_id, std::string &report_json, std::string &config_json)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    if (!db_coverage_op)
        return UNKNOWN;

    return db_coverage_op->get_analysis_execution_details(execution_id, report_json, config_json);
}

rst_code_e Session::get_analysis_execution_details_by_practice(int practice_id, std::string &report_json, std::string &config_json)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;

    std::shared_ptr<PracticeEvent> practice;
    rst_code_e rst = practice_event_op->practice_event_get_by_id(session_user, practice_id, practice);
    if (rst != RST_OK)
        return rst;

    if (!practice)
        return PRACTICE_EVENT_NOT_FOUND;

    std::string exec_id = practice->get_analysis_execution_id();
    if (exec_id.empty())
        return PRACTICE_EVENT_NOT_FOUND;

    return get_analysis_execution_details(exec_id, report_json, config_json);
}

//-------------------------------------------------------------------------------------
// User Configuration Management
//-------------------------------------------------------------------------------------

UserConfiguration& Session::get_user_config(void)
{
    return session_user_config;
}

const UserConfiguration& Session::get_user_config(void) const
{
    return session_user_config;
}

rst_code_e Session::set_user_config(const UserConfiguration &config)
{
    session_user_config = config;
    return save_user_config();
}

rst_code_e Session::load_user_config(void)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
    {
        session_user_config.set_default_values();
        return USER_NO_AUTH;
    }
    if (!db_coverage_op)
    {
        session_user_config.set_default_values();
        return RST_OK;
    }
    rst_code_e res = db_coverage_op->get_user_configuration(session_user->get_useraccountid(), session_user_config);
    if (res != RST_OK)
    {
        session_user_config.set_default_values();
    }
    return RST_OK;
}

rst_code_e Session::save_user_config(void)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
        return USER_NO_AUTH;
    if (!db_coverage_op)
        return UNKNOWN;
    return db_coverage_op->save_user_configuration(session_user->get_useraccountid(), session_user_config);
}

rst_code_e Session::get_hardware_info(cantatema::HardwareInfo &info) const
{
    if (gpu_detector_op)
    {
        info = gpu_detector_op->detect_hardware();
    }
    else
    {
        info = cantatema::infra::detect_hardware();
    }
    return RST_OK;
}

cantatema::HardwareInfo Session::get_hardware_info(void) const
{
    cantatema::HardwareInfo info{};
    get_hardware_info(info);
    return info;
}

rst_code_e Session::analysis_task_submit(int practice_id, std::string &out_task_id, const UserConfiguration &config)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
    {
        return USER_NO_AUTH;
    }
    if (!scheduler_op)
    {
        return UNKNOWN;
    }
    const UserConfiguration &effective_config = (config.comparison.similarity_threshold > 0.0f || !config.whisper.model_name.empty()) ? config : session_user_config;
    return scheduler_op->submit_task(session_user, practice_id, effective_config, out_task_id);
}

rst_code_e Session::analysis_task_cancel(const std::string &task_id)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
    {
        return USER_NO_AUTH;
    }
    if (!scheduler_op)
    {
        return UNKNOWN;
    }
    return scheduler_op->cancel_task(session_user, task_id);
}

rst_code_e Session::analysis_task_get_status(const std::string &task_id, AnalysisTask &out_task)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
    {
        return USER_NO_AUTH;
    }
    if (!scheduler_op)
    {
        return UNKNOWN;
    }
    return scheduler_op->get_task_status(session_user, task_id, out_task);
}

rst_code_e Session::analysis_task_get_user_tasks(std::vector<AnalysisTask> &out_tasks)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
    {
        return USER_NO_AUTH;
    }
    if (!scheduler_op)
    {
        return UNKNOWN;
    }
    return scheduler_op->get_user_tasks(session_user, out_tasks);
}

rst_code_e Session::analysis_task_get_all_tasks(std::vector<AnalysisTask> &out_tasks)
{
    if (session_user == nullptr || !user_op->user_is_authenticated())
    {
        return USER_NO_AUTH;
    }
    if (!scheduler_op)
    {
        return UNKNOWN;
    }
    return scheduler_op->get_all_tasks(session_user, out_tasks);
}

void Session::analysis_task_set_max_parallel(size_t max_tasks)
{
    if (scheduler_op)
    {
        scheduler_op->set_max_parallel_tasks(max_tasks);
    }
}

size_t Session::analysis_task_get_max_parallel() const
{
    if (scheduler_op)
    {
        return scheduler_op->get_max_parallel_tasks();
    }
    return 1;
}


