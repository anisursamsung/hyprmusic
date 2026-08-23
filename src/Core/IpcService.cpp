#include "IpcService.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace Services {

IpcService::IpcService() {}

IpcService::~IpcService() {
  if (m_ipcSocket >= 0) {
    ::close(m_ipcSocket);
    m_ipcSocket = -1;
  }
  if (!m_ipcSocketPath.empty()) {
    ::unlink(m_ipcSocketPath.c_str());
  }
}

void IpcService::init(
    std::function<void(const std::vector<std::string> &uris)> onUrisReceived) {
  m_onUrisReceived = onUrisReceived;
  m_ipcSocketPath = "/tmp/hyprmusic-" + std::to_string(::getuid()) + ".sock";

  ::unlink(m_ipcSocketPath.c_str());

  m_ipcSocket = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (m_ipcSocket < 0) {
    std::cerr << "IPC: Failed to create socket: " << strerror(errno)
              << std::endl;
    return;
  }

  int flags = ::fcntl(m_ipcSocket, F_GETFL, 0);
  ::fcntl(m_ipcSocket, F_SETFL, flags | O_NONBLOCK);

  struct sockaddr_un addr {};
  addr.sun_family = AF_UNIX;
  ::strncpy(addr.sun_path, m_ipcSocketPath.c_str(), sizeof(addr.sun_path) - 1);

  if (::bind(m_ipcSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    std::cerr << "IPC: Failed to bind socket: " << strerror(errno)
              << std::endl;
    ::close(m_ipcSocket);
    m_ipcSocket = -1;
    return;
  }

  if (::listen(m_ipcSocket, 5) < 0) {
    std::cerr << "IPC: Failed to listen: " << strerror(errno) << std::endl;
    ::close(m_ipcSocket);
    m_ipcSocket = -1;
    return;
  }

  std::cout << "IPC: Listening on " << m_ipcSocketPath << std::endl;
}

void IpcService::poll() {
  if (m_ipcSocket < 0)
    return;

  int clientFd = ::accept(m_ipcSocket, nullptr, nullptr);
  if (clientFd < 0)
    return;

  int flags = ::fcntl(clientFd, F_GETFL, 0);
  ::fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

  std::string received;
  char buf[4096];
  ssize_t n;
  while ((n = ::recv(clientFd, buf, sizeof(buf), 0)) > 0) {
    received.append(buf, n);
  }
  ::close(clientFd);

  if (received.empty())
    return;

  std::vector<std::string> uris;
  std::istringstream iss(received);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.empty())
      break;
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (!line.empty())
      uris.push_back(line);
  }

  if (!uris.empty() && m_onUrisReceived) {
    std::cout << "IPC: Received " << uris.size() << " URI(s) from client"
              << std::endl;
    m_onUrisReceived(uris);
  }
}

} // namespace Services
