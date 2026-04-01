#include "IoUringEngine.hpp"
#include <iostream>
#include <liburing.h>
#include <memory>
#include <sys/socket.h>

#define MAX_BUFFER_SIZE 4096
#define MAX_CONNECTIONS 10000

enum class EventType {
  ACCEPTING,
  READING_CLIENT_REQ,

  WRITING_CLIENT_REQ_TO_BACKEND,
  READING_BACKEND_RESP,

  WRITING_BACKEND_RESP_TO_CLIENT,

  // zero copy of req/resp body
  SPLICING_REQ_TO_BACKEND,
  SPLICING_RESP_TO_CLIENT,

  CLOSING_CLIENT_CONNECTION
};

struct RequestData {
  EventType type;

  int client_fd_index; // index of the array in which
                       // we have registered the files clients connection

  int backend_fd_index = -1; // backend direct_fd index
  int server_id = -1;        // which server to route the request to?

  char *buffer[MAX_BUFFER_SIZE];

  sockaddr_in client_addr{};
  socklen_t client_addr_len = sizeof(client_addr);

  RequestData(EventType t, int fd) : type(t), client_fd_index(fd) {};
};

struct IoUringEngine::Impl {
  std::vector<int> direct_descriptors;

  std::unique_ptr<RequestData> listenerCtx;
  struct io_uring ring;
  int server_fd = -1;

  add_accept(int listen_fd) {
    auto *sqe = io_uring_get_sqe(&ring);
    if (!listenerCtx) {
      listenerCtx =
          std::make_unique<RequestData>(EventType::ACCEPTING, listen_fd);
    }

    io_uring_prep_multishot_accept_direct(sqe, listen_fd, nullptr, nullptr, 0);
    sqe->file_index = IORING_FILE_INDEX_ALLOC;
    io_uring_sqe_set_data(sqe, listener_ctx.get());
  }
};

IoUringEngine::IoUringEngine(int port, int queue_depth)
    : pimpl(std::make_unique<Impl>()) {
  pimpl->server_fd = socket(AF_INET, SOCK_STREAM, 0);

  int opt = 1;
  setsockopt(pimpl->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  // since multiple threads will listen on same 8080 hencse reuseport
  setsockopt(pimpl->server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

  sockaddr_in address{};
  address.sin_port = htons(port);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;

  bind(pimpl->server_fd, (struct sockaddr *)&address, sizeof(address));
  listen(pimpl->server_fd, MAX_CONNECTIONS);

  io_uring_queue_init(queue_depth, &pimpl->ring, 0);
  pimpl->direct_descriptors.resize(MAX_CONNECTIONS, -1);
  int ret = io_uring_register_files(
      &pimpl->ring, pimpl->direct_descriptors.data(), MAX_CONNECTIONS);
  if (ret < 0) {
    std::cerr << "Failed to register fixed files\n";
    exit(1);
  }

  int serverCount = 3;
  int connectionsPerServer = 50;
  pool = std::make_unique<BackendPool>();
  pool->init(serverCount,
             connectionsPerServer); // 1 server, 50 connections for testing

  for (size_t serverId = 0; serverId < serverCount; serverId++) {
    for (size_t x = 0; x < connectionsPerServer; x++) {
      int raw_fd = socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in backendAddr{};
      backendAddr.sin_port = htons(8081 + serverId);
      backendAddr.sin_family = AF_INET;
      inet_pton(AF_INET, "10.0.0.2", &backendAddr.sin_addr);

      connect(raw_fd, (struct sockaddr *)&backendAddr, sizeof(backendAddr));

      int chosen_slot = 9000 + (serverId * 50) + x;
      io_uring_register_files_update(&pimpl->ring, chosen_slot, &raw_fd, 1);
      close(raw_fd);
      pool->return_index(serverId, chosen_slot);
    }
  }
}

IoUringEngine::~IoUringEngine() {
  if (pimpl->server_fd != -1)
    close(pimpl->server_fd);
  io_uring_queue_exit(&pimpl->ring);
}

void IoUringEngine::run() {
  pimpl->add_accept(pimpl->server_fd);
  while (true) {
    io_uring_submit(&pimpl->ring);

    struct io_uring_cqe *cqe;
    io_uring_wait_cqe(&pimpl->ring, &cqe);

    RequestData *raw_req =
        static_cast<RequestData *>(io_uring_cqe_get_data(cqe));
    std::unique_ptr<RequestData> req;

    if (raw_req == nullptr) {
      io_uring_cqe_seen(&pimpl->ring, cqe);
      continue; // It was just a close event finishing. Ignore it!
    }

    if (raw_req->type != EventType::ACCEPTING) {
      req.reset(raw_req);
    }

    auto res = cqe->res;
    io_uring_cqe_seen(&pimpl->ring, cqe);

    switch (raw_req->type) {
      case EventType::ACCEPTING{
      }
    }
  }
}
