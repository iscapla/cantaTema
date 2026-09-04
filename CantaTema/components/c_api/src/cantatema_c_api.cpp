/**
 * @file cantatema_c_api.cpp
 * @brief Implementation of C ABI middleware layer connecting CantaTema core with Flutter.
 */

#include "c_api/cantatema_c_api.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <fstream>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "session/Session.hpp"
#include "primitives/definitions.hpp"
#include "primitives/tool_paths.hpp"
#include "primitives/user.hpp"
#include "primitives/category.hpp"
#include "primitives/subject.hpp"
#include "primitives/tag.hpp"
#include "primitives/practice_event.hpp"
#include "primitives/analysis_task.hpp"
#include "primitives/user_configuration.hpp"
#include "primitives/utils_logger.hpp"
#include "models/manager_models.hpp"
#include "database/db_main.hpp"

namespace {

// Thread-safety and single active session state
std::mutex g_engine_mutex;
std::unique_ptr<Session> g_session{nullptr};

// Active audio recording context
std::string g_active_recording_topic_id;
std::string g_active_recording_filepath;
std::chrono::system_clock::time_point g_active_recording_start;
std::atomic<bool> g_is_paused{false};
std::atomic<bool> g_mock_recording_for_test{false};
std::atomic<bool> g_mock_is_recording{false};

/**
 * @brief Allocates heap memory for C string using std::malloc so Dart FFI can call canta_free_string.
 */
char* allocate_string(const std::string& str) {
    char* copy = static_cast<char*>(std::malloc(str.size() + 1));
    if (copy) {
        std::memcpy(copy, str.c_str(), str.size() + 1);
    }
    return copy;
}

bool is_engine_ready() {
    return (g_session != nullptr);
}

} // namespace

//-----------------------------------------------------------------------------------------
// 1. Engine Lifecycle & Session Management
//-----------------------------------------------------------------------------------------

void canta_init_logger_for_test(void) {
    if (logger == nullptr) {
        util_logger_init_for_test();
    }
}

void canta_set_mock_recording_for_test(int enabled) {
    g_mock_recording_for_test.store(enabled != 0);
    if (!enabled) {
        g_mock_is_recording.store(false);
    }
}

void canta_purge_database_for_test(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (g_session) {
            if (g_session->audio_is_recording() || g_mock_is_recording.load()) {
                g_session->audio_stop_recording();
                g_mock_is_recording.store(false);
            }
            if (g_session->audio_is_playing()) {
                g_session->audio_stop_playing();
            }
            g_session->user_logout();
            g_session.reset();
        }
        g_active_recording_topic_id.clear();
        g_active_recording_filepath.clear();
        g_is_paused.store(false);
        g_mock_is_recording.store(false);

        DB_Main::getInstance()->purge();
    } catch (...) {
    }
}

int32_t canta_init_engine(const char* storage_path, const char* config_json) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);

        if (logger == nullptr) {
            util_logger_init();
        }

        if (storage_path && std::strlen(storage_path) > 0) {
            ToolPath::set_base_path(storage_path);
        }

        // Limit to max 1 active session in Flutter in-process embedding
        if (g_session) {
            if (g_session->audio_is_recording() || g_mock_is_recording.load()) {
                g_session->audio_stop_recording();
                g_mock_is_recording.store(false);
            }
            if (g_session->audio_is_playing()) {
                g_session->audio_stop_playing();
            }
            g_session->user_logout();
            g_session.reset();
        }

        g_session = std::make_unique<Session>();
        rst_code_e res = g_session->initialize();
        if (res != RST_OK) {
            g_session.reset();
            return static_cast<int32_t>(res);
        }

        g_active_recording_topic_id.clear();
        g_active_recording_filepath.clear();
        g_is_paused.store(false);
        g_mock_is_recording.store(false);

        return static_cast<int32_t>(RST_OK);
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

void canta_shutdown_engine(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (g_session) {
            if (g_session->audio_is_recording() || g_mock_is_recording.load()) {
                g_session->audio_stop_recording();
                g_mock_is_recording.store(false);
            }
            if (g_session->audio_is_playing()) {
                g_session->audio_stop_playing();
            }
            g_session->user_logout();
            g_session.reset();
        }
        g_active_recording_topic_id.clear();
        g_active_recording_filepath.clear();
        g_is_paused.store(false);
        g_mock_is_recording.store(false);
    } catch (...) {
    }
}

void canta_free_string(const char* ptr) {
    if (ptr) {
        std::free(const_cast<char*>(ptr));
    }
}

const char* canta_get_engine_version(void) {
    return allocate_string("1.0.0-core");
}

const char* canta_get_error_text(int32_t error_code) {
    std::string text = get_rst_txt(static_cast<rst_code_e>(error_code));
    return allocate_string(text);
}

//-----------------------------------------------------------------------------------------
// 2. User Authentication & Profile
//-----------------------------------------------------------------------------------------

