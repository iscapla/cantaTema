#include <gtest/gtest.h>
#include <primitives/utils_thread_pool.hpp>
#include <atomic>
#include <chrono>
#include <thread>

namespace {

class ThreadPoolTest : public ::testing::Test {
protected:
    // No specific setup/teardown needed for basic tests, 
    // but we can ensure clean destruction via RAII in tests.
};

TEST_F(ThreadPoolTest, SimpleTaskExecution) {
    ThreadPool pool(2);
    pool.threads_initialize();

    auto future = pool.submit([]() { return 42; });
    
    // Wait for result
    ASSERT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, MultipleTasksExecution) {
    ThreadPool pool(4);
    pool.threads_initialize();
    
    std::atomic<int> counter{0};
    const int num_tasks = 100;

    for (int i = 0; i < num_tasks; ++i) {
        pool.submit([&counter]() {
            counter++;
        });
    }

    pool.waitForCompletion();
    EXPECT_EQ(counter.load(), num_tasks);
}

TEST_F(ThreadPoolTest, WaitForCompletionBlocks) {
    ThreadPool pool(2);
    pool.threads_initialize();
    
    std::atomic<bool> flag{false};

    pool.submit([&flag]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        flag = true;
    });

    // Should block until the task above sets flag to true and finishes
    pool.waitForCompletion();
    
    EXPECT_TRUE(flag.load());
}

TEST_F(ThreadPoolTest, SubmitOnStoppedPoolThrows) {
    ThreadPool pool(1);
    pool.threads_initialize();
    
    // Manually destroy to stop the pool
    pool.threads_destroy();

    // Attempting to submit to a stopped pool should throw
    EXPECT_THROW({
        pool.submit([](){});
    }, std::runtime_error);
}

} // namespace