#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct SettingsViewContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;

  std::unordered_map<std::string, std::string> mpdSettings;
  std::function<void(const std::unordered_map<std::string, std::string> &newSettings)> saveSettings;
  std::function<void(const std::string &msg)> showNotification;
};

class SettingsView {
public:
  explicit SettingsView(const SettingsViewContext &ctx);

  void rebuildUI(CSharedPointer<CRectangleElement> wrapper);
  void resetLayout() { m_settingsContentLayout = nullptr; }

private:
  void addSettingRow(const std::string &label, const std::string &configKey, bool isDirectory);
  void addSectionHeader(const std::string &title);
  void addSwitchSettingRow(const std::string &label, const std::string &configKey);
  void populateSettingsRows();

  SettingsViewContext m_ctx;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;
  CSharedPointer<CColumnLayoutElement> m_settingsContentLayout;

  std::unordered_map<std::string, std::string> m_pendingSettings;
  std::string m_searchFilter;
};

} // namespace UI::Views