const char* canta_get_current_user_json(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return nullptr;
        }

        std::shared_ptr<const User> user;
        if (g_session->user_get(user) != RST_OK || !user) {
            return nullptr;
        }

        nlohmann::json j;
        j["id"] = std::to_string(user->get_useraccountid());
        j["name"] = user->get_name();
        j["email"] = user->get_workemail().empty() ? user->get_name() : user->get_workemail();
        std::string display = user->get_firstname();
        if (!user->get_lastname().empty()) {
            if (!display.empty()) display += " ";
            display += user->get_lastname();
        }
        j["displayName"] = display.empty() ? user->get_name() : display;
        j["firstName"] = user->get_firstname();
        j["lastName"] = user->get_lastname();
        j["status"] = user->parse_status_to_string(user->get_status());

        return allocate_string(j.dump());
    } catch (...) {
        return nullptr;
    }
}

const char* canta_login_user_json(const char* email_or_name, const char* password) {
    try {
        if (!email_or_name || !password) return nullptr;

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready()) return nullptr;

        rst_code_e res = g_session->user_identify(email_or_name, password);
        if (res != RST_OK) {
            return nullptr;
        }

        std::shared_ptr<const User> user;
        if (g_session->user_get(user) != RST_OK || !user) {
            return nullptr;
        }

        nlohmann::json j;
        j["id"] = std::to_string(user->get_useraccountid());
        j["name"] = user->get_name();
        j["email"] = user->get_workemail().empty() ? user->get_name() : user->get_workemail();
        std::string display = user->get_firstname();
        if (!user->get_lastname().empty()) {
            if (!display.empty()) display += " ";
            display += user->get_lastname();
        }
        j["displayName"] = display.empty() ? user->get_name() : display;
        j["firstName"] = user->get_firstname();
        j["lastName"] = user->get_lastname();
        j["status"] = user->parse_status_to_string(user->get_status());

        return allocate_string(j.dump());
    } catch (...) {
        return nullptr;
    }
}

const char* canta_register_user_json(const char* email_or_name, const char* password, const char* display_name) {
    try {
        if (!email_or_name || !password) return nullptr;

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready()) return nullptr;

        rst_code_e res = g_session->user_add(email_or_name, password);
        if (res != RST_OK) {
            return nullptr;
        }

        // Authenticate into active session
        g_session->user_identify(email_or_name, password);

        // Update display name / email if provided
        std::shared_ptr<const User> user_const;
        if (g_session->user_get(user_const) == RST_OK && user_const) {
            auto user_mut = std::make_shared<User>(*user_const);
            if (display_name && std::strlen(display_name) > 0) {
                std::string full_name_str(display_name);
                auto space_pos = full_name_str.find(' ');
                if (space_pos != std::string::npos) {
                    user_mut->set_firstname(full_name_str.substr(0, space_pos));
                    user_mut->set_lastname(full_name_str.substr(space_pos + 1));
                } else {
                    user_mut->set_firstname(full_name_str);
                }
            }
            std::shared_ptr<const User> user_to_update = user_mut;
            g_session->user_update(user_to_update);
        }

        std::shared_ptr<const User> final_user;
        if (g_session->user_get(final_user) != RST_OK || !final_user) {
            return nullptr;
        }

        nlohmann::json j;
        j["id"] = std::to_string(final_user->get_useraccountid());
        j["name"] = final_user->get_name();
        j["email"] = final_user->get_workemail();
        j["displayName"] = display_name ? display_name : final_user->get_name();
        j["firstName"] = final_user->get_firstname();
        j["lastName"] = final_user->get_lastname();
        j["status"] = final_user->parse_status_to_string(final_user->get_status());

        return allocate_string(j.dump());
    } catch (...) {
        return nullptr;
    }
}

int32_t canta_logout_user(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready()) return static_cast<int32_t>(USER_NO_AUTH);
        return static_cast<int32_t>(g_session->user_logout());
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

//-----------------------------------------------------------------------------------------
// 3. Topic & Category Management
//-----------------------------------------------------------------------------------------

const char* canta_get_categories_json(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return allocate_string("[]");
        }

        std::vector<std::shared_ptr<const Category>> categories;
        rst_code_e res = g_session->category_get_by_user(categories);
        if (res != RST_OK) {
            return allocate_string("[]");
        }

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& cat : categories) {
            if (!cat) continue;
            nlohmann::json item;
            item["id"] = std::to_string(cat->get_id());
            item["name"] = cat->get_name();
            item["averageMastery"] = 0.0;
            item["totalSessions"] = 0;
            arr.push_back(item);
        }

        return allocate_string(arr.dump());
    } catch (...) {
        return allocate_string("[]");
    }
}

