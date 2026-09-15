#pragma once

#include <condition_variable>
#include <format>
#include <mutex>
#include <queue>
#include <stop_token>
#include <string>
#include <string_view>

extern void (*print)(std::string_view message);

class logger {
  private:
    std::condition_variable_any cv_;
    std::queue<std::string> queue_;
    std::mutex mutex_;
  
  public:
    void post(std::string_view message) {
      {
        std::lock_guard lock{mutex_};
        queue_.emplace(message);
      }

      cv_.notify_one();
    }

    template <class... Args>
    void post(std::format_string<Args...> fmt, Args&&... args) {
      post(std::format(
        fmt,
        std::forward<Args>(args)...
      ));
    }

    void run(std::stop_token stop) {
      for (;;) {
        std::string message;

        {
          std::unique_lock lock{mutex_};

          cv_.wait(lock, stop, [&] {
            return !queue_.empty();
          });

          if (queue_.empty()) {
            return;
          }

          message = std::move(queue_.front());
          queue_.pop();
        }

        print(message);
      }
    }
};
