#include "primitives/utils_functions.hpp"
#include "primitives/utils_prints.hpp"
#include "file_handler/file_handler.hpp"
#include "terminal/terminal_session.hpp"

#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <iostream>
#include <filesystem>
#include <ctime>

void TerminalSession::practice_event_add_recorded(std::ostream &out, unsigned int subject_id, const std::string name){

    std::vector<ISoundSystem::SoundSystemDeviceInfo> devices;
    op->audio_get_capture_devices(devices);
    int device_index = -1;
    std::string device_name;

    for (const auto &d : devices)
    {
        if (d.isDefault)
        {
            device_index = d.index;
            device_name = d.name;
            break;
        }
    }

    if (device_index == -1)
    {
        if (!devices.empty())
        {
            device_index = devices[0].index;
            device_name = devices[0].name;
            out << "Warning: No default device found. Using: " << device_name << std::endl;
        }
        else
        {
            out << "Error: No capture devices found." << std::endl;
            return;
        }
    }
    else
    {
        out << "Using default device: " << device_name << std::endl;
    }

    out << "Press 'y' to start recording, or 'n'/'q' to cancel." << std::endl;

    char input;
    while (true)
    {
        std::cin >> input;
        if (!std::cin)
            return;
        if (input == 'y')
            break;
        if (input == 'n' || input == 'q')
            return;
    }

    std::string temp_file = "temp_recording.opus";
    rst_code_e rst_rec = op->audio_start_recording(temp_file, device_index);
    if (rst_rec != RST_OK)
    {
        out << "Error: Could not start recording: " << get_rst_txt(rst_rec) << std::endl;
        return;
    }

    out << "Recording started. Press 'q' to stop." << std::endl;

    std::atomic<bool> running{true};

    std::thread timer_thread([this, &out, &running]() {
        char old_fill = out.fill('0');
        while (running)
        {
            unsigned long long ms = op->audio_get_recording_timestamp();
            unsigned long long s = ms / 1000;
            int mm = s / 60;
            int ss = s % 60;

            out << "\rTimer: "
                << std::setw(2) << mm << ":"
                << std::setw(2) << ss
                << std::flush;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        out.fill(old_fill);
    });

    while (true)
    {
        std::cin >> input;
        if (!std::cin || input == 'q')
        {
            running = false;
            break;
        }
    }

    if (timer_thread.joinable())
        timer_thread.join();

    unsigned long long duration_ms = op->audio_get_recording_timestamp();
    op->audio_stop_recording();
    out << std::endl << "Recording stopped." << std::endl;

    out << "Do you want to save the recording? (y/n): ";
    char confirm;
    while (true)
    {
        std::cin >> confirm;
        if (confirm == 'y' || confirm == 'n')
            break;
    }

    if (confirm == 'y')
    {
        PracticeEvent practice;
        practice.set_subject_id(subject_id);
        practice.set_duration(duration_ms / 1000);
        practice.set_description(name);
        practice.set_date(std::time(nullptr));

        rst_code_e rst = op->practice_event_add_recorded(temp_file, practice);
        if (rst)
        {
            out << "Operation error: " << get_rst_txt(rst) << std::endl;
        }
        else
        {
            out << "Practice event recorded added" << std::endl;
            out << UtilsPrints::get_practice_event_header() << std::endl;
            out << UtilsPrints::get_practice_event_body(practice) << std::endl;
            if (std::filesystem::exists(temp_file))
            {
                std::filesystem::remove(temp_file);
            }
        }
    }
    else
    {
        std::filesystem::remove(temp_file);
        out << "Recording discarded." << std::endl;
    }
}

void TerminalSession::practice_event_play(std::ostream &out, unsigned int id)
{
    std::shared_ptr<PracticeEvent> practice;
    rst_code_e rst = op->practice_event_get_by_id(id, practice);
    if (rst != RST_OK)
    {
        out << "Operation error: " << get_rst_txt(rst) << std::endl;
        return;
    }

    out << UtilsPrints::get_practice_event_header() << std::endl;
    out << UtilsPrints::get_practice_event_body(*practice) << std::endl;

    std::string file_path = practice->get_filepath();
    if (file_path.empty())
    {
        out << "No recording file associated." << std::endl;
        return;
    }

    if (!std::filesystem::exists(file_path))
    {
        out << "File not found: " << file_path << std::endl;
        return;
    }

    std::atomic<bool> running{true};

    auto callback = [&](ISoundSystem::PlaybackEvent event, unsigned int timestamp) {
        if (event == ISoundSystem::PlaybackEvent::PLAY_TIMESTAMP) {
            unsigned long long s = timestamp / 1000;
            int mm = s / 60;
            int ss = s % 60;
            out << "\rTime: "
                << std::setw(2) << std::setfill('0') << mm << ":"
                << std::setw(2) << std::setfill('0') << ss
                << std::flush;
        } else if (event == ISoundSystem::PlaybackEvent::PLAY_END || event == ISoundSystem::PlaybackEvent::PLAY_ERROR) {
            running = false;
        }
    };

    rst_code_e rst_play = op->audio_play(file_path, callback);
    if (rst_play != RST_OK)
    {
        out << "Error: Could not start playback: " << get_rst_txt(rst_play) << std::endl;
        return;
    }

    out << "Playing... Press 'q' to stop." << std::endl;

    // Run input in a separate thread so it doesn't block the UI updates or the exit condition
    std::thread input_thread([this, &running]() {
        char input;
        while (running) {
            std::cin >> input;
            if (input == 'q') {
                op->audio_stop_playing();
                running = false;
                break;
            }
        }
    });

    while (running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Detach because we can't easily cancel the blocking std::cin in the thread if playback ends naturally
    input_thread.detach();

    out << std::endl << "Playback finished." << std::endl;
}