const char* canta_get_all_topics_json(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return allocate_string("[]");
        }

        std::vector<std::shared_ptr<Subject>> subjects;
        rst_code_e res = g_session->subject_get_by_user(subjects);
        if (res != RST_OK) {
            return allocate_string("[]");
        }

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& s : subjects) {
            if (!s) continue;
            nlohmann::json item;
            item["id"] = std::to_string(s->get_id());
            item["categoryId"] = std::to_string(s->get_category_id());

            std::shared_ptr<Category> cat;
            if (s->get_category_id() > 0 && g_session->category_get_by_id(s->get_category_id(), cat) == RST_OK && cat) {
                item["categoryName"] = cat->get_name();
            } else {
                item["categoryName"] = "";
            }

            item["title"] = s->get_name();
            item["filePath"] = s->get_filepath();
            item["language"] = s->get_language().empty() ? "es" : s->get_language();

            std::vector<std::shared_ptr<Tag>> tags;
            g_session->subject_get_tags(s->get_id(), tags);
            std::vector<std::string> tag_names;
            for (const auto& t : tags) {
                if (t) tag_names.push_back(t->get_name());
            }
            item["tags"] = tag_names;
            item["mastery"] = 0.0;
            item["totalSessions"] = 0;

            arr.push_back(item);
        }

        return allocate_string(arr.dump());
    } catch (...) {
        return allocate_string("[]");
    }
}

const char* canta_get_topics_by_category_json(const char* category_id) {
    try {
        if (!category_id) return allocate_string("[]");

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return allocate_string("[]");
        }

        unsigned int cat_id = 0;
        try {
            cat_id = static_cast<unsigned int>(std::stoul(category_id));
        } catch (...) {
            return allocate_string("[]");
        }

        std::vector<std::shared_ptr<Subject>> subjects;
        rst_code_e res = g_session->subject_get_by_category(cat_id, subjects);
        if (res != RST_OK) {
            return allocate_string("[]");
        }

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& s : subjects) {
            if (!s) continue;
            nlohmann::json item;
            item["id"] = std::to_string(s->get_id());
            item["categoryId"] = std::to_string(s->get_category_id());
            item["title"] = s->get_name();
            item["filePath"] = s->get_filepath();
            item["language"] = s->get_language().empty() ? "es" : s->get_language();

            std::vector<std::shared_ptr<Tag>> tags;
            g_session->subject_get_tags(s->get_id(), tags);
            std::vector<std::string> tag_names;
            for (const auto& t : tags) {
                if (t) tag_names.push_back(t->get_name());
            }
            item["tags"] = tag_names;

            arr.push_back(item);
        }

        return allocate_string(arr.dump());
    } catch (...) {
        return allocate_string("[]");
    }
}

const char* canta_get_topic_by_id_json(const char* topic_id) {
    try {
        if (!topic_id) return nullptr;

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return nullptr;
        }

        unsigned int s_id = 0;
        try {
            s_id = static_cast<unsigned int>(std::stoul(topic_id));
        } catch (...) {
            return nullptr;
        }

        std::shared_ptr<Subject> s;
        if (g_session->subject_get_by_id(s_id, s) != RST_OK || !s) {
            return nullptr;
        }

        nlohmann::json item;
        item["id"] = std::to_string(s->get_id());
        item["categoryId"] = std::to_string(s->get_category_id());

        std::shared_ptr<Category> cat;
        if (s->get_category_id() > 0 && g_session->category_get_by_id(s->get_category_id(), cat) == RST_OK && cat) {
            item["categoryName"] = cat->get_name();
        } else {
            item["categoryName"] = "";
        }

        item["title"] = s->get_name();
        item["filePath"] = s->get_filepath();
        item["language"] = s->get_language().empty() ? "es" : s->get_language();

        std::vector<std::shared_ptr<Tag>> tags;
        g_session->subject_get_tags(s->get_id(), tags);
        std::vector<std::string> tag_names;
        for (const auto& t : tags) {
            if (t) tag_names.push_back(t->get_name());
        }
        item["tags"] = tag_names;

        return allocate_string(item.dump());
    } catch (...) {
        return nullptr;
    }
}

