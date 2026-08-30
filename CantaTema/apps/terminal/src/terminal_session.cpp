#include "primitives/utils_functions.hpp"
#include "primitives/utils_prints.hpp"
#include "file_handler/file_handler.hpp"
#include "terminal/terminal_session.hpp"

#include <iomanip>


TerminalSession::TerminalSession()
{
    op = new Session();
    session_thread_pool = new ThreadPool(session_thread_pool_number);

    session_thread_pool->threads_initialize();
}
TerminalSession::~TerminalSession()
{
    session_thread_pool->threads_destroy();
    delete session_thread_pool;

    delete op;
}

void TerminalSession::user_add(std::ostream &out, const std::string &name, const std::string &password)
{
    rst_code_e rst{};

    rst = op->user_add(name, password);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("User added");
    }
}

void TerminalSession::user_get(std::ostream &out)
{

    rst_code_e rst{};

    std::shared_ptr<const User> current_user{nullptr};

    rst = op->user_get(current_user);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("User information:");
        logger->info(UtilsPrints::get_user_header());
        logger->info(UtilsPrints::get_user_body(*current_user));
    }
}

void TerminalSession::user_remove(std::ostream &out)
{

    rst_code_e rst{};

    rst = op->user_remove();
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("User removed");
    }
}

void TerminalSession::user_identify(std::ostream &out, const std::string &name, const std::string &password)
{
    rst_code_e rst{};

    rst = op->user_identify(name, password);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("User identified");
    }
}

void TerminalSession::category_add(std::ostream &out, const std::string &name)
{
    rst_code_e rst = op->category_add(name);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Category added");
    }
}

void TerminalSession::category_update(std::ostream &out, const unsigned int category_id, const std::string &new_name)
{
    rst_code_e rst = op->category_update(category_id, new_name);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Category updated");
    }
}

void TerminalSession::category_remove(std::ostream &out, const unsigned int category_id)
{
    rst_code_e rst = op->category_remove(category_id);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Category removed");
    }
}

void TerminalSession::category_get_by_user(std::ostream &out)
{
    std::vector<std::shared_ptr<const Category>> categories;
    rst_code_e rst = op->category_get_by_user(categories);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Category information:");
        logger->info(UtilsPrints::get_category_header());
        for (const auto &cat : categories)
        {
            logger->info(UtilsPrints::get_category_body(*cat));
        }
    }
}

void TerminalSession::tag_add(std::ostream &out, const std::string &name)
{
    rst_code_e rst = op->tag_add(name);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Tag added");
    }
}

void TerminalSession::tag_update(std::ostream &out, const unsigned int tag_id, const std::string &new_name)
{
    rst_code_e rst = op->tag_update(tag_id, new_name);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Tag updated");
    }
}

void TerminalSession::tag_remove(std::ostream &out, const unsigned int tag_id)
{
    rst_code_e rst = op->tag_remove(tag_id);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Tag removed");
    }
}

void TerminalSession::tag_get_by_user(std::ostream &out)
{
    std::vector<std::shared_ptr<Tag>> tags;
    rst_code_e rst = op->tag_get_by_user(tags);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Tag information:");
        logger->info(UtilsPrints::get_tag_header());
        for (const auto &tag : tags)
        {
            logger->info(UtilsPrints::get_tag_body(*tag));
        }
    }
}

void TerminalSession::subject_add_tag(std::ostream &out, unsigned int subject_id, unsigned int tag_id)
{
    rst_code_e rst = op->subject_add_tag(subject_id, tag_id);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Tag attached to subject");
    }
}

void TerminalSession::subject_remove_tag(std::ostream &out, unsigned int subject_id, unsigned int tag_id)
{
    rst_code_e rst = op->subject_remove_tag(subject_id, tag_id);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Tag removed from subject");
    }
}

void TerminalSession::subject_get_tags(std::ostream &out, unsigned int subject_id)
{
    std::vector<std::shared_ptr<Tag>> tags;
    rst_code_e rst = op->subject_get_tags(subject_id, tags);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Tags for Subject {}:", subject_id);
        logger->info(UtilsPrints::get_tag_header());
        for (const auto &tag : tags)
        {
            logger->info(UtilsPrints::get_tag_body(*tag));
        }
    }
}

void TerminalSession::subject_get_by_tag(std::ostream &out, unsigned int tag_id)
{
    std::vector<std::shared_ptr<Subject>> subjects;
    rst_code_e rst = op->subject_get_by_tag(tag_id, subjects);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Subjects matching Tag ID {}:", tag_id);
        logger->info(UtilsPrints::get_subject_header());
        for (const auto &sub : subjects)
        {
            logger->info(UtilsPrints::get_subject_body(*sub));
        }
    }
}

void TerminalSession::subject_add(std::ostream &out, unsigned int category_id, const std::string &name)
{
    std::string file_path;
    rst_code_e rst = FileHandler::get_file_path_from_user_selection(file_path);
    if(rst != RST_OK){
        logger->error("Error getting file path. {}", get_rst_txt(rst));
    }else{
        subject_add_from_path(out, category_id, name, file_path);
    }
}

