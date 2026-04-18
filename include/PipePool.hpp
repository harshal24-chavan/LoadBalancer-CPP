#pragma once
#include <fcntl.h>  // For O_NONBLOCK and O_CLOEXEC
#include <unistd.h> // For pipe2 and close
#include <vector>

struct PipePair {
  int read_slot;
  int write_slot;
};

class PipePool {
private:
  std::vector<PipePair> available_pipes;

public:
  void init(size_t pool_size) { available_pipes.reserve(pool_size); }

  PipePair getPipe() {
    if (available_pipes.empty()) {
      return {-1, -1};
    }

    PipePair p = available_pipes.back();
    available_pipes.pop_back();
    return p;
  }

  void returnPipe(int readSlot, int writeSlot) {
    available_pipes.push_back({readSlot, writeSlot});
  }
};