const char* canta_create_topic_json(const char* topic_payload_json) {
    try {
        if (!topic_payload_json) return nullptr;

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return nullptr;
        }

        auto payload = nlohmann::json::parse(topic_payload_json);
        std::string title = payload.value("title", payload.value("name", ""));
        if (title.empty()) return nullptr;

        unsigned int category_id = 0;
        if (payload.contains("categoryId")) {
            if (payload["categoryId"].is_number()) {
                category_id = payload["categoryId"].get<unsigned int>();
            } else if (payload["categoryId"].is_string()) {
                std::string cat_str = payload["categoryId"].get<std::string>();
                if (!cat_str.empty()) {
                    category_id = static_cast<unsigned int>(std::stoul(cat_str));
                }
            }
        }

        if (category_id == 0 && payload.contains("categoryName")) {
            std::string cat_name = payload["categoryName"].get<std::string>();
            if (!cat_name.empty()) {
                // Check if existing category matches
                std::vector<std::shared_ptr<const Category>> existing_cats;
                if (g_session->category_get_by_user(existing_cats) == RST_OK) {
                    for (const auto& c : existing_cats) {
                        if (c && c->get_name() == cat_name) {
                            category_id = c->get_id();
                            break;
                        }
                    }
                }
                if (category_id == 0) {
                    g_session->category_add(cat_name);
                    if (g_session->category_get_by_user(existing_cats) == RST_OK) {
                        for (const auto& c : existing_cats) {
                            if (c && c->get_name() == cat_name) {
                                category_id = c->get_id();
                                break;
                            }
                        }
                    }
                }
            }
        }

        std::string file_path = payload.value("filePath", payload.value("rawFileContent", ""));
        std::string language = payload.value("language", "es");

        bool created_temp = false;
        std::filesystem::path temp_file;
        if (file_path.empty() || !std::filesystem::exists(file_path)) {
            temp_file = std::filesystem::temp_directory_path() / ("canta_temp_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".txt");
            std::ofstream ofs(temp_file);
            ofs << title << "\n";
            ofs.close();
            file_path = temp_file.string();
            created_temp = true;
        }

        rst_code_e res = g_session->subject_add(title, category_id, file_path);
        if (created_temp && std::filesystem::exists(temp_file)) {
            std::error_code ec;
            std::filesystem::remove(temp_file, ec);
        }

        if (res != RST_OK) {
            return nullptr;
        }

        // Query created subject
        std::vector<std::shared_ptr<Subject>> subjects;
        g_session->subject_get_by_user(subjects);
        std::shared_ptr<Subject> created_subj;
        for (const auto& s : subjects) {
            if (s && s->get_name() == title) {
                created_subj = s;
                break;
            }
        }

        if (!created_subj) return nullptr;

        if (!language.empty()) {
            g_session->set_subject_language(created_subj->get_id(), language);
            created_subj->set_language(language);
        }

        // Handle tags
        std::vector<std::string> tags_created;
        if (payload.contains("tags") && payload["tags"].is_array()) {
            for (const auto& t_json : payload["tags"]) {
                std::string tag_name = t_json.get<std::string>();
                if (tag_name.empty()) continue;

                g_session->tag_add(tag_name);
                std::shared_ptr<Tag> t_obj;
                if (g_session->tag_get_by_name(tag_name, t_obj) == RST_OK && t_obj) {
                    g_session->subject_add_tag(created_subj->get_id(), t_obj->get_id());
                    tags_created.push_back(tag_name);
                }
            }
        }

        nlohmann::json item;
        item["id"] = std::to_string(created_subj->get_id());
        item["categoryId"] = std::to_string(category_id);
        item["categoryName"] = payload.value("categoryName", "");
        item["title"] = created_subj->get_name();
        item["filePath"] = created_subj->get_filepath();
        item["language"] = language;
        item["tags"] = tags_created;
        item["mastery"] = 0.0;
        item["totalSessions"] = 0;

        return allocate_string(item.dump());
    } catch (...) {
        return nullptr;
    }
}

