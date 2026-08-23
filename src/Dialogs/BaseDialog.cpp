#include "BaseDialog.hpp"
#include "Utils/UIFactory.hpp"
#include <algorithm>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

ModalWindowResult BaseDialog::createCenteredModal(
    CSharedPointer<IWindow> parentWindow,
    const Hyprutils::Math::Vector2D &size,
    CSharedPointer<CPalette> palette,
    float rounding,
    int layoutGap,
    int layoutMargin) {
  if (!parentWindow)
    return {};

  auto windowSize = parentWindow->pixelSize();
  double posX = std::max(0.0, (windowSize.x - size.x) / 2.0);
  double posY = std::max(0.0, (windowSize.y - size.y) / 2.0);

  auto popupWindow =
      CWindowBuilder::begin()
          ->type(HT_WINDOW_POPUP)
          ->parent(parentWindow)
          ->pos(Hyprutils::Math::Vector2D(posX, posY))
          ->preferredSize(size)
          ->commence();

  if (!popupWindow)
    return {};

  auto root = Components::UIFactory::createCard(palette, rounding);
  popupWindow->m_rootElement = root;

  auto cardLayout =
      CColumnLayoutBuilder::begin()
          ->gap(layoutGap)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  cardLayout->setMargin(layoutMargin);
  root->addChild(cardLayout);

  return {popupWindow, root, cardLayout};
}

ModalWindowResult BaseDialog::createAnchoredPopup(
    CSharedPointer<IWindow> parentWindow,
    Hyprutils::Math::Vector2D anchorPos,
    Hyprutils::Math::Vector2D anchorSize,
    const Hyprutils::Math::Vector2D &popupSize,
    CSharedPointer<CPalette> palette,
    float rounding,
    int layoutGap,
    int layoutMargin) {
  if (!parentWindow)
    return {};

  auto windowSize = parentWindow->pixelSize();

  if (anchorPos.x < 0 || anchorPos.y < 0) {
    anchorPos = parentWindow->cursorPos();
    anchorSize = {0.0, 0.0};
  }

  // Vertical Gravity Calculation (Default: Below anchor)
  double posY = anchorPos.y + anchorSize.y + 4.0;
  if (posY + popupSize.y > windowSize.y) {
    // Reverse direction: Place above anchor if space allows
    double posAbove = anchorPos.y - popupSize.y - 4.0;
    if (posAbove >= 0) {
      posY = posAbove;
    } else {
      posY = std::max(0.0, windowSize.y - popupSize.y);
    }
  }

  // Horizontal Gravity Calculation (Default: Align to anchor/cursor)
  double posX = anchorPos.x;
  if (posX + popupSize.x > windowSize.x) {
    // Reverse direction: Align to left side of anchor/cursor
    double posLeft = (anchorSize.x > 0)
                         ? (anchorPos.x + anchorSize.x - popupSize.x)
                         : (anchorPos.x - popupSize.x);
    if (posLeft >= 0) {
      posX = posLeft;
    } else {
      posX = std::max(0.0, windowSize.x - popupSize.x);
    }
  }

  // Safety clamping within window boundaries
  posX = std::clamp(posX, 0.0, std::max(0.0, windowSize.x - popupSize.x));
  posY = std::clamp(posY, 0.0, std::max(0.0, windowSize.y - popupSize.y));

  auto popupWindow =
      CWindowBuilder::begin()
          ->type(HT_WINDOW_POPUP)
          ->parent(parentWindow)
          ->pos(Hyprutils::Math::Vector2D(posX, posY))
          ->preferredSize(popupSize)
          ->commence();

  if (!popupWindow)
    return {};

  auto root = Components::UIFactory::createCard(palette, rounding);
  popupWindow->m_rootElement = root;

  auto cardLayout =
      CColumnLayoutBuilder::begin()
          ->gap(layoutGap)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  cardLayout->setMargin(layoutMargin);
  root->addChild(cardLayout);

  return {popupWindow, root, cardLayout};
}

CSharedPointer<CTextElement> BaseDialog::createHeader(
    const std::string &title,
    const std::string &icon,
    CSharedPointer<CPalette> palette,
    const std::string &fontFamily,
    CFontSize::eSizingBase size) {
  std::string fullText = icon.empty() ? title : (icon + " " + title);
  return Components::UIFactory::createHeader(fullText, palette, fontFamily, size);
}

CSharedPointer<CRowLayoutElement> BaseDialog::createButtonRow(
    const std::string &cancelLabel,
    std::function<void()> onCancel,
    const std::string &confirmLabel,
    std::function<void()> onConfirm,
    CSharedPointer<CPalette> palette,
    const std::string &fontFamily,
    bool confirmIsAccent,
    int gap) {
  auto btnRow =
      CRowLayoutBuilder::begin()
          ->gap(gap)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  auto cancelBtn = Components::UIFactory::createActionButton(
      cancelLabel, onCancel, palette, fontFamily, false);
  btnRow->addChild(cancelBtn);

  auto okBtn = Components::UIFactory::createActionButton(
      confirmLabel, onConfirm, palette, fontFamily, confirmIsAccent);
  btnRow->addChild(okBtn);

  return btnRow;
}

} // namespace UI::Dialogs
