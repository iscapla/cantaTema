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
#include "database/mocks/mock_database.hpp"
#include <memory>
#include <vector>
#include <filesystem>
#include <fstream>

using ::testing::Return;
using ::testing::_;

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

    // 5. User Metrics
    std::shared_ptr<const UserMetrics> metrics;
    EXPECT_EQ(session.user_metrics_get(metrics), RST_OK);
    EXPECT_NE(metrics, nullptr);

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

    // 7. Cleanup operations
    EXPECT_EQ(session.practice_event_remove(practice_id), RST_OK);
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

    std::shared_ptr<const UserMetrics> metrics;
    EXPECT_EQ(session.user_metrics_get(metrics), USER_NO_AUTH);

    PracticeEvent pe;
    EXPECT_EQ(session.practice_event_add_planned(pe), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_add_recorded("path.wav", pe), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_update(pe), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_remove(1), USER_NO_AUTH);
    std::shared_ptr<PracticeEvent> fpe;
    EXPECT_EQ(session.practice_event_get_by_id(1, fpe), USER_NO_AUTH);
    std::vector<std::shared_ptr<PracticeEvent>> pes;
    EXPECT_EQ(session.practice_event_get_by_subject(1, pes), USER_NO_AUTH);
    EXPECT_EQ(session.practice_event_get_by_user(pes), USER_NO_AUTH);

    std::string exec_id, list_json, report_json, config_json;
    EXPECT_EQ(session.analyze_practice_coverage(1, exec_id), USER_NO_AUTH);
    EXPECT_EQ(session.get_analysis_executions_for_practice(1, list_json), USER_NO_AUTH);
    EXPECT_EQ(session.get_analysis_execution_details("exec-123", report_json, config_json), USER_NO_AUTH);
}

TEST_F(SessionTest, EdgeAndErrorCases) {
    Session session;
    std::string name = "session_edge_user";
    std::string pass = "pass123";
    ASSERT_EQ(session.user_add(name, pass), RST_OK);
    ASSERT_EQ(session.user_identify(name, pass), RST_OK);

    // 1. Category update/remove not found
    EXPECT_EQ(session.category_update(9999, "New Name"), CATEGORY_NOT_FOUND);
    EXPECT_EQ(session.category_remove(9999), CATEGORY_NOT_FOUND);

    // 2. Subject update / language set not found
    EXPECT_EQ(session.subject_update(9999, "New Name", 1, "some_path.pdf"), SUBJECT_NOT_FOUND);
    EXPECT_EQ(session.set_subject_language(9999, "es"), SUBJECT_NOT_FOUND);

    // 3. Subject get by category not found
    std::vector<std::shared_ptr<Subject>> subjects;
    EXPECT_EQ(session.subject_get_by_category(9999, subjects), CATEGORY_NOT_FOUND);

    // 4. Practice event get for historic executions not found
    std::string list_json;
    EXPECT_EQ(session.get_analysis_executions_for_practice(9999, list_json), PRACTICE_EVENT_NOT_FOUND);

    // Clean up user
    EXPECT_EQ(session.user_remove(), RST_OK);
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