int32_t canta_update_topic_json(const char* topic_payload_json) {
    try {
        if (!topic_payload_json) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        auto payload = nlohmann::json::parse(topic_payload_json);
        if (!payload.contains("id")) return static_cast<int32_t>(DB_BAD_PARAM);

        unsigned int topic_id = 0;
        if (payload["id"].is_number()) {
            topic_id = payload["id"].get<unsigned int>();
        } else {
            topic_id = static_cast<unsigned int>(std::stoul(payload["id"].get<std::string>()));
        }

        std::shared_ptr<Subject> existing;
        if (g_session->subject_get_by_id(topic_id, existing) != RST_OK || !existing) {
            return static_cast<int32_t>(SUBJECT_NOT_FOUND);
        }

        std::string new_name = payload.value("title", payload.value("name", existing->get_name()));
        unsigned int new_category_id = existing->get_category_id();
        if (payload.contains("categoryId")) {
            if (payload["categoryId"].is_number()) {
                new_category_id = payload["categoryId"].get<unsigned int>();
            } else {
                new_category_id = static_cast<unsigned int>(std::stoul(payload["categoryId"].get<std::string>()));
            }
        }
        std::string new_filepath = payload.value("filePath", existing->get_filepath());

        rst_code_e res = g_session->subject_update(topic_id, new_name, new_category_id, new_filepath);
        if (res != RST_OK) return static_cast<int32_t>(res);

        if (payload.contains("language")) {
            std::string lang = payload["language"].get<std::string>();
            g_session->set_subject_language(topic_id, lang);
        }

        return static_cast<int32_t>(RST_OK);
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

int32_t canta_delete_topic(const char* topic_id) {
    try {
        if (!topic_id) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        unsigned int id = static_cast<unsigned int>(std::stoul(topic_id));
        return static_cast<int32_t>(g_session->subject_remove(id));
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

//-----------------------------------------------------------------------------------------
// 4. Audio Capture, DSP Metering & Recording Studio
//-----------------------------------------------------------------------------------------

int32_t canta_start_recording_session(const char* topic_id) {
    try {
        if (!topic_id) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        if (g_session->audio_is_recording() || g_mock_is_recording.load()) {
            return static_cast<int32_t>(PRACTICE_EVENT_ERROR);
        }

        std::shared_ptr<const User> active_user;
        g_session->user_get(active_user);
        unsigned int user_id = active_user ? active_user->get_useraccountid() : 1;
        unsigned int subject_id = 0;
        try {
            subject_id = static_cast<unsigned int>(std::stoul(topic_id));
        } catch (...) {
            subject_id = 1;
        }

        std::filesystem::path sound_path = ToolPath::get_path_for_practice_event(user_id, subject_id);
        std::error_code dir_ec;
        std::filesystem::create_directories(sound_path, dir_ec);

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        std::string out_path = (sound_path / ("rec_" + std::to_string(now_ms) + ".opus")).string();

        rst_code_e res = RST_OK;
        if (g_mock_recording_for_test.load()) {
            std::vector<std::filesystem::path> candidates = {
                "CantaTema/example_data/subject_es_1_p_1.opus",
                "../CantaTema/example_data/subject_es_1_p_1.opus",
                "../../CantaTema/example_data/subject_es_1_p_1.opus",
                "../../../CantaTema/example_data/subject_es_1_p_1.opus"
            };
            for (const auto& c : candidates) {
                if (std::filesystem::exists(c)) {
                    std::error_code ec;
                    std::filesystem::copy_file(c, out_path, std::filesystem::copy_options::overwrite_existing, ec);
                    break;
                }
            }
            g_mock_is_recording.store(true);
        } else {
            res = g_session->audio_start_recording(out_path);
        }

        if (res != RST_OK) {
            return static_cast<int32_t>(res);
        }

        g_active_recording_topic_id = topic_id;
        g_active_recording_filepath = out_path;
        g_active_recording_start = std::chrono::system_clock::now();
        g_is_paused.store(false);

        return static_cast<int32_t>(RST_OK);
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

int32_t canta_pause_recording_session(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready()) return static_cast<int32_t>(USER_NO_AUTH);
        g_is_paused.store(true);
        return static_cast<int32_t>(RST_OK);
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

int32_t canta_resume_recording_session(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready()) return static_cast<int32_t>(USER_NO_AUTH);
        g_is_paused.store(false);
        return static_cast<int32_t>(RST_OK);
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

const char* canta_stop_recording_session(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return nullptr;
        }

        bool is_recording = g_session->audio_is_recording() || g_mock_is_recording.load();
        if (!is_recording) {
            return nullptr;
        }

        unsigned int duration_sec = 1;
        if (g_mock_is_recording.load()) {
            g_mock_is_recording.store(false);
            duration_sec = 10;
        } else {
            unsigned long long duration_ms = g_session->audio_get_recording_timestamp();
            duration_sec = static_cast<unsigned int>(duration_ms / 1000ULL);
            if (duration_sec == 0) duration_sec = 1;
            g_session->audio_stop_recording();
        }

        unsigned int topic_id_num = 0;
        try {
            topic_id_num = static_cast<unsigned int>(std::stoul(g_active_recording_topic_id));
        } catch (...) {
            topic_id_num = 0;
        }

        PracticeEvent practice;
        practice.set_subject_id(topic_id_num);
        practice.set_status(PracticeEvent::RECORDED);
        practice.set_filepath(g_active_recording_filepath);
        practice.set_duration(duration_sec);
        practice.set_recorded_date(static_cast<unsigned int>(std::time(nullptr)));

        rst_code_e res = g_session->practice_event_add_recorded(g_active_recording_filepath, practice);
        if (res != RST_OK) {
            return nullptr;
        }

        nlohmann::json j;
        j["id"] = std::to_string(practice.get_id());
        j["topicId"] = g_active_recording_topic_id;
        j["audioPath"] = g_active_recording_filepath;
        j["durationSeconds"] = duration_sec;
        j["status"] = "completed";
        j["recordedDate"] = practice.get_recorded_date();

        g_active_recording_topic_id.clear();
        g_active_recording_filepath.clear();
        g_is_paused.store(false);

        return allocate_string(j.dump());
    } catch (...) {
        return nullptr;
    }
}

float canta_get_current_audio_amplitude(void) {
    try {
        if (g_is_paused.load()) return 0.0f;
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (g_mock_is_recording.load()) {
            return 0.42f;
        }
        if (!is_engine_ready() || !g_session->audio_is_recording()) {
            return 0.0f;
        }
        return g_session->audio_get_current_amplitude();
    } catch (...) {
        return 0.0f;
    }
}

const char* canta_poll_live_transcription(void) {
    // Return null while live streaming Whisper STT is inactive
    return nullptr;
}

//-----------------------------------------------------------------------------------------
// 5. Cante Session History
//-----------------------------------------------------------------------------------------

const char* canta_create_study_session_json(const char* topic_id, const char* audio_path, uint32_t duration_seconds) {
    try {
        if (!topic_id) return nullptr;

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return nullptr;
        }

        unsigned int topic_id_num = 0;
        try {
            topic_id_num = static_cast<unsigned int>(std::stoul(topic_id));
        } catch (...) {
            topic_id_num = 0;
        }

        std::string source_file;
        if (audio_path && std::strlen(audio_path) > 0 && std::filesystem::exists(audio_path)) {
            source_file = audio_path;
        } else {
            std::vector<std::filesystem::path> candidates = {
                "CantaTema/example_data/subject_es_1_p_1.opus",
                "../CantaTema/example_data/subject_es_1_p_1.opus",
                "../../CantaTema/example_data/subject_es_1_p_1.opus",
                "../../../CantaTema/example_data/subject_es_1_p_1.opus"
            };
            for (const auto& c : candidates) {
                if (std::filesystem::exists(c)) {
                    source_file = c.string();
                    break;
                }
            }
        }

        if (source_file.empty()) {
            return nullptr;
        }

        PracticeEvent practice;
        practice.set_subject_id(topic_id_num);
        practice.set_status(PracticeEvent::RECORDED);
        practice.set_filepath(source_file);
        practice.set_duration(duration_seconds > 0 ? duration_seconds : 60);
        practice.set_recorded_date(static_cast<unsigned int>(std::time(nullptr)));

        rst_code_e res = g_session->practice_event_add_recorded(source_file, practice);
        if (res != RST_OK) {
            return nullptr;
        }

        nlohmann::json j;
        j["id"] = std::to_string(practice.get_id());
        j["topicId"] = topic_id;
        j["audioPath"] = practice.get_filepath();
        j["durationSeconds"] = practice.get_duration();
        j["status"] = "completed";
        j["recordedDate"] = practice.get_recorded_date();

        return allocate_string(j.dump());
    } catch (...) {
        return nullptr;
    }
}

const char* canta_get_sessions_for_topic_json(const char* topic_id) {
    try {
        if (!topic_id) return allocate_string("[]");

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return allocate_string("[]");
        }

        unsigned int s_id = static_cast<unsigned int>(std::stoul(topic_id));
        std::vector<std::shared_ptr<PracticeEvent>> practices;
        rst_code_e res = g_session->practice_event_get_by_subject(s_id, practices);
        if (res != RST_OK) {
            return allocate_string("[]");
        }

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& p : practices) {
            if (!p) continue;
            nlohmann::json item;
            item["id"] = std::to_string(p->get_id());
            item["topicId"] = std::to_string(p->get_subject_id());
            item["status"] = PracticeEvent::get_status_as_string(p->get_status());
            item["audioPath"] = p->get_filepath();
            item["durationSeconds"] = p->get_duration();
            item["recordedDate"] = p->get_recorded_date();
            item["executionId"] = p->get_analysis_execution_id();
            arr.push_back(item);
        }

        return allocate_string(arr.dump());
    } catch (...) {
        return allocate_string("[]");
    }
}

const char* canta_get_recent_sessions_json(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return allocate_string("[]");
        }

        std::vector<std::shared_ptr<PracticeEvent>> practices;
        rst_code_e res = g_session->practice_event_get_by_user(practices);
        if (res != RST_OK) {
            return allocate_string("[]");
        }

        std::sort(practices.begin(), practices.end(), [](const auto& a, const auto& b) {
            if (!a || !b) return false;
            return a->get_recorded_date() > b->get_recorded_date();
        });

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& p : practices) {
            if (!p) continue;
            nlohmann::json item;
            item["id"] = std::to_string(p->get_id());
            item["topicId"] = std::to_string(p->get_subject_id());

            std::shared_ptr<Subject> subj;
            if (g_session->subject_get_by_id(p->get_subject_id(), subj) == RST_OK && subj) {
                item["topicTitle"] = subj->get_name();
            } else {
                item["topicTitle"] = "";
            }

            item["status"] = PracticeEvent::get_status_as_string(p->get_status());
            item["audioPath"] = p->get_filepath();
            item["durationSeconds"] = p->get_duration();
            item["recordedDate"] = p->get_recorded_date();
            item["executionId"] = p->get_analysis_execution_id();
            arr.push_back(item);
        }

        return allocate_string(arr.dump());
    } catch (...) {
        return allocate_string("[]");
    }
}

