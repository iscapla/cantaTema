#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstdint>

#include "operations/operation_practice_event.hpp"
#include "operations/operation_subject.hpp"
#include "operations/operation_category.hpp"
#include "operations/operation_user.hpp"
#include "operations/operation_user_metrics.hpp"
#include "primitives/practice_event.hpp"
#include "primitives/subject.hpp"
#include "primitives/category.hpp"
#include "primitives/user.hpp"

class OperationPracticeEventTest : public ::testing::Test {
protected:
    std::unique_ptr<OperationPracticeEvent> operation_practice;
    std::shared_ptr<OperationSubject> operation_subject;
    std::shared_ptr<OperationCategory> operation_category;
    std::unique_ptr<OperationUser> operation_user;
    std::shared_ptr<OperationUserMetrics> operation_metrics;

    std::shared_ptr<const User> test_user;
    std::shared_ptr<Category> test_category;
    std::shared_ptr<Subject> test_subject;
    const std::string dummy_file_name = "test_audio_practice.wav";
    const std::string dummy_subject_file_name = "test_subject_practice.txt";

    void SetUp() override {
        // Create dummy file for recorded events
        // Create a valid WAV file with ~1 second duration so SoundFileHandler can read metadata
        std::ofstream outfile(dummy_file_name, std::ios::binary);
        
        // RIFF header
        outfile.write("RIFF", 4);
        int32_t chunk_size = 36 + 44100 * 2; // 36 + SubChunk2Size
        outfile.write(reinterpret_cast<const char*>(&chunk_size), 4);
        outfile.write("WAVE", 4);
        
        // fmt subchunk
        outfile.write("fmt ", 4);
        int32_t sub_chunk1_size = 16;
        outfile.write(reinterpret_cast<const char*>(&sub_chunk1_size), 4);
        int16_t audio_format = 1; // PCM
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
        
        // data subchunk
        outfile.write("data", 4);
        int32_t sub_chunk2_size = 44100 * 2; // 1 second of audio
        outfile.write(reinterpret_cast<const char*>(&sub_chunk2_size), 4);
        
        // Data (silence)
        std::vector<char> data(sub_chunk2_size, 0);
        outfile.write(data.data(), sub_chunk2_size);
        
        outfile.close();

        // Create dummy text file for subject
        std::ofstream text_outfile(dummy_subject_file_name);
        text_outfile << "dummy subject content";
        text_outfile.close();

        // Initialize dependencies
        operation_metrics = std::make_shared<OperationUserMetrics>();
        operation_category = std::make_shared<OperationCategory>();
        
        // Subject needs metrics and category
        std::shared_ptr<IOperationUserMetrics> metrics_for_subject = operation_metrics;
        std::shared_ptr<IOperationCategory> category_for_subject = operation_category;
        
        operation_subject = std::make_shared<OperationSubject>(
            std::move(metrics_for_subject),
            std::move(category_for_subject)
        );

        // User needs metrics
        std::shared_ptr<IOperationUserMetrics> metrics_for_user = operation_metrics;
        operation_user = std::make_unique<OperationUser>(std::move(metrics_for_user));

        // Practice needs metrics and subject
        std::shared_ptr<IOperationUserMetrics> metrics_for_practice = operation_metrics;
        std::shared_ptr<IOperationSubject> subject_for_practice = operation_subject;
        
        operation_practice = std::make_unique<OperationPracticeEvent>(
            std::move(metrics_for_practice),
            std::move(subject_for_practice)
        );

        // Create and Identify User
        std::string username = "PracticeTestUser";
        std::string password = "password";
        if (operation_user->user_identify(username, password) == RST_OK) {
            operation_user->user_remove();
        }
        ASSERT_EQ(operation_user->user_add(username, password), RST_OK);
        ASSERT_EQ(operation_user->user_identify(username, password), RST_OK);
        ASSERT_EQ(operation_user->user_get(test_user), RST_OK);

        // Create Category
        Category cat(0, "Practice Category");
        ASSERT_EQ(operation_category->category_add(test_user, cat), RST_OK);
        std::vector<std::shared_ptr<Category>> categories;
        operation_category->category_get_all_by_user(test_user, categories);
        ASSERT_FALSE(categories.empty());
        test_category = categories.front();

        // Create Subject
        Subject sub(0, "Practice Subject");
        sub.set_category_id(test_category->get_id());
        ASSERT_EQ(operation_subject->subject_add(test_user, dummy_subject_file_name, sub), RST_OK);
        
        std::vector<std::shared_ptr<Subject>> subjects;
        operation_subject->subject_get_all_by_category(test_user, test_category->get_id(), subjects);
        ASSERT_FALSE(subjects.empty());
        test_subject = subjects.front();
    }

