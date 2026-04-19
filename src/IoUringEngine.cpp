#include "IoUringEngine.hpp"
#include "BackendPool.hpp"
#include "PipePool.hpp"

#include <charconv> // For std::from_chars
#include <cstring>  // For memset/memcpy
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h> // For O_NONBLOCK and O_CLOEXEC
#include <liburing.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h> // For pipe2 and close

#define MAX_BUFFER_SIZE 8192
#define MAX_CONNECTIONS 10000
#define MAX_PIPE_CONNECTIONS                                                   \
  1000 // i.e 2000 total pipes as read and write are 2 seperate pipes

enum class EventType {
  ACCEPTING,
  READING_CLIENT_REQ,
  WRITING_CLIENT_REQ_TO_BACKEND,
  READING_BACKEND_RESP,
  WRITING_BACKEND_HEADERS_TO_CLIENT,

  // Zero-Copy states
  SPLICING_REQ_TO_PIPE,    // Backend -> Pipe
  SPLICING_RESP_TO_CLIENT, // Pipe -> Client

  CLOSING_CLIENT_CONNECTION
};

struct RequestData {
  EventType type;

  // Standardized FD names
  int direct_fd_index;        // The Client's registered FD
  int backend_direct_fd = -1; // The Backend's registered FD
  int server_id = -1;

  int pipe_fds[2] = {-1, -1}; // The Zero-Copy Pipe

  char buffer[MAX_BUFFER_SIZE];
  size_t total_bytes_read = 0;
  size_t body_bytes_forwarded = 0;
  bool backend_header_parsed = false;
  size_t expected_content_length = 0;

  RequestData(EventType t, int fd) : type(t), direct_fd_index(fd) {}
};

struct IoUringEngine::Impl {
  std::vector<int> direct_descriptors;
  std::unique_ptr<RequestData> listener_ctx;
  struct io_uring ring;
  int server_fd = -1;

  void add_accept(int listen_fd) {
    auto *sqe = io_uring_get_sqe(&ring);
    if (!listener_ctx) {
      listener_ctx =
          std::make_unique<RequestData>(EventType::ACCEPTING, listen_fd);
    }

    io_uring_prep_multishot_accept_direct(sqe, listen_fd, nullptr, nullptr, 0);
    sqe->file_index = IORING_FILE_INDEX_ALLOC;
    io_uring_sqe_set_data(sqe, listener_ctx.get());
  }

  void add_read(int fd, RequestData *req) {
    auto *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_recv(sqe, fd, req->buffer, MAX_BUFFER_SIZE, 0);
    sqe->flags |= IOSQE_FIXED_FILE;
    io_uring_sqe_set_data(sqe, req);
  }

  void add_read_offset(int fd, char *custom_buffer, size_t size_to_read,
                       RequestData *req) {
    auto *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_recv(sqe, fd, custom_buffer, size_to_read, 0);
    sqe->flags |= IOSQE_FIXED_FILE;
    io_uring_sqe_set_data(sqe, req);
  }

  void add_write(int fd, const char *buf, size_t len, EventType next_event,
                 RequestData *req) {
    auto *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_send(sqe, fd, buf, len, 0);
    sqe->flags |= IOSQE_FIXED_FILE;
    req->type = next_event;
    io_uring_sqe_set_data(sqe, req);
  }

  void add_splice(int fd_in, int fd_out, size_t len, RequestData *req,
                  bool in_is_fixed, bool out_is_fixed) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    unsigned int splice_flags = SPLICE_F_MOVE | SPLICE_F_MORE;

    if (in_is_fixed) {
      splice_flags |= SPLICE_F_FD_IN_FIXED;
    }

    io_uring_prep_splice(sqe, fd_in, -1, fd_out, -1, len, splice_flags);

    if (out_is_fixed) {
      sqe->flags |= IOSQE_FIXED_FILE;
    }