const char* canta_get_session_by_id_json(const char* session_id) {
    try {
        if (!session_id) return nullptr;

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return nullptr;
        }

        unsigned int id = static_cast<unsigned int>(std::stoul(session_id));
        std::shared_ptr<PracticeEvent> p;
        if (g_session->practice_event_get_by_id(id, p) != RST_OK || !p) {
            return nullptr;
        }

        nlohmann::json item;
        item["id"] = std::to_string(p->get_id());
        item["topicId"] = std::to_string(p->get_subject_id());

        std::shared_ptr<Subject> subj;
        if (g_session->subject_get_by_id(p->get_subject_id(), subj) == RST_OK && subj) {
            item["topicTitle"] = subj->get_name();
        } else {
            item["topicTitle"] = "";
        }

        item["status"] = PracticeEvent::get_status_as_string(p->get_status());
        item["audioPath"] = p->get_filepath();
        item["durationSeconds"] = p->get_duration();
        item["recordedDate"] = p->get_recorded_date();
        item["executionId"] = p->get_analysis_execution_id();

        return allocate_string(item.dump());
    } catch (...) {
        return nullptr;
    }
}

int32_t canta_delete_session(const char* session_id) {
    try {
        if (!session_id) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        unsigned int id = static_cast<unsigned int>(std::stoul(session_id));
        return static_cast<int32_t>(g_session->practice_event_remove(id));
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

//-----------------------------------------------------------------------------------------
// 6. Speech Analysis & Syllabus Evaluation
//-----------------------------------------------------------------------------------------

const char* canta_generate_session_analysis_json(const char* session_id) {
    try {
        if (!session_id) return nullptr;

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return nullptr;
        }

        int practice_id = std::stoi(session_id);
        std::string exec_id;
        rst_code_e res = g_session->analyze_practice_coverage(practice_id, exec_id);
        if (res != RST_OK || exec_id.empty()) {
            return nullptr;
        }

        std::string report_json, config_json;
        if (g_session->get_analysis_execution_details(exec_id, report_json, config_json) != RST_OK) {
            return nullptr;
        }

        return allocate_string(report_json);
    } catch (...) {
        return nullptr;
    }
}

const char* canta_get_session_analysis_report_json(const char* session_id) {
    try {
        if (!session_id) return nullptr;

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return nullptr;
        }

        int practice_id = std::stoi(session_id);
        std::string report_json, config_json;
        if (g_session->get_analysis_execution_details_by_practice(practice_id, report_json, config_json) != RST_OK) {
            return nullptr;
        }

        return allocate_string(report_json);
    } catch (...) {
        return nullptr;
    }
}

