#include <logger.hpp>
#include <mutex>
#include <cstdio>
#include <cassert>
#include <vector>

static std::mutex mutex;
std::vector<std::string> messages;

void (*print)(std::string_view message) = [](std::string_view message) 
{
  std::lock_guard lock{ mutex };
  messages.emplace_back(message);
  std::fwrite(message.data(), sizeof(char), message.size(), stdout);
  std::fputc('\n', stdout);
  std::fflush(stdout);
};

int main(int argc, char* argv[]) 
{
  logger log; 

  {
    std::jthread worker{&logger::run, &log}; 
  
    log.post("hello");
    log.post("number: {}", 67);
    log.post("number: {}", 76);
  }

  assert(messages.size() == 3); 
  assert(messages[0] == "hello");
  assert(messages[1] == "number: 67");
  assert(messages[2] == "number: 76");
}
