#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Services {

class IpcService {
public:
  IpcService();
  ~IpcService();

  void init(std::function<void(const std::vector<std::string> &uris)> onUrisReceived);
  void poll();

private:
  int m_ipcSocket = -1;
  std::string m_ipcSocketPath;
  std::function<void(const std::vector<std::string> &uris)> m_onUrisReceived;
};

} // namespace Services