    io_uring_sqe_set_data(sqe, req);
  }

  void close_client_direct(int fd) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_close_direct(sqe, fd);
    io_uring_sqe_set_data(sqe, nullptr);
  }

  void terminate_connection(RequestData *req, BackendPool *pool,
                            PipePool *pipePool) {
    if (req->backend_direct_fd != -1 && req->server_id != -1) {
      pool->returnConnection(req->server_id, req->backend_direct_fd);
      req->backend_direct_fd = -1;
    }
    if (req->pipe_fds[0] != -1 || req->pipe_fds[1] != -1) {
      pipePool->returnPipe(req->pipe_fds[0], req->pipe_fds[1]);
      req->pipe_fds[0] = -1;
      req->pipe_fds[1] = -1;
    }
    close_client_direct(req->direct_fd_index);
  }
};

IoUringEngine::IoUringEngine(
    std::function<int()> routing_callback,
    std::vector<std::pair<std::string, int>> &parsedServers, int port,
    int queue_depth)
    : pimpl(std::make_unique<Impl>()),
      get_next_server_callback(std::move(routing_callback)) {
  pimpl->server_fd = socket(AF_INET, SOCK_STREAM, 0);

  int opt = 1;
  setsockopt(pimpl->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  setsockopt(pimpl->server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

  sockaddr_in address{};
  address.sin_port = htons(port);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;

  bind(pimpl->server_fd, (struct sockaddr *)&address, sizeof(address));
  listen(pimpl->server_fd, MAX_CONNECTIONS);

  io_uring_queue_init(queue_depth, &pimpl->ring, 0);
  pimpl->direct_descriptors.resize(
      MAX_CONNECTIONS + (MAX_PIPE_CONNECTIONS * 2),
      -1); //*2 because there are 2 pipes read and write
  int ret =
      io_uring_register_files(&pimpl->ring, pimpl->direct_descriptors.data(),
                              pimpl->direct_descriptors.size());
  if (ret < 0) {
    std::cerr << "Failed to register fixed files\n";
    exit(1);
  }

  // making the backend pool connections
  int serverCount = parsedServers.size();
  int connectionsPerServer = 200;
  pool = std::make_unique<BackendPool>();
  pool->init(serverCount, connectionsPerServer);

  for (size_t serverId = 0; serverId < serverCount; serverId++) {

    const char *backend_host = parsedServers[serverId].first.c_str();
    int backend_port = parsedServers[serverId].second;

    for (size_t x = 0; x < connectionsPerServer; x++) {
      int raw_fd = socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in backendAddr{};
      backendAddr.sin_port = htons(backend_port);
      backendAddr.sin_family = AF_INET;

      inet_pton(AF_INET, backend_host, &backendAddr.sin_addr);

      connect(raw_fd, (struct sockaddr *)&backendAddr, sizeof(backendAddr));

      int chosen_slot = 9000 + (serverId * connectionsPerServer) + x;
      io_uring_register_files_update(&pimpl->ring, chosen_slot, &raw_fd, 1);
      close(raw_fd);
      pool->returnConnection(serverId, chosen_slot);
    }
  }

  // making and registering zero-copy kernel pipes with iouring
  pipePool = std::make_unique<PipePool>();
  pipePool->init(MAX_PIPE_CONNECTIONS);
  for (size_t pipeId = 0; pipeId < MAX_PIPE_CONNECTIONS; pipeId++) {
    int pipeFD[2] = {-1, -1};
    if (pipe2(pipeFD, O_NONBLOCK | O_CLOEXEC) < 0) {
      std::cerr << "CRITICAL: Failed to allocate kernel pipe!\n";
      exit(1);
    }

    int chosen_slot = 10000 + (pipeId * 2);
    io_uring_register_files_update(&pimpl->ring, chosen_slot, pipeFD, 2);

    close(pipeFD[0]);
    close(pipeFD[1]);

    pipePool->returnPipe(chosen_slot, chosen_slot + 1);
  }
  std::cout << "Successfully pooled " << MAX_PIPE_CONNECTIONS
            << " zero-copy pipes.\n";

  std::cout << "Running on port: " << port << std::endl;
}

IoUringEngine::~IoUringEngine() {
  if (pimpl->server_fd != -1)
    close(pimpl->server_fd);
  io_uring_queue_exit(&pimpl->ring);
}

void IoUringEngine::run() {
  pimpl->add_accept(pimpl->server_fd);

  std::cout << "Zero-Copy Load Balancer Listening\n";

  while (true) {
    io_uring_submit(&pimpl->ring);

    struct io_uring_cqe *cqe;
    io_uring_wait_cqe(&pimpl->ring, &cqe);

    RequestData *raw_req =
        static_cast<RequestData *>(io_uring_cqe_get_data(cqe));
    std::unique_ptr<RequestData> req;

    if (raw_req == nullptr) {
      io_uring_cqe_seen(&pimpl->ring, cqe);
      continue;
    }

    if (raw_req->type != EventType::ACCEPTING) {
      req.reset(raw_req);
    }

    auto res = cqe->res;
    io_uring_cqe_seen(&pimpl->ring, cqe);

    switch (raw_req->type) {
    case EventType::ACCEPTING: {
      if (res < 0) {
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

    case EventType::READING_CLIENT_REQ: {
      if (res <= 0) {
        pimpl->terminate_connection(req.get(), pool.get(), pipePool.get());
        break;
      }

      req->total_bytes_read += res;
      std::string_view window(req->buffer, req->total_bytes_read);
      size_t boundary = window.find("\r\n\r\n");

      if (boundary != std::string_view::npos) {
        req->server_id = get_next_server_callback();
        req->backend_direct_fd = pool->getServerConnection(req->server_id);

        PipePair p = pipePool->getPipe();
        req->pipe_fds[0] = p.read_slot;
        req->pipe_fds[1] = p.write_slot;

        if (req->backend_direct_fd == -1 || req->pipe_fds[0] == -1 ||
            req->pipe_fds[1] == -1) {
          pimpl->terminate_connection(req.get(), pool.get(), pipePool.get());
          break;
        }

        // Forward the exact parsed request to the backend
        pimpl->add_write(req->backend_direct_fd, req->buffer,
                         req->total_bytes_read,
                         EventType::WRITING_CLIENT_REQ_TO_BACKEND, req.get());

        req.release();
        io_uring_submit(&pimpl->ring);
      } else {
        // Read next chunk
        pimpl->add_read_offset(
            req->direct_fd_index, req->buffer + req->total_bytes_read,
            MAX_BUFFER_SIZE - req->total_bytes_read, req.get());
        req.release();
        io_uring_submit(&pimpl->ring);
      }
      break;
    }

    case EventType::WRITING_CLIENT_REQ_TO_BACKEND: {
      if (res < 0) {
        pimpl->terminate_connection(req.get(), pool.get(), pipePool.get());
        break;
      }
      // Reset bytes_read for the upcoming backend response
      req->total_bytes_read = 0;
      req->type = EventType::READING_BACKEND_RESP;
      pimpl->add_read(req->backend_direct_fd, req.get());
      io_uring_submit(&pimpl->ring);
      req.release();
      break;
    }

    case EventType::READING_BACKEND_RESP: {
      if (res <= 0) {
        pimpl->terminate_connection(req.get(), pool.get(), pipePool.get());
        break;
      }

      if (!req->backend_header_parsed) {
        req->total_bytes_read += res;
        std::string_view window(req->buffer, req->total_bytes_read);
        size_t boundary = window.find("\r\n\r\n");

        if (boundary != std::string_view::npos) {
          req->backend_header_parsed = true;
          size_t cl_pos = window.find("Content-Length: ");

          if (cl_pos != std::string_view::npos) {
            cl_pos += 16;
            size_t end_pos = window.find("\r", cl_pos);
            if (end_pos != std::string_view::npos) {
              std::string_view num_str =
                  window.substr(cl_pos, end_pos - cl_pos);
              std::from_chars(num_str.data(), num_str.data() + num_str.size(),
                              req->expected_content_length);
            }
          }

          size_t header_size = boundary + 4;
          req->body_bytes_forwarded = req->total_bytes_read - header_size;

          pimpl->add_write(
              req->direct_fd_index, req->buffer, req->total_bytes_read,
              EventType::WRITING_BACKEND_HEADERS_TO_CLIENT, req.get());
        } else {
          pimpl->add_read_offset(
              req->backend_direct_fd, req->buffer + req->total_bytes_read,
              MAX_BUFFER_SIZE - req->total_bytes_read, req.get());
        }
      } else {
        // Fallback if headers were parsed, but we are doing a normal copy
        req->body_bytes_forwarded += res;
        pimpl->add_write(req->direct_fd_index, req->buffer, res,
                         EventType::WRITING_BACKEND_HEADERS_TO_CLIENT,
                         req.get());
      }

      io_uring_submit(&pimpl->ring);
      req.release();
      break;
    }

    case EventType::WRITING_BACKEND_HEADERS_TO_CLIENT: {
      if (res < 0) {
        pimpl->terminate_connection(req.get(), pool.get(), pipePool.get());
        break;
      }

      if (req->backend_header_parsed) {
        if (req->body_bytes_forwarded >= req->expected_content_length) {
          pool->returnConnection(req->server_id, req->backend_direct_fd);
          pipePool->returnPipe(req->pipe_fds[0], req->pipe_fds[1]);

          req->total_bytes_read = 0;
          req->body_bytes_forwarded = 0;
          req->backend_header_parsed = false;
          req->expected_content_length = 0;
          req->pipe_fds[0] = -1;
          req->pipe_fds[1] = -1;

          req->type = EventType::READING_CLIENT_REQ;
          pimpl->add_read(req->direct_fd_index, req.get());
        } else {
          req->type = EventType::SPLICING_REQ_TO_PIPE;
          size_t bytes_to_forward =
              req->expected_content_length - req->body_bytes_forwarded;

          pimpl->add_splice(req->backend_direct_fd, req->pipe_fds[1],
                            bytes_to_forward, req.get(), true, true);
        }
      } else {
        req->type = EventType::READING_BACKEND_RESP;
        pimpl->add_read(req->backend_direct_fd, req.get());
      }

      io_uring_submit(&pimpl->ring);
      req.release();
      break;
    }

    case EventType::SPLICING_REQ_TO_PIPE: {
      if (res <= 0) {
        pimpl->terminate_connection(req.get(), pool.get(), pipePool.get());
        break;
      }

      req->type = EventType::SPLICING_RESP_TO_CLIENT;
      pimpl->add_splice(req->pipe_fds[0], req->direct_fd_index, res, req.get(),
                        true, true);

      io_uring_submit(&pimpl->ring);
      req.release();
      break;
    }

    case EventType::SPLICING_RESP_TO_CLIENT: {
      if (res <= 0) {
        pimpl->terminate_connection(req.get(), pool.get(), pipePool.get());
        break;
      }

      req->body_bytes_forwarded += res;

      if (req->body_bytes_forwarded >= req->expected_content_length) {
        pool->returnConnection(req->server_id, req->backend_direct_fd);
        pipePool->returnPipe(req->pipe_fds[0], req->pipe_fds[1]);

        // req->backend_direct_fd = -1;
        req->total_bytes_read = 0;
        req->body_bytes_forwarded = 0;
        req->backend_header_parsed = false;
        req->expected_content_length = 0;
        req->pipe_fds[0] = -1;
        req->pipe_fds[1] = -1;

        req->type = EventType::READING_CLIENT_REQ;
        pimpl->add_read(req->direct_fd_index, req.get());
      } else {
        req->type = EventType::SPLICING_REQ_TO_PIPE;
        size_t bytes_left =
            req->expected_content_length - req->body_bytes_forwarded;
        pimpl->add_splice(req->backend_direct_fd, req->pipe_fds[1], bytes_left,
                          req.get(), true, true);
      }

      io_uring_submit(&pimpl->ring);
      req.release();
      break;
    }

    case EventType::CLOSING_CLIENT_CONNECTION:
      break;
    }
  }
}