void TerminalSession::subject_add_from_path(std::ostream &out, unsigned int category_id, const std::string &name, const std::string &file_path)
{
    logger->info("File path: {}", file_path);
    std::filesystem::path p(file_path);

    rst_code_e rst = op->subject_add(name, category_id, file_path);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Subject added");
    }
}

void TerminalSession::subject_update(std::ostream &out, unsigned int id, const unsigned int new_category_id, const std::string &name)
{
    std::string file_path;
    rst_code_e rst = FileHandler::get_file_path_from_user_selection(file_path);
    std::string file_path_new = "";
    std::string name_new = "";
    if(rst != RST_OK){
        logger->info("No new path provided.");
    }else{
        logger->info("New file path: {}", file_path);
        file_path_new = file_path;
        name_new = std::filesystem::path(file_path).stem().string();
    }

    rst = op->subject_update(id, name_new, new_category_id, file_path_new);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Subject updated");
    }
}

void TerminalSession::subject_remove(std::ostream &out, unsigned int id)
{
    rst_code_e rst = op->subject_remove(id);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Subject removed");
    }
}

void TerminalSession::subject_get_by_category(std::ostream &out, unsigned int category_id)
{
    std::vector<std::shared_ptr<Subject>> subjects;
    rst_code_e rst = op->subject_get_by_category(category_id, subjects);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        out << std::left << std::setw(10) << "ID" << std::setw(30) << "Name" << std::setw(30) << "Category" << std::setw(50) << "File Path" << std::endl;
        out << std::string(120, '-') << std::endl;

        for (const auto &sub : subjects)
        {
            std::string category_info = sub->get_category_id() == 0 ? "None" : std::to_string(sub->get_category_id());
            out << std::left << std::setw(10) << sub->get_id() << std::setw(30) << sub->get_name() << std::setw(30) << category_info << std::setw(50) << UtilsPrints::format_path_for_display(sub->get_filepath()) << std::endl;
        }
    }
}

void TerminalSession::subject_get_by_user(std::ostream &out)
{
    std::vector<std::shared_ptr<Subject>> subjects;
    rst_code_e rst = op->subject_get_by_user(subjects);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Subject information:");
        logger->info(UtilsPrints::get_subject_header());
        for (const auto &sub : subjects)
        {
            logger->info(UtilsPrints::get_subject_body(*sub));
        }
    }
}

void TerminalSession::user_metrics_get(std::ostream &out)
{
    std::shared_ptr<const UserMetrics> user_metrics;
    rst_code_e rst = op->user_metrics_get(user_metrics);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }else{
        logger->info("User metrics information:");
        logger->info(UtilsPrints::get_user_metrics_header());
        logger->info(UtilsPrints::get_user_metrics_body(*user_metrics));
    }
}

void TerminalSession::practice_event_add_planned(std::ostream &out, unsigned int subject_id, const std::string &date, const std::string &description)
{

    PracticeEvent practice;
    practice.set_subject_id(subject_id);
    practice.set_duration(0);
    practice.set_description(description);
    practice.set_date(parse_time_to_time_t(date, DATE_STRING_FORMAT_SHORT));

    rst_code_e rst = op->practice_event_add_planned(practice);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Practice event planned added");
    }
}

void TerminalSession::practice_event_add_recorded_from_file(std::ostream &out, unsigned int subject_id, const std::string &date)
{

    std::string file_path;
    rst_code_e rst = FileHandler::get_file_path_from_user_selection(file_path);
    if(rst != RST_OK){
        logger->error("Error getting file path. {}", get_rst_txt(rst));
    }

    logger->info("File path: {}", file_path);
    std::filesystem::path p(file_path);

    PracticeEvent practice;
    practice.set_subject_id(subject_id);
    practice.set_duration(0);
    practice.set_description(p.stem().string());
    practice.set_date(parse_time_to_time_t(date, DATE_STRING_FORMAT_SHORT));

    rst = op->practice_event_add_recorded(file_path, practice);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Practice event recorded added");
    }
}

void TerminalSession::practice_event_update(std::ostream &out, unsigned int id, const std::string &new_status, const std::string &description)
{
    std::shared_ptr<PracticeEvent> practice;
    rst_code_e rst = op->practice_event_get_by_id(id, practice);
    if (rst != RST_OK)
    {
        logger->error("Operation error fetching practice event: {}", get_rst_txt(rst));
        return;
    }

    practice->set_description(description);
    practice->set_status(PracticeEvent::parse_status_from_string(new_status));

    rst = op->practice_event_update(*practice);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Practice event updated");
    }
}

void TerminalSession::practice_event_remove(std::ostream &out, unsigned int id)
{
    rst_code_e rst = op->practice_event_remove(id);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Practice event removed");
    }
}

