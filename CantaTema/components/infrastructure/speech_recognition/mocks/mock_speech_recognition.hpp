#ifndef MOCK_SPEECH_RECOGNITION_HPP
#define MOCK_SPEECH_RECOGNITION_HPP

#include <gmock/gmock.h>
#include "speech_recognition/i_speech_recognition.hpp"

class MockSpeechRecognition : public ISpeechRecognition {
public:
    MOCK_METHOD(rst_code_e, initialize, (const speech_recognition_config_t& config), (override));
    MOCK_METHOD(rst_code_e, initialize, (const UserConfiguration& user_config), (override));
    MOCK_METHOD(rst_code_e, submit_task, (const std::string& audio_file_path), (override));
    MOCK_METHOD(speech_recognition_status_e, get_status, (), (override));
    MOCK_METHOD(rst_code_e, get_result, (std::string& text_document_path), (override));
    MOCK_METHOD(rst_code_e, get_segments, (std::vector<TranscriptSegment>& out_segments), (override));
};

#endif // MOCK_SPEECH_RECOGNITION_HPP
