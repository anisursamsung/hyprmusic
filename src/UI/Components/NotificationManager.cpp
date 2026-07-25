#include "NotificationManager.hpp"
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

  m_notificationBox = CRectangleBuilder::begin()
                          ->color([] { return CHyprColor(0, 0, 0, 0); })
                          ->rounding(10)
                          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                              CDynamicSize::HT_SIZE_ABSOLUTE,
                                              {240.0F, 50.0F}))
                          ->commence();
  m_notificationBox->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  m_notificationBox->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  m_notificationText =
      CTextBuilder::begin()
          ->text("")
          ->color([palette] {
            return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
          ->align(HT_FONT_ALIGN_CENTER)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  m_notificationText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  m_notificationText->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER,
                                      true);

  m_notificationBox->addChild(m_notificationText);
  if (m_rootElement) {
    m_rootElement->addChild(m_notificationBox);
  }
}

void NotificationManager::showNotification(const std::string &msg) {
  if (!m_notificationBox || !m_notificationText)
    return;

  auto palette = m_palette;

  m_notificationText->rebuild()
      ->text(std::string(msg))
      ->color([palette, msg] {
        if (msg.find("Already") != std::string::npos) {
          return CHyprColor(0.95, 0.4, 0.4, 1.0);
        }
        return palette ? palette->m_colors.accent
                       : CHyprColor(0.2, 0.85, 0.45, 1.0);
      })
      ->commence();
  m_notificationText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  m_notificationText->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER,
                                      true);

  m_notificationBox->rebuild()
      ->color([palette] {
        return palette ? palette->m_colors.base
                       : CHyprColor(0.12, 0.12, 0.12, 0.95);
      })
      ->rounding(10)
      ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                          CDynamicSize::HT_SIZE_ABSOLUTE, {240.0F, 50.0F}))
      ->commence();

  m_notificationBox->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  m_notificationBox->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  if (m_window && m_window->m_rootElement) {
    m_window->m_rootElement->forceReposition();
  }

  if (m_backend) {
    m_backend->addTimer(
        std::chrono::milliseconds(1800),
        [this](CAtomicSharedPointer<CTimer>, void *) {
          if (m_notificationText) {
            m_notificationText->rebuild()->text("")->commence();
          }
          if (m_notificationBox) {
            m_notificationBox->rebuild()
                ->color([] { return CHyprColor(0, 0, 0, 0); })
                ->commence();
          }
          if (m_window && m_window->m_rootElement) {
            m_window->m_rootElement->forceReposition();
          }
        },
        nullptr);
  }
}

} // namespace UI::Components
