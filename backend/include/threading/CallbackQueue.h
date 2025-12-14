#pragma once

#include <queue>
#include <mutex>
#include <memory>
#include <functional>
#include <vector>

// Worker Thread: Xử lý xong Task 
// → Tạo FunctionCallback chứa logic gửi phản hồi 
// → Gọi callbackQueue->push()

// CallbackQueue: Đẩy vào hàng đợi 
// → Ghi vào notifyFd.

// Network Thread: Đang chờ ở epoll_wait 
// → Bị đánh thức bởi notifyFd 
// → Gọi callbackQueue->popAll() 
// → Chạy callback->execute() để gửi dữ liệu cho client.

namespace hangman {

// Callback interface - used to pass results back from worker to network thread
class Callback {
public:
    virtual ~Callback() = default;
    virtual void execute() = 0;
};

using CallbackPtr = std::shared_ptr<Callback>;

// Simple callback wrapper for std::function
class FunctionCallback : public Callback {
public:
    explicit FunctionCallback(std::function<void()> func) : func(func) {}
    void execute() override {
        if (func) {
            func();
        }
    }

private:
    std::function<void()> func;
};

class CallbackQueue {
public:
    CallbackQueue();
    ~CallbackQueue();

    // Push callback from worker thread
    void push(CallbackPtr callback);

    // Pop all pending callbacks (network thread only)
    std::vector<CallbackPtr> popAll();

    // Get notification fd for epoll integration
    int getNotificationFd() const { return notifyFd; }

    // Reset the notification after processing
    void resetNotification();

private:
    mutable std::mutex mutex;
    std::queue<CallbackPtr> queue;
    int notifyFd;  // eventfd for epoll notification
};

} // namespace hangman