//-----------------------------------------------------------------------------------------
// 7. Task Scheduler & Agenda Calendar
//-----------------------------------------------------------------------------------------

const char* canta_get_schedule_items_json(int32_t status_filter, const char* search_query) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return allocate_string("[]");
        }

        std::vector<AnalysisTask> tasks;
        g_session->analysis_task_get_user_tasks(tasks);

        std::string search_str = search_query ? search_query : "";

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& t : tasks) {
            int task_status_code = static_cast<int>(t.get_status());
            if (status_filter >= 0 && task_status_code != status_filter) {
                continue;
            }

            if (!search_str.empty() && t.get_task_id().find(search_str) == std::string::npos) {
                continue;
            }

            nlohmann::json item;
            item["id"] = t.get_task_id();
            item["practiceId"] = t.get_practice_id();
            item["status"] = AnalysisTask::status_to_string(t.get_status());
            item["statusCode"] = task_status_code;
            item["progress"] = t.get_progress_percentage();
            item["stage"] = t.get_stage_description();
            item["errorMessage"] = t.get_error_message();
            arr.push_back(item);
        }

        return allocate_string(arr.dump());
    } catch (...) {
        return allocate_string("[]");
    }
}

const char* canta_create_schedule_item_json(const char* schedule_payload_json) {
    try {
        if (!schedule_payload_json) return nullptr;

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return nullptr;
        }

        auto payload = nlohmann::json::parse(schedule_payload_json);
        int practice_id = payload.value("practiceId", 0);
        if (practice_id <= 0) return nullptr;

        std::string task_id;
        rst_code_e res = g_session->analysis_task_submit(practice_id, task_id);
        if (res != RST_OK || task_id.empty()) {
            return nullptr;
        }

        nlohmann::json item;
        item["id"] = task_id;
        item["practiceId"] = practice_id;
        item["status"] = "waiting";
        item["statusCode"] = 0;
        item["progress"] = 0;

        return allocate_string(item.dump());
    } catch (...) {
        return nullptr;
    }
}

