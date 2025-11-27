#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace hangman {

// Forward declaration
class Task;
using TaskPtr = std::shared_ptr<Task>;

class TaskQueue {
public:
    TaskQueue() = default;
    ~TaskQueue() = default;

    // Push a task to the queue (thread-safe)
    void push(TaskPtr task);

    // Pop a task from the queue (blocks if empty, unless stopped)
    TaskPtr pop();

    // Signal that no more tasks will be added
    void stop();

    // Check if queue is empty
    bool empty() const;

    // Get queue size
    size_t size() const;

private:
    mutable std::mutex mutex;
    std::condition_variable cv;
    std::queue<TaskPtr> queue;
    bool stopped = false;
};

} // namespace hangman
