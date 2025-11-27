// include/network/EventLoop.h
#pragma once
#include <functional>
#include <map>
#include <atomic>
#include <cstdint>

class EventLoop {
public:
    using EventCallback = std::function<void()>;
    
    // Event types
    static constexpr uint32_t EVENT_READ = 1;
    static constexpr uint32_t EVENT_WRITE = 2;
    
    EventLoop();
    ~EventLoop();
    
    // Đăng ký fd với callback (events = EVENT_READ | EVENT_WRITE | ...)
    void addFd(int fd, EventCallback callback, uint32_t events = EVENT_READ);
    void removeFd(int fd);
    void modifyFd(int fd, uint32_t events);
    
    // Chạy event loop
    void run();
    void stop();
    
private:
    int epollFd;
    std::atomic<bool> running;
    std::map<int, EventCallback> callbacks;
    
    static constexpr int MAX_EVENTS = 64;
};