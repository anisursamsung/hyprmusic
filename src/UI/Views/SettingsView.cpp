#include "SettingsView.hpp"
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <vector>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

SettingsView::SettingsView(const SettingsViewContext &ctx) : m_ctx(ctx) {}

void SettingsView::addSettingRow(const std::string &label,
                                 const std::string &configKey,
                                 bool isDirectory) {
  if (!m_settingsContentLayout)
    return;

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;
  int rounding = palette ? palette->m_vars.smallRounding : 5;

  auto rowItem =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.base
                           : CHyprColor(0.15, 0.15, 0.15, 1.0);
          })
          ->rounding(rounding)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 50.0F}))
          ->commence();

  auto rowLayout = CRowLayoutBuilder::begin()
                       ->gap(0)
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                           CDynamicSize::HT_SIZE_PERCENT,
                                           {1.0F, 1.0F}))
                       ->commence();
  rowLayout->setMargin(10);
  rowItem->addChild(rowLayout);

  auto cell0 = CRowLayoutBuilder::begin()
                   ->gap(0)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {0.50F, 1.0F}))
                   ->commence();
  auto labelText = CTextBuilder::begin()
                       ->text(std::string(label))
                       ->color([palette] {
                         return palette ? palette->m_colors.text
                                        : CHyprColor(1, 1, 1, 1);
                       })
                       ->fontFamily(std::string(fontFamily))
                       ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                           CDynamicSize::HT_SIZE_PERCENT,
                                           {1.0F, 1.0F}))
                       ->align(HT_FONT_ALIGN_LEFT)
                       ->noEllipsize(false)
                       ->commence();
  cell0->addChild(labelText);
  rowLayout->addChild(cell0);

  std::string valueStr = m_pendingSettings.count(configKey)
                             ? m_pendingSettings[configKey]
                             : m_ctx.mpdSettings[configKey];

  auto cell1 = CRowLayoutBuilder::begin()
                   ->gap(0)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {0.50F, 1.0F}))
                   ->commence();
  auto inputTextbox =
      CTextboxBuilder::begin()
          ->placeholder(isDirectory ? "Path..." : "Value...")
          ->defaultText(std::string(valueStr))
          ->onTextEdited([this, configKey](CSharedPointer<CTextboxElement>,
                                           const std::string &text) {
            m_pendingSettings[configKey] = text;
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {0.98F, 30.0F}))
          ->commence();
  cell1->addChild(inputTextbox);
  rowLayout->addChild(cell1);

  m_settingsContentLayout->addChild(rowItem);
}

void SettingsView::addSectionHeader(const std::string &title) {
  if (!m_settingsContentLayout)
    return;

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;

  auto headerText =
      CTextBuilder::begin()
          ->text(std::string(title))
          ->color([palette] {
            return palette ? palette->m_colors.accent
                           : CHyprColor(0.2, 0.8, 0.4, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->align(HT_FONT_ALIGN_LEFT)
          ->commence();
  m_settingsContentLayout->addChild(headerText);
}

void SettingsView::addSwitchSettingRow(const std::string &label,
                                        const std::string &configKey) {
  if (!m_settingsContentLayout)
    return;

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;
  int rounding = palette ? palette->m_vars.smallRounding : 5;

  auto rowItem =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.base
                           : CHyprColor(0.15, 0.15, 0.15, 1.0);
          })
          ->rounding(rounding)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 50.0F}))
          ->commence();

  auto rowLayout = CRowLayoutBuilder::begin()
                       ->gap(0)
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                           CDynamicSize::HT_SIZE_PERCENT,
                                           {1.0F, 1.0F}))
                       ->commence();
  rowLayout->setMargin(10);
  rowItem->addChild(rowLayout);

  auto cell0 = CRowLayoutBuilder::begin()
                   ->gap(0)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {0.50F, 1.0F}))
                   ->commence();
  auto labelText = CTextBuilder::begin()
                       ->text(std::string(label))
                       ->color([palette] {
                         return palette ? palette->m_colors.text
                                        : CHyprColor(1, 1, 1, 1);
                       })
                       ->fontFamily(std::string(fontFamily))
                       ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                           CDynamicSize::HT_SIZE_PERCENT,
                                           {1.0F, 1.0F}))
                       ->align(HT_FONT_ALIGN_LEFT)
                       ->noEllipsize(false)
                       ->commence();
  cell0->addChild(labelText);
  rowLayout->addChild(cell0);

  std::string curValStr = m_pendingSettings.count(configKey)
                              ? m_pendingSettings[configKey]
                              : m_ctx.mpdSettings[configKey];
  bool currentVal = (curValStr != "no");
  size_t currentIdx = currentVal ? 0 : 1;

  std::vector<std::string> comboItems = {"ON", "OFF"};

  auto cell1 = CRowLayoutBuilder::begin()
                   ->gap(0)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {0.50F, 1.0F}))
                   ->commence();
  auto combobox =
      CComboboxBuilder::begin()
          ->items(std::move(comboItems))
          ->currentItem(currentIdx)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {0.98F, 30.0F}))
          ->onChanged([this, configKey](
                          CSharedPointer<CComboboxElement>,
                          size_t idx) {
            m_pendingSettings[configKey] = (idx == 0 ? "yes" : "no");
          })
          ->commence();
  cell1->addChild(combobox);
  rowLayout->addChild(cell1);

  m_settingsContentLayout->addChild(rowItem);
}