    void TearDown() override {
        std::remove(dummy_file_name.c_str());
        std::remove(dummy_subject_file_name.c_str());
        
        operation_practice.reset();
        operation_subject.reset();
        operation_category.reset();
        if (operation_user) {
            operation_user->user_remove();
            operation_user.reset();
        }
        operation_metrics.reset();
    }
};

TEST_F(OperationPracticeEventTest, AddPlannedEvent) {
    PracticeEvent event;
    event.set_subject_id(test_subject->get_id());
    event.set_description("Planned practice");
    event.set_duration(60);

    ASSERT_EQ(operation_practice->practice_event_add_planned(test_user, event), RST_OK);
    EXPECT_GT(event.get_id(), 0u);
    EXPECT_EQ(event.get_status(), PracticeEvent::PLANNED);
}

TEST_F(OperationPracticeEventTest, AddRecordedEvent) {
    PracticeEvent event;
    event.set_subject_id(test_subject->get_id());
    event.set_description("Recorded practice");
    event.set_duration(120);

    ASSERT_EQ(operation_practice->practice_event_add_recorded(test_user, dummy_file_name, event), RST_OK);
    EXPECT_GT(event.get_id(), 0u);
    EXPECT_EQ(event.get_status(), PracticeEvent::RECORDED);
    EXPECT_FALSE(event.get_filepath().empty());
}

TEST_F(OperationPracticeEventTest, UpdateEvent) {
    PracticeEvent event;
    event.set_subject_id(test_subject->get_id());
    ASSERT_EQ(operation_practice->practice_event_add_planned(test_user, event), RST_OK);

    // Fetch fresh object to ensure we have all system-set fields before updating
    std::shared_ptr<PracticeEvent> fetched;
    ASSERT_EQ(operation_practice->practice_event_get_by_id(test_user, event.get_id(), fetched), RST_OK);
    ASSERT_NE(fetched, nullptr);
    
    fetched->set_description("Updated description");
    EXPECT_EQ(operation_practice->practice_event_update(test_user, *fetched), RST_OK);

    std::shared_ptr<PracticeEvent> refetched;
    operation_practice->practice_event_get_by_id(test_user, event.get_id(), refetched);
    ASSERT_NE(refetched, nullptr);
    EXPECT_EQ(refetched->get_description(), "Updated description");
}

TEST_F(OperationPracticeEventTest, RemoveEvent) {
    PracticeEvent event;
    event.set_subject_id(test_subject->get_id());
    ASSERT_EQ(operation_practice->practice_event_add_planned(test_user, event), RST_OK);

    EXPECT_EQ(operation_practice->practice_event_remove(test_user, event.get_id()), RST_OK);

    std::shared_ptr<PracticeEvent> fetched;
    operation_practice->practice_event_get_by_id(test_user, event.get_id(), fetched);
    EXPECT_EQ(fetched, nullptr);
}

TEST_F(OperationPracticeEventTest, GetAllBySubject) {
    PracticeEvent e1;
    e1.set_subject_id(test_subject->get_id());
    operation_practice->practice_event_add_planned(test_user, e1);

    PracticeEvent e2;
    e2.set_subject_id(test_subject->get_id());
    operation_practice->practice_event_add_planned(test_user, e2);

    std::vector<std::shared_ptr<PracticeEvent>> events;
    EXPECT_EQ(operation_practice->practice_event_get_all_by_subject(test_user, test_subject->get_id(), events), RST_OK);
    EXPECT_GE(events.size(), 2u);
}