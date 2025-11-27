#include "threading/CallbackQueue.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

namespace hangman {

CallbackQueue::CallbackQueue() {
    // Create eventfd for epoll notification
    notifyFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (notifyFd < 0) {
        throw std::runtime_error("Failed to create eventfd");
    }
}

CallbackQueue::~CallbackQueue() {
    if (notifyFd >= 0) {
        ::close(notifyFd);
    }
}

void CallbackQueue::push(CallbackPtr callback) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(callback);
    }
    
    // Signal epoll that callbacks are available
    uint64_t value = 1;
    write(notifyFd, &value, sizeof(value));
}

std::vector<CallbackPtr> CallbackQueue::popAll() {
    std::vector<CallbackPtr> result;
    {
        std::lock_guard<std::mutex> lock(mutex);
        while (!queue.empty()) {
            result.push_back(queue.front());
            queue.pop();
        }
    }
    return result;
}

void CallbackQueue::resetNotification() {
    // Read from eventfd to reset it
    uint64_t value;
    read(notifyFd, &value, sizeof(value));
}

} // namespace hangman