void SettingsView::rebuildUI(CSharedPointer<CRectangleElement> wrapper) {
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;

  m_tabContentWrapper->clearChildren();

  auto settingsMainLayout =
      CColumnLayoutBuilder::begin()
          ->gap(15)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  settingsMainLayout->setMargin(20);
  m_tabContentWrapper->addChild(settingsMainLayout);

  auto titleHeader =
      CTextBuilder::begin()
          ->text("Settings")
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0, 1.0, 1.0, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H1))
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  settingsMainLayout->addChild(titleHeader);

  auto scrollArea =
      CScrollAreaBuilder::begin()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.90F}))
          ->scrollY(true)
          ->commence();
  settingsMainLayout->addChild(scrollArea);

  m_settingsContentLayout =
      CColumnLayoutBuilder::begin()
          ->gap(10)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  m_settingsContentLayout->setMargin(5);
  scrollArea->addChild(m_settingsContentLayout);

  addSectionHeader("📁 Directories & Files");
  addSettingRow("Music Directory", "music_directory", true);
  addSettingRow("Playlist Directory", "playlist_directory", true);
  addSettingRow("Database File", "db_file", false);
  addSettingRow("Log File", "log_file", false);
  addSettingRow("PID File", "pid_file", false);
  addSettingRow("State File", "state_file", false);
  addSettingRow("Sticker File", "sticker_file", false);

  addSectionHeader("🌐 Network");
  addSettingRow("Bind Address", "bind_to_address", false);
  addSettingRow("Port", "port", false);

  addSectionHeader("⚙️ Playback & Daemon Behavior");
  addSwitchSettingRow("Restore Paused on Start", "restore_paused");
  addSwitchSettingRow("Auto Update Database", "auto_update");

  addSectionHeader("🔊 Audio Output");
  addSwitchSettingRow("PipeWire Sound Server (Pulse)", "audio_output_pulse");
  addSwitchSettingRow("Visualizer FIFO (/tmp/mpd.fifo)", "audio_output_fifo");

  auto bottomRow =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 40.0F}))
          ->commence();

  auto saveBtn =
      CButtonBuilder::begin()
          ->label("💾 Save & Restart MPD")
          ->alignText(HT_FONT_ALIGN_CENTER)
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->onMainClick([this](CSharedPointer<CButtonElement>) {
            if (m_ctx.saveSettings) {
              m_ctx.saveSettings(m_pendingSettings);
            }
            if (m_ctx.showNotification) {
              m_ctx.showNotification("Saved settings & restarted MPD");
            }
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
          ->commence();
  bottomRow->addChild(saveBtn);
  settingsMainLayout->addChild(bottomRow);

  m_tabContentWrapper->forceReposition();
}

} // namespace UI::Views
