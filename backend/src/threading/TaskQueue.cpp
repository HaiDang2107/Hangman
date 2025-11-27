#include "threading/TaskQueue.h"
#include "threading/Task.h"

namespace hangman {

void TaskQueue::push(TaskPtr task) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopped) {
            return;  // Ignore tasks after stop
        }
        queue.push(task);
    }
    cv.notify_one();
}

TaskPtr TaskQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex);
    
    // Wait until queue has elements or is stopped
    cv.wait(lock, [this] {
        return !queue.empty() || stopped;
    });

    if (queue.empty()) {
        return nullptr;  // Stopped and no more tasks
    }

    TaskPtr task = queue.front();
    queue.pop();
    return task;
}

void TaskQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        stopped = true;
    }
    cv.notify_all();
}

bool TaskQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.empty();
}

size_t TaskQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.size();
}

} // namespace hangman
