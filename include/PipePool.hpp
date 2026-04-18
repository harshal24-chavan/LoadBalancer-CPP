#pragma once
#include <unistd.h> // For pipe2 and close
#include <vector>

struct PipePair {
  int read_fd;
  int write_fd;
};

class PipePool {
private:
  std::vector<PipePair> available_pipes;

public:
  void init(size_t pool_size) {
    for (size_t i = 0; i < pool_size; i++) {
      int pipeFD[2] = {-1, -1};
      if (pipe2(pipeFD, O_NONBLOCK | O_CLOEXEC) < 0) {
        std::cerr << "CRITICAL: Failed to allocate kernel pipe!\n";
        exit(1);
      }
    }
    std::cout << "Successfully pooled " << pool_size << " zero-copy pipes.\n";
  }

  PipePair getPipe() {
    if (available_pipes.empty()) {
      return {-1, -1};
    }

    PipePair p = available_pipe.back();
    available_pipes.pop_back();
    return p;
  }

  void returnPipe(int readFd, int writeFd) {
    available_pipes.push_back({readFd, writeFd});
  }

  ~PipePool {
    for (const PipePair &p : available_pipes) {
      close(p.read_fd);
      close(p.write_fd);
    }
  }
};
