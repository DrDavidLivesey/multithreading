#pragma once

#include <string_view>
#include <stop_token>
#include <utility>
#include <format>
#include <atomic>
#include <string>
#include <thread>

extern void (*print)(std::string_view message);

class logger {
  private:
    struct node {
      std::string message;
      std::atomic<node*> next{nullptr};

      explicit node(std::string_view msg) : message(msg) {}
    };
   
    node* head_;
    std::atomic<node*> tail_;

  public:
    logger() : head_(new node({})), tail_(head_) {}

    ~logger() {
      while (head_ != nullptr) {
        node* next = head_->next.load(std::memory_order_relaxed);
        delete head_;
        head_ = next;
      }
    }

    /// Called on many threads. Must queue the message.
    /// Should return as soon as possible and rarely block.
    void post(std::string_view message) {
      node* new_node = new node(message);
      node* prev = tail_.exchange(new_node, std::memory_order_acq_rel);
      prev->next.store(new_node, std::memory_order_release);
    }

    /// Called on many threads. Must format and queue the message.
    /// Should return as soon as possible and rarely block.
    template <class... Args>
    void post(std::format_string<Args...> fmt, Args&&... args) {
      post(std::format(fmt, std::forward<Args>(args)...));
    }

    /// Called on one thread. Must call `print` once per message.
    void run(std::stop_token stop) 
    {
      while (!stop.stop_requested()) {
        node* next = head_->next.load(std::memory_order_acquire);

        if (next == nullptr) {
          std::this_thread::yield();
          continue;
        }

        print(next->message);

        delete head_;
        head_ = next;
      }

      for (;;) {
        node* next = head_->next.load(std::memory_order_acquire);

        if (next == nullptr) {
          break;
        }

        print(next->message);

        delete head_;
        head_ = next;
      }
    }
};
