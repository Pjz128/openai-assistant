//
// Created by Pjz128 on 23-11-22.
//

#ifndef ASSISTANT_MESSAGE_QUEUE_H
#define ASSISTANT_MESSAGE_QUEUE_H
#include <deque>
#include <memory>
#include <mutex>
#include <shared_mutex>

template <class MsgType>
class Queue {
public:
    void BackPop() {
        auto lock = WriteLock();
        deque_.pop_back();
    }

    void BackPush(const MsgType& msg) {
        auto lock = WriteLock();
        deque_.push_back(msg);
    }

    void FrontPush(const MsgType& msg) {
        auto lock = WriteLock();
        deque_.push_front(msg);
    }

    void FrontPop() {
        auto lock = WriteLock();
        deque_.pop_front();
    }

    void Clear() {
        auto lock = WriteLock();
        deque_.clear();
    }

    MsgType Front() {
        auto lock = ReadLock();
        return deque_.front();
    }

    size_t Size() {
        auto lock = ReadLock();
        return deque_.size();
    }

    bool Empty() {
        auto lock = ReadLock();
        return deque_.empty();
    }

    inline std::unique_lock<std::shared_timed_mutex> WriteLock() {
        return std::unique_lock<std::shared_timed_mutex>(st_mutex_);
    }
    inline std::shared_lock<std::shared_timed_mutex> ReadLock() {
        return std::shared_lock<std::shared_timed_mutex>(st_mutex_);
    }

    std::shared_timed_mutex st_mutex_;

    static std::deque<MsgType> deque_;

};  // class Queue

template <class MsgType>
std::deque<MsgType> Queue<MsgType>::deque_;

#endif //ASSISTANT_MESSAGE_QUEUE_H
