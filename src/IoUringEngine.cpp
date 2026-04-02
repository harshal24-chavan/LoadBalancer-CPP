#include "IoUringEngine.hpp"
#include <iostream>
#include <liburing.h>
#include <memory>
#include <string_view>
#include <sys/socket.h>

#define MAX_BUFFER_SIZE 8192
#define MAX_CONNECTIONS 10000

enum class EventType {
  ACCEPTING,
  READING_CLIENT_REQ,

  WRITING_CLIENT_REQ_TO_BACKEND,
  READING_BACKEND_RESP,

  WRITING_BACKEND_HEADERS_TO_CLIENT,

  // zero copy of req/resp body
  SPLICING_REQ_TO_PIPE,    // backend to pipe
  SPLICING_RESP_TO_CLIENT, // pipe to client

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

  void add_accept(int listen_fd) {
    auto *sqe = io_uring_get_sqe(&ring);
    if (!listenerCtx) {
      listenerCtx =
          std::make_unique<RequestData>(EventType::ACCEPTING, listen_fd);
    }

    io_uring_prep_multishot_accept_direct(sqe, listen_fd, nullptr, nullptr, 0);
    sqe->file_index = IORING_FILE_INDEX_ALLOC;

    io_uring_sqe_set_data(sqe, listener_ctx.get());
  }

  void add_read(int client_direct_index, RequestData *req) {
    auto *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_recv(sqe, client_direct_index, req->buffer, MAX_BUFFER_SIZE,
                       0);
    sqe->flags |= IOSQE_FIXED_FILE;
    io_uring_sqe_set_data(sqe, req);
  }

  void close_client_direct(int fd) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);

    // Tell the kernel to empty this specific lock-free array slot
    io_uring_prep_close_direct(sqe, fd);

    // We pass nullptr because we don't need anything to track a closure.
    // We just fire and forget!
    io_uring_sqe_set_data(sqe, nullptr);
  }
  void terminate_connection(RequestData *req, BackendPool *pool) {
    // 1. Safely return the backend to the pool (if we had one)
    if (req->backend_direct_fd != -1 && req->server_id != -1) {
      pool->return_index(req->server_id, req->backend_direct_fd);
      req->backend_direct_fd = -1; // Prevent double-returns
    }

    // 2. Tell io_uring to kill the client socket (Fire and forget!)
    close_client_direct(req->direct_fd_index);

    // Note: We don't need to manually close the pipes here because
    // the ~RequestData() destructor will automatically close them for us!
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

        if(res < 0){
          if (!(cqe->flags & IORING_CQE_F_MORE)) {
            pimpl->add_accept(pimpl->server_fd);
            io_uring_submit(&pimpl->ring);
          }
          break;
        }

        int client_direct_index = res;
        auto new_client_req = std::make_unique<RequestData>(
            EventType::READING_CLIENT_REQ, client_direct_index);

        pimpl->add_read(client_direct_index, new_client_req.get());
        new_client_req.release();

        if (!(cqe->flags & IORING_CQE_F_MORE)) {
          pimpl->add_accept(pimpl->server_fd);
        }
        io_uring_submit(&pimpl->ring);
        break;
      }
      case EventType::READING_CLIENT_REQ{
        if(res <= 0){
          pimpl->terminate_connection(req.get(), pool.get());
          break;
        }

        req->total_bytes_read += res;

        std::string_view window(req->buffer, req->total_bytes_read);

        size_t boundry = window.find("\r\n\r\n");

        if(bound != std::string_view::npos){
          // found complete headers now forward the request to backendserver
          req->server_id = 0;
          req->backend_direct_fd = pool->checkout(req->server_id);

          if (req->backend_direct_fd == -1) {
            pimpl->terminate_connection(req.get(), pool.get());
            break; // FIX: Drop client safely if pool is completely empty
          }

          req->type = EventType::WRITING_CLIENT_REQ_TO_BACKEND;
          pimpl->forward_to_backend(req.get());
          
          req.release();
          io_uring_submit(&pimpl->ring);
        } else{
          // header not finished read next chunk
          auto *sqe = io_uring_get_sqe(&ring);
          io_uring_prep_recv(sqe, req->direct_fd_index, 
              req->buffer + req->total_bytes_read, // Pointer math!
              MAX_BUFFER_SIZE - req->total_bytes_read, 
              0);
          io_uring_sqe_set_data(sqe, req.release());
        }

        break;
      }
      case EventType::WRITING_CLIENT_REQ_TO_BACKEND{
        if(res < 0){
          pimpl->terminate_connection(req.get(), pool.get());
          break;
        }
        
        req->type = EventType::READING_BACKEND_RESP;
        pimpl->add_read(req->backend_direct_fd, req.get());
        io_uring_submit(&pimpl->ring);
        req.release();
        break;
      }
      case EventType::
    }
  }
}