void TerminalSession::practice_event_get_by_id(std::ostream &out, unsigned int id)
{
    std::shared_ptr<PracticeEvent> practice;
    rst_code_e rst = op->practice_event_get_by_id(id, practice);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info(UtilsPrints::get_practice_event_header);
        logger->info(UtilsPrints::get_practice_event_body(*practice));
    }
}

void TerminalSession::practice_event_get_by_subject(std::ostream &out, unsigned int subject_id)
{
    std::vector<std::shared_ptr<PracticeEvent>> practices;
    rst_code_e rst = op->practice_event_get_by_subject(subject_id, practices);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info(UtilsPrints::get_practice_event_header());
        for (const auto &practice : practices)
        {
            logger->info(UtilsPrints::get_practice_event_body(*practice));
        }
    }
}

void TerminalSession::practice_event_get_by_user(std::ostream &out)
{
    std::vector<std::shared_ptr<PracticeEvent>> practices;
    rst_code_e rst = op->practice_event_get_by_user(practices);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Practice event information:");
        logger->info(UtilsPrints::get_practice_event_header());
        for (const auto &practice : practices)
        {
            logger->info(UtilsPrints::get_practice_event_body(*practice));
        }
    }
}

void TerminalSession::subject_set_language(std::ostream &out, unsigned int subject_id, const std::string &language)
{
    rst_code_e rst = op->set_subject_language(subject_id, language);
    if (rst)
    {
        logger->error("Operation error: {}", get_rst_txt(rst));
    }
    else
    {
        logger->info("Subject language set to '{}'", language);
    }
}

void TerminalSession::hardware_info(std::ostream &out)
{
    cantatema::HardwareInfo hw = op->get_hardware_info();

    out << "\n============================== Host Hardware Summary ==============================\n";
    out << "CPU Information:\n";
    out << "  Processor:       " << hw.cpu.name << "\n";
    out << "  Architecture:    " << hw.cpu.architecture << "\n";
    out << "  Logical Cores:   " << hw.cpu.core_count << "\n";
    if (hw.cpu.system_ram_mb > 0) {
        out << "  System RAM:      " << hw.cpu.system_ram_mb << " MB\n";
    }

    out << "\nGPU Devices Detected (" << hw.gpus.size() << "):\n";
    if (hw.gpus.empty()) {
        out << "  No dedicated or integrated GPU devices detected.\n";
    } else {
        for (std::size_t i = 0; i < hw.gpus.size(); ++i) {
            const auto &g = hw.gpus[i];
            out << "  [" << i << "] " << g.description << " (" << g.name << ")\n";
            out << "      Backend:     " << g.backend_name << "\n";
            out << "      Type:        " << g.type_str << "\n";
            if (g.memory_total_mb > 0) {
                out << "      VRAM:        " << g.memory_total_mb << " MB Total";
                if (g.memory_free_mb > 0 && g.memory_free_mb <= g.memory_total_mb) {
                    out << " / " << g.memory_free_mb << " MB Free";
                }
                out << "\n";
            }
        }
    }

    out << "\nHardware Acceleration Status:\n";
    out << "  CUDA Available:     " << (hw.has_cuda ? "YES" : "NO") << "\n";
    out << "  Vulkan Available:   " << (hw.has_vulkan ? "YES" : "NO") << "\n";
    out << "  Metal Available:    " << (hw.has_metal ? "YES" : "NO") << "\n";
    out << "  AI Acceleration:    " << (hw.use_gpu ? "ENABLED" : "DISABLED (CPU-only fallback)")
        << " [Selected Backend: " << hw.selected_backend << "]\n";
    out << "==================================================================================\n";
}

void TerminalSession::hardware_cpu(std::ostream &out)
{
    cantatema::HardwareInfo hw = op->get_hardware_info();

    out << "\n--- CPU Information ---\n";
    out << "  Processor:       " << hw.cpu.name << "\n";
    out << "  Architecture:    " << hw.cpu.architecture << "\n";
    out << "  Logical Cores:   " << hw.cpu.core_count << "\n";
    if (hw.cpu.system_ram_mb > 0) {
        out << "  System RAM:      " << hw.cpu.system_ram_mb << " MB\n";
    }
}

void TerminalSession::hardware_gpu(std::ostream &out)
{
    cantatema::HardwareInfo hw = op->get_hardware_info();

    out << "\n--- GPU Devices Detected (" << hw.gpus.size() << ") ---\n";
    if (hw.gpus.empty()) {
        out << "  No GPU devices detected.\n";
    } else {
        for (std::size_t i = 0; i < hw.gpus.size(); ++i) {
            const auto &g = hw.gpus[i];
            out << "  [" << i << "] " << g.description << " (" << g.name << ")\n";
            out << "      Backend:     " << g.backend_name << "\n";
            out << "      Type:        " << g.type_str << "\n";
            if (g.memory_total_mb > 0) {
                out << "      VRAM:        " << g.memory_total_mb << " MB Total\n";
            }
        }
    }
    out << "  AI Acceleration: " << (hw.use_gpu ? "ENABLED" : "DISABLED") << " (" << hw.selected_backend << ")\n";
}

