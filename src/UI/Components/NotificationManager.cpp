#include "NotificationManager.hpp"
#include <algorithm>
#include <iostream>

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

namespace UI::Components {

void NotificationManager::init(
    CSharedPointer<CRectangleElement> rootElement,
    CSharedPointer<IWindow> window, CSharedPointer<IBackend> backend,
    CSharedPointer<CPalette> palette, const std::string &fontFamily) {
  m_rootElement = rootElement;
  m_window = window;
  m_backend = backend;
  m_palette = palette;
  m_fontFamily = fontFamily;
}

void NotificationManager::showNotification(const std::string &msg) {
  if (!m_window)
    return;

  // If a notification popup is currently active, close it first
  if (m_activeNotificationWindow) {
    m_activeNotificationWindow->close();
    m_activeNotificationWindow.reset();
  }

  auto palette = m_palette;
  auto windowSize = m_window->pixelSize();
  double popupWidth = std::max(280.0, msg.length() * 9.5 + 40.0);
  double popupHeight = 44.0;
  double posX = std::max(0.0, (windowSize.x - popupWidth) / 2.0);
  double posY = 30.0; // Centered near top of window

  auto popupWindow =
      CWindowBuilder::begin()
          ->type(HT_WINDOW_POPUP)
          ->parent(m_window)
          ->pos(Hyprutils::Math::Vector2D(posX, posY))
          ->preferredSize(Hyprutils::Math::Vector2D(popupWidth, popupHeight))
          ->commence();

  if (!popupWindow)
    return;

  m_activeNotificationWindow = popupWindow;

  CHyprColor textColor = (msg.find("Already") != std::string::npos ||
                          msg.find("❌") != std::string::npos)
                             ? CHyprColor(0.95, 0.4, 0.4, 1.0)
                             : (palette ? palette->m_colors.text
                                        : CHyprColor(1.0, 1.0, 1.0, 1.0));

  auto root =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.base
                           : CHyprColor(0.12, 0.12, 0.12, 0.95);
          })
          ->rounding(10)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  popupWindow->m_rootElement = root;

  auto textElem =
      CTextBuilder::begin()
          ->text(std::string(msg))
          ->color([textColor] { return textColor; })
          ->fontFamily(std::string(m_fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
          ->align(HT_FONT_ALIGN_CENTER)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  textElem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  textElem->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  root->addChild(textElem);
  popupWindow->open();

  if (m_backend) {
    m_backend->addTimer(
        std::chrono::milliseconds(2000),
        [this, popupWindow](CAtomicSharedPointer<CTimer>, void *) {
          if (m_activeNotificationWindow == popupWindow) {
            m_activeNotificationWindow->close();
            m_activeNotificationWindow.reset();
          } else if (popupWindow) {
            popupWindow->close();
          }
        },
        nullptr);
  }
}

} // namespace UI::Components
