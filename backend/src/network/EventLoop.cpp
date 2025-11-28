// src/network/EventLoop.cpp
#include "network/EventLoop.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

EventLoop::EventLoop() : running(false)
{
    epollFd = epoll_create1(0); // Create epoll instance (a table to monitor fds)
    if (epollFd < 0)
    {
        throw std::runtime_error("Failed to create epoll");
    }
}

EventLoop::~EventLoop()
{
    if (epollFd >= 0)
    {
        close(epollFd);
    }
}

// ASK EPOLL TO MONITOR A SOCKET
void EventLoop::addFd(int fd, EventCallback callback, uint32_t events)
{
    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));

    // Convert our event flags to epoll flags
    ev.events = EPOLLET; // Edge-triggered by default
    if (events & EVENT_READ)
    {
        ev.events |= EPOLLIN;
    }
    if (events & EVENT_WRITE)
    {
        ev.events |= EPOLLOUT;
    }

    ev.data.fd = fd;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        throw std::runtime_error("Failed to add fd to epoll");
    }

    callbacks[fd] = callback;
}

void EventLoop::removeFd(int fd)
{
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    callbacks.erase(fd);
}

void EventLoop::modifyFd(int fd, uint32_t events)
{
    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));

    ev.events = EPOLLET;
    if (events & EVENT_READ)
    {
        ev.events |= EPOLLIN;
    }
    if (events & EVENT_WRITE)
    {
        ev.events |= EPOLLOUT;
    }

    ev.data.fd = fd;

    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) < 0)
    {
        throw std::runtime_error("Failed to modify fd in epoll");
    }
}

void EventLoop::run()
{
    running = true;
    epoll_event events[MAX_EVENTS];

    while (running)
    {
        int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);

        if (nfds < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("epoll_wait failed");
        }

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            auto it = callbacks.find(fd);
            if (it != callbacks.end())
            {
                it->second(); // Gọi callback
            }
        }
    }
}

void EventLoop::stop()
{
    running = false;
}