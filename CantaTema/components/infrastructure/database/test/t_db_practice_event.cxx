#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <memory>
#include <vector>

// Mocks
#include "mock_tool_paths.hpp"

// Components under test
#include "database/db_practice_event.hpp"
#include "database/db_subject.hpp"
#include "database/db_user.hpp"
#include "database/db_category.hpp"
#include "database/db_connection.hpp"
#include "primitives/practice_event.hpp"
#include "primitives/subject.hpp"
#include "primitives/user.hpp"
#include "primitives/category.hpp"

using ::testing::Return;
using ::testing::_;

class DBPracticeEventTest : public ::testing::Test {
protected:
    MockToolPath mockToolPath;
    std::filesystem::path temp_db_dir;

    void SetUp() override {
        // 1. Redirect ToolPath calls to our mock object
        g_mockToolPath = &mockToolPath;

        // 2. Create a unique temporary directory for this test run
        temp_db_dir = std::filesystem::temp_directory_path() / ("canta_tema_test_db_pe_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(temp_db_dir);

        // 3. Configure the mock to return our temp directory
        EXPECT_CALL(mockToolPath, get_path_for_database())
            .WillRepeatedly(Return(temp_db_dir));

        // 4. Ensure a fresh start by resetting any existing connection
        DB_Connection::reset_connection();

        // 5. Initialize tables required.
        // PracticeEvent depends on User and Subject. Subject depends on Category (optional) and User.
        DB_User db_user;
        ASSERT_EQ(db_user.user_tables_create(), RST_OK);

        DB_Category db_category;
        ASSERT_EQ(db_category.category_tables_create(), RST_OK);

        DB_Subject db_subject;
        ASSERT_EQ(db_subject.subject_tables_create(), RST_OK);

        DB_PracticeEvent db_pe;
        ASSERT_EQ(db_pe.practice_event_tables_create(), RST_OK);

        // Enable foreign keys for this connection to ensure ON DELETE CASCADE works during tests
        std::shared_ptr<sqlite3> db = DB_Connection::getConn();
        sqlite3_exec(db.get(), "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    }

    void TearDown() override {
        // 1. Close the database connection
        DB_Connection::reset_connection();

        // 2. Clean up the physical files
        if (std::filesystem::exists(temp_db_dir)) {
            std::filesystem::remove_all(temp_db_dir);
        }

        // 3. Reset the global mock pointer
        g_mockToolPath = nullptr;
    }

    // Helper to create a user and return its ID
    unsigned int create_test_user(const std::string& username) {
        DB_User db_user;
        User user(username);
        user.set_passwordkey("hashed_secret");
        user.set_roleid(1);
        user.set_creationdate(1234567890);
        user.set_status(User::Account_status::ACTIVE);
        
        if (db_user.add_new_user(user) == RST_OK) {
            return user.get_useraccountid();
        }
        return 0;
    }

    // Helper to create a subject and return its ID
    unsigned int create_test_subject(unsigned int user_id, const std::string& name) {
        DB_Subject db_subject;
        Subject subject(0, name);
        subject.set_user_id(user_id);
        
        if (db_subject.add_new_subject(subject) == RST_OK) {
            return subject.get_id();
        }
        return 0;
    }
};

TEST_F(DBPracticeEventTest, AddNewPracticeEvent_Success) {
    unsigned int user_id = create_test_user("pe_test_user_1");
    ASSERT_NE(user_id, 0);
    unsigned int sub_id = create_test_subject(user_id, "pe_test_sub_1");
    ASSERT_NE(sub_id, 0);

    DB_PracticeEvent db_pe;
    PracticeEvent event;
    event.set_status(PracticeEvent::PLANNED);
    event.set_user_id(user_id);
    event.set_subject_id(sub_id);
    event.set_date(100000);
    event.set_recorded_date(100050);
    event.set_duration(60);
    event.set_description("Test Description");
    event.set_filepath("/path/to/file");

    // Perform DB operation
    rst_code_e result = db_pe.add_new_practice_event(event);

    // Verify results
    EXPECT_EQ(result, RST_OK);
    EXPECT_NE(event.get_id(), 0) << "Event ID should be auto-generated";
}

TEST_F(DBPracticeEventTest, GetPracticeEventById_Success) {
    unsigned int user_id = create_test_user("pe_test_user_2");
    unsigned int sub_id = create_test_subject(user_id, "pe_test_sub_2");

    DB_PracticeEvent db_pe;
    PracticeEvent original_event;
    original_event.set_status(PracticeEvent::RECORDED);
    original_event.set_user_id(user_id);
    original_event.set_subject_id(sub_id);
    original_event.set_date(200000);
    original_event.set_recorded_date(200050);
    original_event.set_duration(120);
    original_event.set_description("Retrieval Desc");
    original_event.set_filepath("/path/retrieval");

    ASSERT_EQ(db_pe.add_new_practice_event(original_event), RST_OK);

    // Retrieve
    std::shared_ptr<PracticeEvent> retrieved_event;
    rst_code_e result = db_pe.get_practice_event_by_id(original_event.get_id(), retrieved_event);

    EXPECT_EQ(result, RST_OK);
    ASSERT_NE(retrieved_event, nullptr);
    EXPECT_EQ(retrieved_event->get_id(), original_event.get_id());
    EXPECT_EQ(retrieved_event->get_user_id(), user_id);
    EXPECT_EQ(retrieved_event->get_subject_id(), sub_id);
    EXPECT_EQ(retrieved_event->get_description(), "Retrieval Desc");
    EXPECT_EQ(retrieved_event->get_recorded_date(), 200050);
    EXPECT_EQ(retrieved_event->get_duration(), 120);
    EXPECT_EQ(retrieved_event->get_status(), PracticeEvent::RECORDED);
}

TEST_F(DBPracticeEventTest, UpdatePracticeEvent_Success) {
    unsigned int user_id = create_test_user("pe_test_user_3");
    unsigned int sub_id = create_test_subject(user_id, "pe_test_sub_3");

    DB_PracticeEvent db_pe;
    PracticeEvent event;
    event.set_status(PracticeEvent::PLANNED);
    event.set_user_id(user_id);
    event.set_subject_id(sub_id);
    event.set_date(300000);
    event.set_recorded_date(300050);
    event.set_duration(30);

    ASSERT_EQ(db_pe.add_new_practice_event(event), RST_OK);

    // Modify fields
    event.set_description("Updated Description");
    event.set_status(PracticeEvent::RECORDED);
    event.set_recorded_date(300060);
    event.set_duration(45);

    // Update
    rst_code_e result = db_pe.update_practice_event(event);
    EXPECT_EQ(result, RST_OK);

    // Verify
    std::shared_ptr<PracticeEvent> retrieved;
    ASSERT_EQ(db_pe.get_practice_event_by_id(event.get_id(), retrieved), RST_OK);
    EXPECT_EQ(retrieved->get_description(), "Updated Description");
    EXPECT_EQ(retrieved->get_status(), PracticeEvent::RECORDED);
    EXPECT_EQ(retrieved->get_recorded_date(), 300060);
    EXPECT_EQ(retrieved->get_duration(), 45);
}

TEST_F(DBPracticeEventTest, RemovePracticeEvent_Success) {
    unsigned int user_id = create_test_user("pe_test_user_4");
    unsigned int sub_id = create_test_subject(user_id, "pe_test_sub_4");

    DB_PracticeEvent db_pe;
    PracticeEvent event;
    event.set_status(PracticeEvent::PLANNED);
    event.set_user_id(user_id);
    event.set_subject_id(sub_id);
    event.set_date(400000);

    ASSERT_EQ(db_pe.add_new_practice_event(event), RST_OK);

    // Remove
    rst_code_e result = db_pe.remove_practice_event(event.get_id());
    EXPECT_EQ(result, RST_OK);

    // Verify it's gone
    std::shared_ptr<PracticeEvent> temp;
    EXPECT_NE(db_pe.get_practice_event_by_id(event.get_id(), temp), RST_OK);
}

TEST_F(DBPracticeEventTest, RemoveAllPracticeEventsFromUser_Success) {
    unsigned int user_id = create_test_user("pe_test_user_5");
    unsigned int sub_id = create_test_subject(user_id, "pe_test_sub_5");

    DB_PracticeEvent db_pe;
    
    PracticeEvent ev1;
    ev1.set_status(PracticeEvent::PLANNED);
    ev1.set_user_id(user_id);
    ev1.set_subject_id(sub_id);
    ev1.set_date(100);
    
    PracticeEvent ev2;
    ev2.set_status(PracticeEvent::PLANNED);
    ev2.set_user_id(user_id);
    ev2.set_subject_id(sub_id);
    ev2.set_date(200);

    ASSERT_EQ(db_pe.add_new_practice_event(ev1), RST_OK);
    ASSERT_EQ(db_pe.add_new_practice_event(ev2), RST_OK);

    // Remove all for user
    rst_code_e result = db_pe.remove_all_practice_events_by_user(user_id);
    EXPECT_EQ(result, RST_OK);

    // Verify
    std::vector<std::shared_ptr<PracticeEvent>> events;
    EXPECT_EQ(db_pe.get_all_practice_events_by_user(user_id, events), RST_OK);
    EXPECT_TRUE(events.empty());
}

TEST_F(DBPracticeEventTest, RemoveAllPracticeEventsFromSubject_Success) {
    unsigned int user_id = create_test_user("pe_test_user_6");
    unsigned int sub_id = create_test_subject(user_id, "pe_test_sub_6");

    DB_PracticeEvent db_pe;
    
    PracticeEvent ev1;
    ev1.set_status(PracticeEvent::PLANNED);
    ev1.set_user_id(user_id);
    ev1.set_subject_id(sub_id);
    ev1.set_date(100);
    
    PracticeEvent ev2;
    ev2.set_status(PracticeEvent::PLANNED);
    ev2.set_user_id(user_id);
    ev2.set_subject_id(sub_id);
    ev2.set_date(200);

    ASSERT_EQ(db_pe.add_new_practice_event(ev1), RST_OK);
    ASSERT_EQ(db_pe.add_new_practice_event(ev2), RST_OK);

    // Remove all for subject
    rst_code_e result = db_pe.remove_all_practice_events_by_subject(sub_id);
    EXPECT_EQ(result, RST_OK);

    // Verify
    std::vector<std::shared_ptr<PracticeEvent>> events;
    EXPECT_EQ(db_pe.get_all_practice_events_by_subject(sub_id, events), RST_OK);
    EXPECT_TRUE(events.empty());
}

TEST_F(DBPracticeEventTest, GetAllPracticeEventsByUser_Success) {
    unsigned int user_id = create_test_user("pe_test_user_7");
    unsigned int sub_id = create_test_subject(user_id, "pe_test_sub_7");

    DB_PracticeEvent db_pe;
    
    PracticeEvent ev1;
    ev1.set_status(PracticeEvent::PLANNED);
    ev1.set_user_id(user_id);
    ev1.set_subject_id(sub_id);
    ev1.set_date(100);
    
    PracticeEvent ev2;
    ev2.set_status(PracticeEvent::RECORDED);
    ev2.set_user_id(user_id);
    ev2.set_subject_id(sub_id);
    ev2.set_date(200);

    ASSERT_EQ(db_pe.add_new_practice_event(ev1), RST_OK);
    ASSERT_EQ(db_pe.add_new_practice_event(ev2), RST_OK);

    // Retrieve
    std::vector<std::shared_ptr<PracticeEvent>> events;
    rst_code_e result = db_pe.get_all_practice_events_by_user(user_id, events);
    
    EXPECT_EQ(result, RST_OK);
    EXPECT_EQ(events.size(), 2);
}

TEST_F(DBPracticeEventTest, CascadeDelete_Subject) {
    unsigned int user_id = create_test_user("pe_test_user_8");
    unsigned int sub_id = create_test_subject(user_id, "pe_test_sub_8");

    DB_PracticeEvent db_pe;
    PracticeEvent event;
    event.set_status(PracticeEvent::PLANNED);
    event.set_user_id(user_id);
    event.set_subject_id(sub_id);
    event.set_date(500);
    ASSERT_EQ(db_pe.add_new_practice_event(event), RST_OK);

    // Delete Subject
    DB_Subject db_subject;
    ASSERT_EQ(db_subject.remove_subject(sub_id), RST_OK);

    // Verify Event is gone (Cascade)
    std::shared_ptr<PracticeEvent> temp;
    EXPECT_NE(db_pe.get_practice_event_by_id(event.get_id(), temp), RST_OK);
}

TEST_F(DBPracticeEventTest, DBCloseDatabaseConnectionErrors) {
    DB_PracticeEvent db_pe;
    PracticeEvent ev;
    ev.set_user_id(1);
    ev.set_subject_id(1);

    sqlite3_close(DB_Connection::getConn().get());

    EXPECT_EQ(db_pe.practice_event_tables_create(), DB_FAIL);
    EXPECT_EQ(db_pe.add_new_practice_event(ev), DB_FAIL);
    EXPECT_EQ(db_pe.update_practice_event(ev), DB_FAIL);
    EXPECT_EQ(db_pe.remove_practice_event(1), DB_FAIL);
    EXPECT_EQ(db_pe.remove_all_practice_events_by_user(1), DB_FAIL);
    EXPECT_EQ(db_pe.remove_all_practice_events_by_subject(1), DB_FAIL);
    std::shared_ptr<PracticeEvent> retrieved;
    EXPECT_EQ(db_pe.get_practice_event_by_id(1, retrieved), DB_FAIL);
    std::vector<std::shared_ptr<PracticeEvent>> events;
    EXPECT_EQ(db_pe.get_all_practice_events_by_subject(1, events), RST_OK);
    EXPECT_EQ(db_pe.get_all_practice_events_by_user(1, events), RST_OK);

    DB_Connection::reset_connection();
    db_pe.practice_event_tables_create();
}

