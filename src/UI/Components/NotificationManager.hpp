#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#define private public
#include <hyprtoolkit/element/Text.hpp>
#undef private
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>

#include <string>

namespace UI::Components {

class NotificationManager {
public:
  NotificationManager() = default;

  void init(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRectangleElement> rootElement,
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> window,
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend,
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette,
            const std::string &fontFamily);

  void showNotification(const std::string &msg);

private:
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRectangleElement> m_rootElement;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> m_window;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> m_backend;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> m_palette;
  std::string m_fontFamily;

  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRectangleElement> m_notificationBox;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement> m_notificationText;
};

} // namespace UI::Components
