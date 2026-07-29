#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <functional>
#include <memory>
#include <string>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct HelpViewContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;
  std::function<void(const std::string &)> showNotification;
};

class HelpView {
public:
  explicit HelpView(const HelpViewContext &ctx);

  void rebuildUI(CSharedPointer<CRectangleElement> wrapper);
  void resetLayout() { m_helpContentLayout = nullptr; }

private:
  HelpViewContext m_ctx;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;
  CSharedPointer<CColumnLayoutElement> m_helpContentLayout;
};

} // namespace UI::Views