int32_t canta_update_schedule_item_status(const char* item_id, int32_t new_status) {
    try {
        if (!item_id) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        // Cancel requested
        if (new_status == 3) {
            return static_cast<int32_t>(g_session->analysis_task_cancel(item_id));
        }

        return static_cast<int32_t>(RST_OK);
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

int32_t canta_delete_schedule_item(const char* item_id) {
    try {
        if (!item_id) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        return static_cast<int32_t>(g_session->analysis_task_cancel(item_id));
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

const char* canta_get_calendar_events_json(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return allocate_string("[]");
        }

        std::vector<std::shared_ptr<PracticeEvent>> practices;
        rst_code_e res = g_session->practice_event_get_by_user(practices);
        if (res != RST_OK) {
            return allocate_string("[]");
        }

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& p : practices) {
            if (!p) continue;
            nlohmann::json item;
            item["id"] = std::to_string(p->get_id());
            item["topicId"] = std::to_string(p->get_subject_id());

            std::shared_ptr<Subject> subj;
            if (g_session->subject_get_by_id(p->get_subject_id(), subj) == RST_OK && subj) {
                item["topicTitle"] = subj->get_name();
            } else {
                item["topicTitle"] = "";
            }

            item["status"] = (p->get_status() == PracticeEvent::PLANNED) ? "planned" : "recorded";
            item["date"] = (p->get_status() == PracticeEvent::PLANNED) ? p->get_date() : p->get_recorded_date();
            item["durationSeconds"] = p->get_duration();
            item["executionId"] = p->get_analysis_execution_id();
            arr.push_back(item);
        }

        return allocate_string(arr.dump());
    } catch (...) {
        return allocate_string("[]");
    }
}

int32_t canta_add_calendar_event_json(const char* event_payload_json) {
    try {
        if (!event_payload_json) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        auto payload = nlohmann::json::parse(event_payload_json);
        unsigned int topic_id = 0;
        if (payload.contains("topicId")) {
            if (payload["topicId"].is_number()) {
                topic_id = payload["topicId"].get<unsigned int>();
            } else {
                topic_id = static_cast<unsigned int>(std::stoul(payload["topicId"].get<std::string>()));
            }
        }

        unsigned int date = payload.value("date", static_cast<unsigned int>(std::time(nullptr)));

        PracticeEvent practice;
        practice.set_subject_id(topic_id);
        practice.set_date(date);
        practice.set_status(PracticeEvent::PLANNED);
        practice.set_description(payload.value("description", ""));

        return static_cast<int32_t>(g_session->practice_event_add_planned(practice));
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

int32_t canta_remove_calendar_event(const char* event_id) {
    try {
        if (!event_id) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        unsigned int id = static_cast<unsigned int>(std::stoul(event_id));
        return static_cast<int32_t>(g_session->practice_event_remove(id));
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

//-----------------------------------------------------------------------------------------
// 8. Local AI Model Management
//-----------------------------------------------------------------------------------------

const char* canta_get_ai_models_json(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready()) {
            return allocate_string("[]");
        }

        std::vector<ManagerModels::ModelInfo> models;
        g_session->models_get_all(false, models);

        const auto& user_cfg = g_session->get_user_config();

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& m : models) {
            nlohmann::json item;
            item["name"] = m.name;
            item["type"] = (m.type == ModelType::Whisper) ? "whisper" : "llama";
            item["isDownloaded"] = m.available_local;
            item["path"] = m.path;
            item["isActive"] = (m.name == user_cfg.whisper.model_name || m.name == user_cfg.comparison.embedding_model_name);
            arr.push_back(item);
        }

        return allocate_string(arr.dump());
    } catch (...) {
        return allocate_string("[]");
    }
}

int32_t canta_update_ai_model_json(const char* model_payload_json) {
    try {
        if (!model_payload_json) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        auto payload = nlohmann::json::parse(model_payload_json);
        std::string name = payload.value("name", "");
        std::string type = payload.value("type", "");
        bool is_active = payload.value("isActive", false);

        if (name.empty()) return static_cast<int32_t>(DB_BAD_PARAM);

        if (is_active) {
            auto cfg = g_session->get_user_config();
            if (type == "whisper") {
                cfg.whisper.model_name = name;
            } else if (type == "llama") {
                cfg.comparison.embedding_model_name = name;
            }
            g_session->set_user_config(cfg);
            g_session->save_user_config();
        }

        return static_cast<int32_t>(RST_OK);
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

//-----------------------------------------------------------------------------------------
// 9. Audio Playback & Streaming Range
//-----------------------------------------------------------------------------------------

int32_t canta_start_playback(const char* session_id) {
    try {
        if (!session_id) return static_cast<int32_t>(DB_BAD_PARAM);

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        unsigned int id = static_cast<unsigned int>(std::stoul(session_id));
        std::shared_ptr<PracticeEvent> p;
        if (g_session->practice_event_get_by_id(id, p) != RST_OK || !p) {
            return static_cast<int32_t>(PRACTICE_EVENT_NOT_FOUND);
        }

        if (p->get_filepath().empty()) {
            return static_cast<int32_t>(FILE_NOT_FOUND);
        }

        return static_cast<int32_t>(g_session->audio_play(p->get_filepath()));
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

int32_t canta_stop_playback(void) {
    try {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready()) return static_cast<int32_t>(USER_NO_AUTH);
        return static_cast<int32_t>(g_session->audio_stop_playing());
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}

int32_t canta_read_audio_stream(
    uint32_t session_id,
    uint64_t offset,
    uint32_t length,
    uint8_t* out_buffer,
    uint32_t* out_bytes_read,
    int32_t* out_is_eof
) {
    try {
        if (!out_buffer || !out_bytes_read || !out_is_eof) {
            return static_cast<int32_t>(DB_BAD_PARAM);
        }

        std::lock_guard<std::mutex> lock(g_engine_mutex);
        if (!is_engine_ready() || !g_session->user_is_authenticated()) {
            return static_cast<int32_t>(USER_NO_AUTH);
        }

        std::vector<uint8_t> buffer;
        bool is_eof = false;
        rst_code_e res = g_session->audio_stream_read_range(
            session_id,
            offset,
            length,
            buffer,
            is_eof
        );

        if (res != RST_OK) {
            *out_bytes_read = 0;
            *out_is_eof = 1;
            return static_cast<int32_t>(res);
        }

        size_t to_copy = std::min(static_cast<size_t>(length), buffer.size());
        if (to_copy > 0) {
            std::memcpy(out_buffer, buffer.data(), to_copy);
        }
        *out_bytes_read = static_cast<uint32_t>(to_copy);
        *out_is_eof = is_eof ? 1 : 0;

        return static_cast<int32_t>(RST_OK);
    } catch (...) {
        return static_cast<int32_t>(UNKNOWN);
    }
}
