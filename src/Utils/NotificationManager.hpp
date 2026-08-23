#pragma once

#include <string>

namespace Utils {

class NotificationManager {
public:
  NotificationManager() = default;

  void showNotification(const std::string &msg);
};

} // namespace Utils
