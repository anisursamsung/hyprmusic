#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <pwd.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_set>
#include <vector>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Checkbox.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Line.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Slider.hpp>
#define private public
#include <hyprtoolkit/element/Text.hpp>
#undef private
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/system/Icons.hpp>
#include <hyprtoolkit/window/Window.hpp>

#include "MPDClient.hpp"
#include "UI/Components/NotificationManager.hpp"
#include "UI/Dialogs/AddItemDialog.hpp"
#include "Utils/StreamUtils.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class HyprMusicApp {
public:
  HyprMusicApp() {
    m_backend = IBackend::create();
    if (!m_backend) {
      throw std::runtime_error("Failed to create backend");
    }
  }

  void run() {
    ensureMpdRunningAndConfigured();
    createWindow();
    createUI();
    setupEventHandlers();

    // Initial update and setup periodic status query
    updateStatus();
    setupTimer();

    std::cout << "Starting HyprMusic..." << std::endl;
    m_window->open();
    m_backend->enterLoop();
  }

private:
  CSharedPointer<IBackend> m_backend;
  CSharedPointer<IWindow> m_window;

  CSharedPointer<CTextElement> m_nowPlayingText;
  CSharedPointer<CTextElement> m_timeText;
  CSharedPointer<CSliderElement> m_seekBar;
  bool m_isUpdatingSeekBar = false;
  CSharedPointer<CSliderElement> m_volumeSlider;
  bool m_isUpdatingVolumeSlider = false;
  CSharedPointer<IElement> m_pauseBtn;

  CSharedPointer<CColumnLayoutElement> m_controlsHeader;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;
  CSharedPointer<CColumnLayoutElement> m_mainTabPanel;
  std::string m_queueSearchQuery = "";
  std::string m_dbSearchQuery = "";
  std::string m_dbCurrentPath = "";
  std::string m_playlistsSearchQuery = "";
  std::string m_selectedPlaylist = "";
  bool m_playlistsDetailedView = false;
  std::string m_savePlaylistName = "";
  std::string m_streamUrlInput = "";
  std::string m_newPlaylistNameInput = "";
  CSharedPointer<CPalette> m_palette;
  std::string m_fontFamily = "Sans Serif";
  unsigned m_lastQueueVersion = 0;
  int m_lastActiveSongId = -2;
  bool m_playlistLoaded = false;
  std::string m_renamingPlaylist = "";
  std::string m_renameInputText = "";

  CSharedPointer<CColumnLayoutElement> m_playlistsLeftItemsLayout;
  CSharedPointer<CColumnLayoutElement> m_playlistsRightItemsLayout;
  double m_lastPlaylistsWidth = 0.0;
  std::vector<std::string> m_currentPlaylists;
  std::unordered_map<std::string, int> m_playlistTrackCounts;
  std::unordered_map<int, CSharedPointer<CTextElement>> m_queueSongTexts;
  CSharedPointer<CColumnLayoutElement> m_dbContentLayout;
  CSharedPointer<CColumnLayoutElement> m_queueContentLayout;
  UI::Components::NotificationManager m_notificationManager;
  CSharedPointer<CRectangleElement> m_notificationBox;
  CSharedPointer<CTextElement> m_notificationText;

  enum eViewMode {
    VIEW_QUEUE,
    VIEW_DATABASE,
    VIEW_PLAYLISTS,
    VIEW_YTDLP,
    VIEW_SETTINGS,
    VIEW_HELP
  };
  eViewMode m_viewMode = VIEW_QUEUE;

  CSharedPointer<CTextElement> m_queueTabBtn;
  CSharedPointer<CTextElement> m_dbTabBtn;
  CSharedPointer<CTextElement> m_playlistsTabBtn;
  CSharedPointer<CTextElement> m_ytdlpTabBtn;
  CSharedPointer<CTextElement> m_settingsTabBtn;
  CSharedPointer<CTextElement> m_helpTabBtn;
  CSharedPointer<CRectangleElement> m_queueTabLine;
  CSharedPointer<CRectangleElement> m_dbTabLine;
  CSharedPointer<CRectangleElement> m_playlistsTabLine;
  CSharedPointer<CRectangleElement> m_ytdlpTabLine;
  CSharedPointer<CRectangleElement> m_settingsTabLine;
  CSharedPointer<CRectangleElement> m_helpTabLine;
  CSharedPointer<CColumnLayoutElement> m_settingsContentLayout;
  CSharedPointer<CColumnLayoutElement> m_helpContentLayout;
  std::unordered_map<std::string, std::string> m_mpdSettings;
  std::unordered_map<std::string, std::string> m_pendingSettings;

  std::string m_ytdlpSearchTitle = "";
  std::string m_ytdlpResultCount = "5";
  bool m_ytdlpSearching = false;
  bool m_ytdlpNeedRebuild = false;
  std::mutex m_ytdlpMutex;

  struct YtDlpResult {
    std::string id;
    std::string title;
    std::string uploader;
    std::string duration;
    std::string url;
  };
  std::vector<YtDlpResult> m_ytdlpResults;
  bool m_ytdlpIsPlaylist = false;
  std::string m_ytdlpPlaylistTitle = "";
  std::string m_ytdlpPlaylistId = "";

  std::unordered_map<std::string, std::pair<std::string, std::string>>
      m_urlTitleMap;
  std::mutex m_titleMapMutex;

  void setUrlTitle(const std::string &url, const std::string &title,
                   const std::string &uploader = "") {
    std::lock_guard<std::mutex> lock(m_titleMapMutex);
    m_urlTitleMap[url] = {title, uploader};
  }

  bool getUrlTitle(const std::string &url, std::string &title,
                   std::string &uploader) {
    std::lock_guard<std::mutex> lock(m_titleMapMutex);
    auto it = m_urlTitleMap.find(url);
    if (it != m_urlTitleMap.end()) {
      title = it->second.first;
      uploader = it->second.second;
      return true;
    }
    return false;
  }

  bool m_isPlaying = false;

  Hyprutils::Signal::CHyprSignalListener m_keyboardListener;

  void createWindow() {
    m_window = CWindowBuilder::begin()
                   ->type(HT_WINDOW_TOPLEVEL)
                   ->appTitle("HyprMusic")
                   ->appClass("hyprmusic")
                   ->preferredSize({0, 0})
                   ->minSize({600, 400})
                   ->commence();

    if (!m_window) {
      throw std::runtime_error("Failed to create window");
    }
  }

  void createUI() {
    m_palette = CPalette::palette();
    m_fontFamily =
        m_palette ? std::string(m_palette->m_vars.fontFamily) : "Sans Serif";
    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;

    // ==========================================
    // 1. ROOT & MAIN BACKGROUND
    // ==========================================
    auto root =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    m_window->m_rootElement = root;

    auto mainBg =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.background
                             : CHyprColor(0.1, 0.1, 0.1, 1.0);
            })
            ->rounding(palette ? palette->m_vars.bigRounding : 10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    root->addChild(mainBg);

    auto mainColumn =
        CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    mainBg->addChild(mainColumn);

    // ==========================================
    // 3. TOP TABS HEADER & DISPLAY AREA
    // ==========================================
    // Tab buttons layout (Top Header wrapped in tabsSection rectangle)
    auto tabsSection =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.background
                             : CHyprColor(0.15, 0.15, 0.15, 1.0);
            })
            ->rounding(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 42.0F}))
            ->commence();
    tabsSection->setGrow(false);

    auto tabsRow =
        CRowLayoutBuilder::begin()
            ->gap(15)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    tabsRow->setMargin(12);

    m_queueTabBtn =
        CTextBuilder::begin()
            ->text("Queue")
            ->color([palette] {
              return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_queueTabBtn->setReceivesMouse(true);
    m_queueTabBtn->setMouseButton(
        [this](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            switchViewMode(VIEW_QUEUE);
          }
        });

    m_queueTabLine =
        CRectangleBuilder::begin()
            ->color([this, palette] {
              if (m_viewMode == VIEW_QUEUE) {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              }
              return CHyprColor(0, 0, 0, 0);
            })
            ->rounding(1)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 2.0F}))
            ->commence();

    auto queueCol =
        CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    queueCol->addChild(m_queueTabBtn);
    queueCol->addChild(m_queueTabLine);
    tabsRow->addChild(queueCol);

    m_dbTabBtn =
        CTextBuilder::begin()
            ->text("Database")
            ->color([palette] {
              return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_dbTabBtn->setReceivesMouse(true);
    m_dbTabBtn->setMouseButton([this](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && !down) {
        switchViewMode(VIEW_DATABASE);
      }
    });

    m_dbTabLine =
        CRectangleBuilder::begin()
            ->color([this, palette] {
              if (m_viewMode == VIEW_DATABASE) {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              }
              return CHyprColor(0, 0, 0, 0);
            })
            ->rounding(1)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 2.0F}))
            ->commence();

    auto dbCol =
        CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    dbCol->addChild(m_dbTabBtn);
    dbCol->addChild(m_dbTabLine);
    tabsRow->addChild(dbCol);

    m_playlistsTabBtn =
        CTextBuilder::begin()
            ->text("Playlists")
            ->color([palette] {
              return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_playlistsTabBtn->setReceivesMouse(true);
    m_playlistsTabBtn->setMouseButton(
        [this](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            switchViewMode(VIEW_PLAYLISTS);
          }
        });

    m_playlistsTabLine =
        CRectangleBuilder::begin()
            ->color([this, palette] {
              if (m_viewMode == VIEW_PLAYLISTS) {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              }
              return CHyprColor(0, 0, 0, 0);
            })
            ->rounding(1)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 2.0F}))
            ->commence();

    auto playlistsCol =
        CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    playlistsCol->addChild(m_playlistsTabBtn);
    playlistsCol->addChild(m_playlistsTabLine);
    tabsRow->addChild(playlistsCol);

    m_ytdlpTabBtn =
        CTextBuilder::begin()
            ->text("YT DLP")
            ->color([palette] {
              return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_ytdlpTabBtn->setReceivesMouse(true);
    m_ytdlpTabBtn->setMouseButton(
        [this](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            switchViewMode(VIEW_YTDLP);
          }
        });

    m_ytdlpTabLine =
        CRectangleBuilder::begin()
            ->color([this, palette] {
              if (m_viewMode == VIEW_YTDLP) {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              }
              return CHyprColor(0, 0, 0, 0);
            })
            ->rounding(1)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 2.0F}))
            ->commence();

    auto ytdlpCol =
        CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    ytdlpCol->addChild(m_ytdlpTabBtn);
    ytdlpCol->addChild(m_ytdlpTabLine);
    tabsRow->addChild(ytdlpCol);

    m_settingsTabBtn =
        CTextBuilder::begin()
            ->text("Settings")
            ->color([palette] {
              return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_settingsTabBtn->setReceivesMouse(true);
    m_settingsTabBtn->setMouseButton(
        [this](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            switchViewMode(VIEW_SETTINGS);
          }
        });

    m_settingsTabLine =
        CRectangleBuilder::begin()
            ->color([this, palette] {
              if (m_viewMode == VIEW_SETTINGS) {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              }
              return CHyprColor(0, 0, 0, 0);
            })
            ->rounding(1)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 2.0F}))
            ->commence();

    auto settingsCol =
        CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    settingsCol->addChild(m_settingsTabBtn);
    settingsCol->addChild(m_settingsTabLine);
    tabsRow->addChild(settingsCol);

    m_helpTabBtn =
        CTextBuilder::begin()
            ->text("Help")
            ->color([palette] {
              return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_helpTabBtn->setReceivesMouse(true);
    m_helpTabBtn->setMouseButton(
        [this](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            switchViewMode(VIEW_HELP);
          }
        });

    m_helpTabLine =
        CRectangleBuilder::begin()
            ->color([this, palette] {
              if (m_viewMode == VIEW_HELP) {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              }
              return CHyprColor(0, 0, 0, 0);
            })
            ->rounding(1)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 2.0F}))
            ->commence();

    auto helpCol =
        CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    helpCol->addChild(m_helpTabBtn);
    helpCol->addChild(m_helpTabLine);
    tabsRow->addChild(helpCol);

    tabsSection->addChild(tabsRow);
    mainColumn->addChild(tabsSection);

    // ==========================================
    // 3. DISPLAY AREA (MAIN CONTENT VIEW)
    // ==========================================
    auto contentSection =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.alternateBase
                             : CHyprColor(0.2, 0.2, 0.2, 1.0);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
            ->commence();
    contentSection->setGrow(true);

    auto contentLayout =
        CRowLayoutBuilder::begin()
            ->gap(20)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    contentLayout->setMargin(15);
    contentSection->addChild(contentLayout);

    m_tabContentWrapper =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();

    contentLayout->addChild(m_tabContentWrapper);
    mainColumn->addChild(contentSection);

    // ==========================================
    // ==========================================
    // ==========================================
    // 4a. SONG INFO SECTION (BAR) - 6%
    // ==========================================
    auto songInfoSection =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.background
                             : CHyprColor(0.15, 0.15, 0.15, 1.0);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 42.0F}))
            ->commence();

    m_nowPlayingText =
        CTextBuilder::begin()
            ->text("Track 1 - Unknown Artist")
            ->color([palette] {
              return palette ? palette->m_colors.accent
                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->align(HT_FONT_ALIGN_LEFT)
            ->noEllipsize(false)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.96F, 1.0F}))
            ->commence();
    m_nowPlayingText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    m_nowPlayingText->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
    m_nowPlayingText->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);

    songInfoSection->addChild(m_nowPlayingText);
    mainColumn->addChild(songInfoSection);

    // ==========================================
    // 4b. SEEK BAR & TIME DISPLAY SECTION
    // ==========================================
    auto seekBarSection =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.background
                             : CHyprColor(0.15, 0.15, 0.15, 1.0);
            })
            ->rounding(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 42.0F}))
            ->commence();

    auto seekBarRow =
        CRowLayoutBuilder::begin()
            ->gap(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    seekBarRow->setMargin(15);

    m_timeText =
        CTextBuilder::begin()
            ->text("0:00 / 0:00")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(0.8, 0.8, 0.8, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->align(HT_FONT_ALIGN_LEFT)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    m_timeText->setGrow(false);

    m_seekBar =
        CSliderBuilder::begin()
            ->min(0.0f)
            ->max(1.0f)
            ->val(0.0f)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 10.0F}))
            ->onChanged([this](CSharedPointer<CSliderElement>, float val) {
              if (m_isUpdatingSeekBar)
                return;
              runMpdCommand([val](struct mpd_connection *conn) {
                struct mpd_status *status = mpd_run_status(conn);
                if (status) {
                  unsigned total = mpd_status_get_total_time(status);
                  if (total > 0) {
                    float seconds = val * static_cast<float>(total);
                    mpd_run_seek_current(conn, seconds, false);
                  }
                  mpd_status_free(status);
                }
              });
            })
            ->commence();
    m_seekBar->setReceivesMouse(true);
    m_seekBar->setMouseButton([this](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && down) {
        auto cursorPos = m_window->cursorPos();
        auto sliderSize = m_seekBar->size();
        if (sliderSize.x > 0.0) {
          float pct = std::clamp(static_cast<float>(cursorPos.x / sliderSize.x),
                                 0.0f, 1.0f);
          runMpdCommand([this, pct](struct mpd_connection *conn) {
            struct mpd_status *status = mpd_run_status(conn);
            if (status) {
              unsigned total = mpd_status_get_total_time(status);
              if (total > 0) {
                float seconds = pct * static_cast<float>(total);
                mpd_run_seek_current(conn, seconds, false);
                m_isUpdatingSeekBar = true;
                m_seekBar->rebuild()->val(pct)->commence();
                m_isUpdatingSeekBar = false;
              }
              mpd_status_free(status);
            }
          });
        }
      }
    });
    m_seekBar->setGrow(true);

    seekBarRow->addChild(m_timeText);
    seekBarRow->addChild(m_seekBar);
    seekBarSection->addChild(seekBarRow);
    mainColumn->addChild(seekBarSection);

    // ==========================================
    // 5. MUSIC CONTROL SECTION (BAR) - 10%
    // ==========================================
    auto controlsSection =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.background
                             : CHyprColor(0.15, 0.15, 0.15, 1.0);
            })
            ->rounding(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.12F}))
            ->commence();

    auto controlsLayout =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();

    auto iconFactory = m_backend->systemIcons();

    auto addControlColumn =
        [&](const std::string &iconName, const std::string &fallbackLabel,
            std::function<void(Input::eMouseButton, bool)> &&onClick) {
          auto col = CRectangleBuilder::begin()
                         ->color([] { return CHyprColor(0, 0, 0, 0); })
                         ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                             CDynamicSize::HT_SIZE_PERCENT,
                                             {1.0F / 4.0F, 1.0F}))
                         ->commence();

          CSharedPointer<IElement> btn;
          CSharedPointer<ISystemIconDescription> iconDesc;
          if (iconFactory) {
            iconDesc = iconFactory->lookupIcon(iconName);
          }

          if (iconDesc) {
            btn = CImageBuilder::begin()
                      ->icon(iconDesc)
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                          CDynamicSize::HT_SIZE_PERCENT,
                                          {0.6F, 0.6F}))
                      ->fitMode(IMAGE_FIT_MODE_CONTAIN)
                      ->commence();
          } else {
            btn = CTextBuilder::begin()
                      ->text(std::string(fallbackLabel))
                      ->color([palette] {
                        return palette ? palette->m_colors.text
                                       : CHyprColor(1, 1, 1, 1);
                      })
                      ->fontFamily(std::string(fontFamily))
                      ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
                      ->align(HT_FONT_ALIGN_CENTER)
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                          CDynamicSize::HT_SIZE_AUTO,
                                          {1.0F, 1.0F}))
                      ->interactable(true)
                      ->commence();
          }

          btn->setReceivesMouse(true);
          btn->setMouseButton(std::move(onClick));
          btn->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
          btn->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

          col->addChild(btn);
          controlsLayout->addChild(col);
          return btn;
        };

    // 1st box: Volume Slider (Column 0)
    {
      auto col = CRectangleBuilder::begin()
                     ->color([] { return CHyprColor(0, 0, 0, 0); })
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                         CDynamicSize::HT_SIZE_PERCENT,
                                         {1.0F / 4.0F, 1.0F}))
                     ->commence();

      m_volumeSlider =
          CSliderBuilder::begin()
              ->min(0.0f)
              ->max(1.0f)
              ->val(1.0f)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {0.9F, 0.25F}))
              ->onChanged([this](CSharedPointer<CSliderElement>, float val) {
                if (m_isUpdatingVolumeSlider)
                  return;
                int vol = std::clamp(static_cast<int>(val * 100.0f), 0, 100);
                runMpdCommand([vol](struct mpd_connection *conn) {
                  mpd_run_set_volume(conn, vol);
                });
              })
              ->commence();
      m_volumeSlider->setReceivesMouse(true);
      m_volumeSlider->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
      m_volumeSlider->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

      m_volumeSlider->setMouseButton(
          [this](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && down) {
              auto cursorPos = m_window->cursorPos();
              auto sliderSize = m_volumeSlider->size();
              if (sliderSize.x > 0.0) {
                float pct = std::clamp(
                    static_cast<float>(cursorPos.x / sliderSize.x), 0.0f, 1.0f);
                runMpdCommand([this, pct](struct mpd_connection *conn) {
                  int vol = std::clamp(static_cast<int>(pct * 100.0f), 0, 100);
                  mpd_run_set_volume(conn, vol);
                  m_isUpdatingVolumeSlider = true;
                  m_volumeSlider->rebuild()->val(pct)->commence();
                  m_isUpdatingVolumeSlider = false;
                });
              }
            }
          });

      col->addChild(m_volumeSlider);
      controlsLayout->addChild(col);
    }

    addControlColumn("media-skip-backward", "⏮",
                     [this](Input::eMouseButton button, bool down) {
                       if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                         prevMpdTrack();
                       }
                     });

    m_pauseBtn =
        addControlColumn("media-playback-start", "▶",
                         [this](Input::eMouseButton button, bool down) {
                           if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                             toggleMpdPlayPause();
                           }
                         });

    addControlColumn("media-skip-forward", "⏭",
                     [this](Input::eMouseButton button, bool down) {
                       if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                         nextMpdTrack();
                       }
                     });

    controlsSection->addChild(controlsLayout);
    mainColumn->addChild(controlsSection);

    m_notificationManager.init(root, m_window, m_backend, palette, std::string(fontFamily));
  }

  void setupEventHandlers() {
    m_window->m_events.closeRequest.listenStatic([this] {
      if (m_backend) {
        m_backend->addIdle([this] { m_backend->destroy(); });
      }
    });

    m_keyboardListener = m_window->m_events.keyboardKey.listen(
        [this](const Input::SKeyboardKeyEvent &e) {
          if (!e.down)
            return;
          if (e.xkbKeysym == XKB_KEY_Escape) {
            m_window->close();
            if (m_backend) {
              m_backend->addIdle([this] { m_backend->destroy(); });
            }
          }
        });
  }

  std::string getMpdConfPath() {
    std::string homeDir = getUserHomeDir();
    return homeDir + "/.config/mpd/mpd.conf";
  }

  std::unordered_map<std::string, std::string>
  parseMpdConfig(const std::string &path) {
    std::unordered_map<std::string, std::string> settings;
    std::ifstream file(path);
    if (!file.is_open()) {
      settings["auto_update"] = "yes";
      settings["restore_paused"] = "yes";
      settings["audio_output_pulse"] = "yes";
      settings["audio_output_fifo"] = "yes";
      return settings;
    }

    std::string line;
    bool inAudioBlock = false;
    std::string currentAudioType = "";

    while (std::getline(file, line)) {
      size_t first = line.find_first_not_of(" \t");
      if (first == std::string::npos)
        continue;
      if (line[first] == '#')
        continue;

      if (line.find("audio_output {") != std::string::npos ||
          (line.find("audio_output") != std::string::npos &&
           line.find('{') != std::string::npos)) {
        inAudioBlock = true;
        currentAudioType = "";
        continue;
      }

      if (inAudioBlock) {
        if (line.find('}') != std::string::npos) {
          inAudioBlock = false;
          if (currentAudioType == "pulse") {
            settings["audio_output_pulse"] = "yes";
          } else if (currentAudioType == "fifo") {
            settings["audio_output_fifo"] = "yes";
          }
          continue;
        }
        if (line.find("type") != std::string::npos) {
          if (line.find("pulse") != std::string::npos) {
            currentAudioType = "pulse";
          } else if (line.find("fifo") != std::string::npos) {
            currentAudioType = "fifo";
          }
        }
        continue;
      }

      size_t keyEnd = line.find_first_of(" \t", first);
      if (keyEnd == std::string::npos)
        continue;

      std::string key = line.substr(first, keyEnd - first);
      size_t valStart = line.find_first_not_of(" \t", keyEnd);
      if (valStart == std::string::npos)
        continue;

      std::string val = line.substr(valStart);
      size_t comment = val.find('#');
      if (comment != std::string::npos) {
        val = val.substr(0, comment);
      }
      size_t valLast = val.find_last_not_of(" \t");
      if (valLast != std::string::npos) {
        val = val.substr(0, valLast + 1);
      }

      if (val.length() >= 2 && val.front() == '"' && val.back() == '"') {
        val = val.substr(1, val.length() - 2);
      }

      settings[key] = val;
    }

    if (settings.find("auto_update") == settings.end())
      settings["auto_update"] = "yes";
    if (settings.find("restore_paused") == settings.end())
      settings["restore_paused"] = "yes";

    return settings;
  }

  void
  saveMpdConfig(const std::string &path,
                const std::unordered_map<std::string, std::string> &settings) {
    std::ofstream file(path);
    if (!file.is_open())
      return;

    file << "# Autogenerated by HyprMusic\n\n";

    std::vector<std::string> keys = {
        "music_directory", "playlist_directory", "db_file",
        "log_file",        "pid_file",           "state_file",
        "sticker_file",    "bind_to_address",    "port",
        "restore_paused",  "auto_update"};

    for (const auto &key : keys) {
      auto it = settings.find(key);
      if (it != settings.end() && !it->second.empty()) {
        file << key << " \"" << it->second << "\"\n";
      }
    }

    auto pulseIt = settings.find("audio_output_pulse");
    if (pulseIt != settings.end() && pulseIt->second == "yes") {
      file << "\n# Audio Output for PipeWire (via Pulse plugin)\n"
           << "audio_output {\n"
           << "        type            \"pulse\"\n"
           << "        name            \"PipeWire Sound Server\"\n"
           << "}\n";
    }

    auto fifoIt = settings.find("audio_output_fifo");
    if (fifoIt != settings.end() && fifoIt->second == "yes") {
      file << "\n# Optional: Visualizer output\n"
           << "audio_output {\n"
           << "    type                    \"fifo\"\n"
           << "    name                    \"my_fifo\"\n"
           << "    path                    \"/tmp/mpd.fifo\"\n"
           << "    format                  \"44100:16:2\"\n"
           << "}\n";
    }
    file.close();
  }

  void ensureMpdRunningAndConfigured() {
    struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);
    bool isConnected =
        conn && (mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS);
    if (conn) {
      mpd_connection_free(conn);
    }

    if (isConnected) {
      m_mpdSettings = parseMpdConfig(getMpdConfPath());
      return;
    }

    std::string confPath = getMpdConfPath();
    std::filesystem::path p(confPath);
    if (!std::filesystem::exists(p)) {
      std::filesystem::create_directories(p.parent_path());

      std::string homeDir = getUserHomeDir();

      std::filesystem::create_directories(homeDir + "/.config/mpd/playlists");
      std::filesystem::create_directories(homeDir + "/Music");

      std::ofstream file(confPath);
      if (file.is_open()) {
        file << "# Files and directories\n"
             << "music_directory     \"" << homeDir << "/Music\"\n"
             << "playlist_directory  \"" << homeDir
             << "/.config/mpd/playlists\"\n"
             << "db_file             \"" << homeDir
             << "/.config/mpd/database\"\n"
             << "log_file            \"" << homeDir << "/.config/mpd/log\"\n"
             << "pid_file            \"" << homeDir << "/.config/mpd/pid\"\n"
             << "state_file          \"" << homeDir << "/.config/mpd/state\"\n"
             << "sticker_file        \"" << homeDir
             << "/.config/mpd/sticker.sql\"\n\n"
             << "# Network\n"
             << "bind_to_address     \"127.0.0.1\"\n"
             << "port                \"6600\"\n"
             << "restore_paused      \"yes\"\n"
             << "auto_update         \"yes\"\n\n"
             << "# Audio Output for PipeWire (via Pulse plugin)\n"
             << "audio_output {\n"
             << "        type            \"pulse\"\n"
             << "        name            \"PipeWire Sound Server\"\n"
             << "}\n\n"
             << "# Optional: Visualizer output\n"
             << "audio_output {\n"
             << "    type                    \"fifo\"\n"
             << "    name                    \"my_fifo\"\n"
             << "    path                    \"/tmp/mpd.fifo\"\n"
             << "    format                  \"44100:16:2\"\n"
             << "}\n";
        file.close();
      }
    }

    m_mpdSettings = parseMpdConfig(confPath);

    std::string startCmd = "mpd " + confPath + " >/dev/null 2>&1";
    int ret = std::system(startCmd.c_str());
    (void)ret;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // ==========================================
  // MPD HELPER & CONTROLS
  // =============================================================
  void runMpdCommand(const std::function<void(struct mpd_connection *)> &cmd) {
    struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);
    if (!conn) {
      std::cerr << "MPD: Failed to create connection" << std::endl;
      return;
    }

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
      std::cerr << "MPD Connection Error: "
                << mpd_connection_get_error_message(conn) << std::endl;
      mpd_connection_free(conn);
      return;
    }

    cmd(conn);
    mpd_connection_free(conn);
  }

  void toggleMpdPlayPause() {
    runMpdCommand([this](struct mpd_connection *conn) {
      struct mpd_status *status = mpd_run_status(conn);
      if (status) {
        enum mpd_state state = mpd_status_get_state(status);
        bool pause = (state == MPD_STATE_PLAY);
        mpd_run_pause(conn, pause);
        mpd_status_free(status);
        std::cout << "MPD: Toggled pause state." << std::endl;
      }
    });
    updateStatus();
  }

  void prevMpdTrack() {
    runMpdCommand([](struct mpd_connection *conn) {
      mpd_run_previous(conn);
      std::cout << "MPD: Prev track." << std::endl;
    });
    updateStatus();
  }

  void nextMpdTrack() {
    runMpdCommand([](struct mpd_connection *conn) {
      mpd_run_next(conn);
      std::cout << "MPD: Next track." << std::endl;
    });
    updateStatus();
  }

  void playMpdSongId(int songId) {
    runMpdCommand([songId](struct mpd_connection *conn) {
      mpd_run_play_id(conn, songId);
      std::cout << "MPD: Play song ID " << songId << std::endl;
    });
    updateStatus();
  }

  void switchViewMode(eViewMode mode) {
    if (m_viewMode == mode)
      return;
    m_viewMode = mode;
    m_selectedPlaylist = "";
    m_playlistsDetailedView = false;
    m_playlistsLeftItemsLayout = nullptr;
    m_playlistsRightItemsLayout = nullptr;
    m_dbContentLayout = nullptr;
    m_queueContentLayout = nullptr;
    m_settingsContentLayout = nullptr;
    m_helpContentLayout = nullptr;

    if (m_queueTabLine) {
      m_queueTabLine->rebuild()
          ->color([this] {
            auto palette = m_palette;
            return (m_viewMode == VIEW_QUEUE)
                       ? (palette ? palette->m_colors.accent
                                  : CHyprColor(0.2, 0.8, 0.4, 1.0))
                       : CHyprColor(0, 0, 0, 0);
          })
          ->commence();
    }
    if (m_dbTabLine) {
      m_dbTabLine->rebuild()
          ->color([this] {
            auto palette = m_palette;
            return (m_viewMode == VIEW_DATABASE)
                       ? (palette ? palette->m_colors.accent
                                  : CHyprColor(0.2, 0.8, 0.4, 1.0))
                       : CHyprColor(0, 0, 0, 0);
          })
          ->commence();
    }
    if (m_playlistsTabLine) {
      m_playlistsTabLine->rebuild()
          ->color([this] {
            auto palette = m_palette;
            return (m_viewMode == VIEW_PLAYLISTS)
                       ? (palette ? palette->m_colors.accent
                                  : CHyprColor(0.2, 0.8, 0.4, 1.0))
                       : CHyprColor(0, 0, 0, 0);
          })
          ->commence();
    }
    if (m_ytdlpTabLine) {
      m_ytdlpTabLine->rebuild()
          ->color([this] {
            auto palette = m_palette;
            return (m_viewMode == VIEW_YTDLP)
                       ? (palette ? palette->m_colors.accent
                                  : CHyprColor(0.2, 0.8, 0.4, 1.0))
                       : CHyprColor(0, 0, 0, 0);
          })
          ->commence();
    }
    if (m_settingsTabLine) {
      m_settingsTabLine->rebuild()
          ->color([this] {
            auto palette = m_palette;
            return (m_viewMode == VIEW_SETTINGS)
                       ? (palette ? palette->m_colors.accent
                                  : CHyprColor(0.2, 0.8, 0.4, 1.0))
                       : CHyprColor(0, 0, 0, 0);
          })
          ->commence();
    }
    if (m_helpTabLine) {
      m_helpTabLine->rebuild()
          ->color([this] {
            auto palette = m_palette;
            return (m_viewMode == VIEW_HELP)
                       ? (palette ? palette->m_colors.accent
                                  : CHyprColor(0.2, 0.8, 0.4, 1.0))
                       : CHyprColor(0, 0, 0, 0);
          })
          ->commence();
    }

    m_playlistLoaded = false;
    updateStatus();
  }

  void showNotification(const std::string &msg) {
    m_notificationManager.showNotification(msg);
  }

  void showRenameDialog(const std::string &oldName) {
    if (!m_window)
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;
    m_renameInputText = oldName;

    auto windowSize = m_window->pixelSize();
    double popupWidth = 340.0;
    double popupHeight = 170.0;
    double posX = std::max(0.0, (windowSize.x - popupWidth) / 2.0);
    double posY = std::max(0.0, (windowSize.y - popupHeight) / 2.0);

    auto popupWindow =
        CWindowBuilder::begin()
            ->type(HT_WINDOW_POPUP)
            ->parent(m_window)
            ->pos(Hyprutils::Math::Vector2D(posX, posY))
            ->preferredSize(Hyprutils::Math::Vector2D(popupWidth, popupHeight))
            ->commence();

    if (!popupWindow)
      return;

    auto root =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.base
                             : CHyprColor(0.12, 0.12, 0.12, 1.0);
            })
            ->rounding(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    popupWindow->m_rootElement = root;

    auto cardLayout =
        CColumnLayoutBuilder::begin()
            ->gap(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    cardLayout->setMargin(15);

    auto headerText =
        CTextBuilder::begin()
            ->text("✏️ Rename Playlist")
            ->color([palette] {
              return palette ? palette->m_colors.accent
                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    cardLayout->addChild(headerText);

    auto nameInput =
        CTextboxBuilder::begin()
            ->placeholder("Playlist name...")
            ->defaultText(std::string(oldName))
            ->onTextEdited(
                [this](CSharedPointer<CTextboxElement>,
                       const std::string &text) { m_renameInputText = text; })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
            ->commence();
    cardLayout->addChild(nameInput);

    auto btnRow =
        CRowLayoutBuilder::begin()
            ->gap(20)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

    auto cancelBtn =
        CTextBuilder::begin()
            ->text("Cancel")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(0.7, 0.7, 0.7, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    cancelBtn->setReceivesMouse(true);
    cancelBtn->setMouseButton(
        [popupWindow](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            if (popupWindow)
              popupWindow->close();
          }
        });
    btnRow->addChild(cancelBtn);

    auto okBtn =
        CTextBuilder::begin()
            ->text("Ok")
            ->color([palette] {
              return palette ? palette->m_colors.accent
                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    okBtn->setReceivesMouse(true);
    okBtn->setMouseButton(
        [this, popupWindow, oldName](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            std::string newName = m_renameInputText;
            if (!newName.empty() && newName != oldName) {
              runMpdCommand([oldName, newName](struct mpd_connection *conn) {
                mpd_run_rename(conn, oldName.c_str(), newName.c_str());
              });
              if (m_selectedPlaylist == oldName) {
                m_selectedPlaylist = newName;
              }
              m_backend->addTimer(
                  std::chrono::milliseconds(100),
                  [this](CAtomicSharedPointer<CTimer>, void *) {
                    runMpdCommand([this](struct mpd_connection *conn) {
                      rebuildPlaylistsLeftItems(conn);
                      rebuildPlaylistsRightItems(conn);
                    });
                  },
                  nullptr);
            }
            if (popupWindow)
              popupWindow->close();
          }
        });
    btnRow->addChild(okBtn);
    cardLayout->addChild(btnRow);
    root->addChild(cardLayout);

    popupWindow->open();
  }

  void showCreatePlaylistDialog() {
    if (!m_window)
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;
    m_newPlaylistNameInput = "";

    auto windowSize = m_window->pixelSize();
    double popupWidth = 340.0;
    double popupHeight = 170.0;
    double posX = std::max(0.0, (windowSize.x - popupWidth) / 2.0);
    double posY = std::max(0.0, (windowSize.y - popupHeight) / 2.0);

    auto popupWindow =
        CWindowBuilder::begin()
            ->type(HT_WINDOW_POPUP)
            ->parent(m_window)
            ->pos(Hyprutils::Math::Vector2D(posX, posY))
            ->preferredSize(Hyprutils::Math::Vector2D(popupWidth, popupHeight))
            ->commence();

    if (!popupWindow)
      return;

    auto root =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.base
                             : CHyprColor(0.12, 0.12, 0.12, 1.0);
            })
            ->rounding(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    popupWindow->m_rootElement = root;

    auto cardLayout =
        CColumnLayoutBuilder::begin()
            ->gap(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    cardLayout->setMargin(15);

    auto headerText =
        CTextBuilder::begin()
            ->text("➕ Create New Playlist")
            ->color([palette] {
              return palette ? palette->m_colors.accent
                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    cardLayout->addChild(headerText);

    auto nameInput =
        CTextboxBuilder::begin()
            ->placeholder("Playlist name...")
            ->defaultText("")
            ->onTextEdited([this](CSharedPointer<CTextboxElement>,
                                  const std::string &text) {
              m_newPlaylistNameInput = text;
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
            ->commence();
    cardLayout->addChild(nameInput);

    auto btnRow =
        CRowLayoutBuilder::begin()
            ->gap(20)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

    auto cancelBtn =
        CTextBuilder::begin()
            ->text("Cancel")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(0.7, 0.7, 0.7, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    cancelBtn->setReceivesMouse(true);
    cancelBtn->setMouseButton(
        [popupWindow](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            if (popupWindow)
              popupWindow->close();
          }
        });
    btnRow->addChild(cancelBtn);

    auto okBtn =
        CTextBuilder::begin()
            ->text("Ok")
            ->color([palette] {
              return palette ? palette->m_colors.accent
                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    okBtn->setReceivesMouse(true);
    okBtn->setMouseButton(
        [this, popupWindow](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            std::string plName = m_newPlaylistNameInput;
            if (!plName.empty()) {
              m_selectedPlaylist = plName;
              m_playlistsDetailedView = true;
              m_playlistLoaded = false;
              runMpdCommand([plName](struct mpd_connection *conn) {
                mpd_run_save(conn, plName.c_str());
                mpd_run_playlist_clear(conn, plName.c_str());
              });
              m_newPlaylistNameInput = "";
              m_backend->addTimer(
                  std::chrono::milliseconds(100),
                  [this, plName](CAtomicSharedPointer<CTimer>, void *) {
                    showNotification("Created " + plName);
                    updateStatus();
                  },
                  nullptr);
            }
            if (popupWindow)
              popupWindow->close();
          }
        });
    btnRow->addChild(okBtn);
    cardLayout->addChild(btnRow);
    root->addChild(cardLayout);

    popupWindow->open();
  }

  void showPlaylistSelectionDialog(const std::string &songUri,
                                   int moveFromSongPos = -1) {
    if (!m_window || songUri.empty())
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;

    std::vector<std::string> playlists;
    runMpdCommand([&playlists](struct mpd_connection *conn) {
      if (conn && mpd_send_list_playlists(conn)) {
        struct mpd_playlist *pl;
        while ((pl = mpd_recv_playlist(conn)) != NULL) {
          const char *name = mpd_playlist_get_path(pl);
          if (name) {
            playlists.push_back(std::string(name));
          }
          mpd_playlist_free(pl);
        }
        mpd_response_finish(conn);
      }
    });

    if (playlists.empty()) {
      showNotification("No playlists available");
      return;
    }

    auto windowSize = m_window->pixelSize();
    double popupWidth = 320.0;
    double popupHeight = std::min(380.0, 60.0 + playlists.size() * 42.0 + 50.0);
    double posX = std::max(0.0, (windowSize.x - popupWidth) / 2.0);
    double posY = std::max(0.0, (windowSize.y - popupHeight) / 2.0);

    auto popupWindow =
        CWindowBuilder::begin()
            ->type(HT_WINDOW_POPUP)
            ->parent(m_window)
            ->pos(Hyprutils::Math::Vector2D(posX, posY))
            ->preferredSize(Hyprutils::Math::Vector2D(popupWidth, popupHeight))
            ->commence();

    if (!popupWindow)
      return;

    auto root =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.base
                             : CHyprColor(0.12, 0.12, 0.12, 0.98);
            })
            ->rounding(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    popupWindow->m_rootElement = root;

    auto cardLayout =
        CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    cardLayout->setMargin(12);

    auto headerText =
        CTextBuilder::begin()
            ->text((moveFromSongPos >= 0) ? "📁 Move to Playlist"
                                          : "📁 Add to Playlist")
            ->color([palette] {
              return palette ? palette->m_colors.accent
                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    cardLayout->addChild(headerText);

    auto scrollArea =
        CScrollAreaBuilder::begin()
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->scrollY(true)
            ->commence();
    scrollArea->setGrow(true);

    auto listLayout =
        CColumnLayoutBuilder::begin()
            ->gap(6)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

    for (const auto &plName : playlists) {
      auto plItem = CRectangleBuilder::begin()
                        ->color([palette] {
                          return palette ? palette->m_colors.alternateBase
                                         : CHyprColor(0.2, 0.2, 0.2, 1.0);
                        })
                        ->rounding(6)
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {1.0F, 36.0F}))
                        ->commence();

      auto plRow =
          CRowLayoutBuilder::begin()
              ->gap(8)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->commence();
      plRow->setMargin(6);

      auto itemText =
          CTextBuilder::begin()
              ->text(std::string("📁 " + plName))
              ->color([palette] {
                return palette ? palette->m_colors.text
                               : CHyprColor(1, 1, 1, 1);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->align(HT_FONT_ALIGN_LEFT)
              ->commence();
      itemText->setGrow(true);
      plRow->addChild(itemText);
      plItem->addChild(plRow);

      plItem->setReceivesMouse(true);
      plItem->setMouseButton([this, popupWindow, plName, songUri,
                              moveFromSongPos](Input::eMouseButton button,
                                               bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          std::string currentPl = m_selectedPlaylist;
          std::thread([this, plName, songUri, moveFromSongPos, currentPl]() {
            std::string finalUri = songUri;
            if (songUri.rfind("http://", 0) == 0 ||
                songUri.rfind("https://", 0) == 0) {
              if (songUri.find("youtube.com") != std::string::npos ||
                  songUri.find("youtu.be") != std::string::npos) {
                m_backend->addTimer(
                    std::chrono::milliseconds(1),
                    [this](CAtomicSharedPointer<CTimer>, void *) {
                      showNotification("⏳ Resolving YouTube stream...");
                    },
                    nullptr);
                std::string resTitle = "Stream Track";
                std::string resUploader = "";
                {
                  std::lock_guard<std::mutex> lock(m_ytdlpMutex);
                  for (const auto &res : m_ytdlpResults) {
                    if (res.url == songUri) {
                      resTitle = res.title;
                      resUploader = res.uploader;
                      break;
                    }
                  }
                }
                std::string realUrl = extractDirectStreamUrl(songUri);
                if (!realUrl.empty()) {
                  finalUri = realUrl;
                  setUrlTitle(realUrl, resTitle, resUploader);
                } else {
                  m_backend->addTimer(
                      std::chrono::milliseconds(1),
                      [this](CAtomicSharedPointer<CTimer>, void *) {
                        showNotification("❌ Failed to resolve stream");
                      },
                      nullptr);
                  return;
                }
              }
            }

            runMpdCommand([this, plName, finalUri, moveFromSongPos,
                           currentPl](struct mpd_connection *conn) {
              bool alreadyInPlaylist = false;
              if (conn && mpd_send_list_playlist_meta(conn, plName.c_str())) {
                struct mpd_song *s;
                while ((s = mpd_recv_song(conn)) != NULL) {
                  const char *pUri = mpd_song_get_uri(s);
                  if (pUri && std::string(pUri) == finalUri) {
                    alreadyInPlaylist = true;
                  }
                  mpd_song_free(s);
                }
                mpd_response_finish(conn);
              }

              if (alreadyInPlaylist) {
                m_backend->addTimer(
                    std::chrono::milliseconds(1),
                    [this, plName](CAtomicSharedPointer<CTimer>, void *) {
                      showNotification("Already added to " + plName);
                    },
                    nullptr);
              } else {
                mpd_run_playlist_add(conn, plName.c_str(), finalUri.c_str());
                if (moveFromSongPos >= 0 && !currentPl.empty()) {
                  mpd_run_playlist_delete(conn, currentPl.c_str(),
                                          moveFromSongPos);
                }
                m_backend->addTimer(
                    std::chrono::milliseconds(100),
                    [this, plName](CAtomicSharedPointer<CTimer>, void *) {
                      showNotification("Added to " + plName);
                      updateStatus();
                      if (m_viewMode == VIEW_PLAYLISTS) {
                        runMpdCommand([this](struct mpd_connection *conn) {
                          rebuildPlaylistsRightItems(conn);
                        });
                      }
                    },
                    nullptr);
              }
            });
          }).detach();

          if (popupWindow)
            popupWindow->close();
        }
      });

      listLayout->addChild(plItem);
    }

    scrollArea->addChild(listLayout);
    cardLayout->addChild(scrollArea);

    auto cancelRow =
        CRowLayoutBuilder::begin()
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 30.0F}))
            ->commence();

    auto cancelBtn =
        CTextBuilder::begin()
            ->text("Cancel")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(0.7, 0.7, 0.7, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->align(HT_FONT_ALIGN_CENTER)
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    cancelBtn->setReceivesMouse(true);
    cancelBtn->setMouseButton(
        [popupWindow](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            if (popupWindow)
              popupWindow->close();
          }
        });
    cancelRow->addChild(cancelBtn);
    cardLayout->addChild(cancelRow);

    root->addChild(cardLayout);
    popupWindow->open();
  }
  static std::unordered_set<std::string> getQueueUris(struct mpd_connection *conn) {
    return MPDUtils::getQueueUris(conn);
  }

  static std::unordered_set<std::string> getPlaylistUris(struct mpd_connection *conn, const std::string &playlistName) {
    return MPDUtils::getPlaylistUris(conn, playlistName);
  }

  void showQueueAddItemDialog() {
    UI::Dialogs::AddItemDialogContext ctx;
    ctx.targetType = UI::Dialogs::AddItemTargetType::QUEUE;
    ctx.window = m_window;
    ctx.backend = m_backend;
    ctx.palette = m_palette;
    ctx.fontFamily = m_fontFamily;
    ctx.runMpdCommand = [this](std::function<void(struct mpd_connection *)> cmd) {
      runMpdCommand(cmd);
    };
    ctx.showNotification = [this](const std::string &msg) {
      showNotification(msg);
    };
    ctx.addSongToQueue = [this](const std::string &uri) {
      addSongToQueue(uri);
    };
    ctx.refreshCallback = [this]() {
      runMpdCommand([this](struct mpd_connection *conn) {
        struct mpd_status *status = mpd_run_status(conn);
        int activeSongId = -1;
        if (status) {
          activeSongId = mpd_status_get_song_id(status);
          mpd_status_free(status);
        }
        populateQueueSongs(conn, activeSongId);
      });
    };
    UI::Dialogs::showAddItemDialog(ctx);
  }

  void showPlaylistAddItemDialog(const std::string &playlistName) {
    UI::Dialogs::AddItemDialogContext ctx;
    ctx.targetType = UI::Dialogs::AddItemTargetType::PLAYLIST;
    ctx.targetPlaylistName = playlistName;
    ctx.window = m_window;
    ctx.backend = m_backend;
    ctx.palette = m_palette;
    ctx.fontFamily = m_fontFamily;
    ctx.runMpdCommand = [this](std::function<void(struct mpd_connection *)> cmd) {
      runMpdCommand(cmd);
    };
    ctx.showNotification = [this](const std::string &msg) {
      showNotification(msg);
    };
    ctx.addSongToQueue = [this](const std::string &uri) {
      addSongToQueue(uri);
    };
    ctx.refreshCallback = [this]() {
      runMpdCommand([this](struct mpd_connection *conn) {
        rebuildPlaylistsRightItems(conn);
      });
    };
    UI::Dialogs::showAddItemDialog(ctx);
  }

  std::string sanitizePlaylistName(const std::string &name) {
    std::string sanitized = "";
    for (char c : name) {
      if (std::isalnum(c) || c == ' ' || c == '-' || c == '_') {
        sanitized += c;
      }
    }
    if (sanitized.empty()) {
      sanitized = "Imported YouTube Playlist";
    }
    return sanitized;
  }

  void loadYoutubePlaylist(const std::vector<YtDlpResult> &tracks,
                           bool startPlaying,
                           const std::string &saveToPlaylistName = "") {
    if (tracks.empty())
      return;

    std::string displayPlName = saveToPlaylistName;
    if (!saveToPlaylistName.empty()) {
      displayPlName = sanitizePlaylistName(saveToPlaylistName);
    }

    std::thread([this, tracks, startPlaying, displayPlName]() {
      m_backend->addTimer(
          std::chrono::milliseconds(1),
          [this, displayPlName, tracks,
           startPlaying](CAtomicSharedPointer<CTimer>, void *) {
            if (!displayPlName.empty()) {
              showNotification("⏳ Saving YouTube playlist '" + displayPlName +
                               "'...");
            } else {
              showNotification("⏳ Resolving YouTube playlist (" +
                               std::to_string(tracks.size()) + " tracks)...");
            }
          },
          nullptr);

      if (startPlaying && displayPlName.empty()) {
        runMpdCommand([](struct mpd_connection *conn) { mpd_run_clear(conn); });
      }

      bool first = true;
      int resolvedCount = 0;
      for (const auto &track : tracks) {
        std::string realUrl = extractDirectStreamUrl(track.url);
        if (realUrl.empty())
          continue;

        setUrlTitle(realUrl, track.title, track.uploader);

        if (!displayPlName.empty()) {
          runMpdCommand([displayPlName, realUrl](struct mpd_connection *conn) {
            mpd_run_playlist_add(conn, displayPlName.c_str(), realUrl.c_str());
          });
        } else {
          runMpdCommand([this, realUrl, startPlaying,
                         first](struct mpd_connection *conn) {
            int id = mpd_run_add_id(conn, realUrl.c_str());
            if (first && startPlaying) {
              if (id >= 0) {
                mpd_run_play_id(conn, id);
              } else {
                if (mpd_run_add(conn, realUrl.c_str())) {
                  mpd_run_play(conn);
                }
              }
            }
          });
          first = false;
        }
        resolvedCount++;
      }

      m_backend->addTimer(
          std::chrono::milliseconds(100),
          [this, displayPlName, resolvedCount](CAtomicSharedPointer<CTimer>,
                                               void *) {
            updateStatus();
            if (!displayPlName.empty()) {
              showNotification("Saved YouTube playlist '" + displayPlName +
                               "' (" + std::to_string(resolvedCount) +
                               " tracks)");
              runMpdCommand([this](struct mpd_connection *conn) {
                rebuildPlaylistsLeftItems(conn);
              });
            } else {
              showNotification("YouTube Playlist added (" +
                               std::to_string(resolvedCount) + " tracks)");
            }
          },
          nullptr);
    }).detach();
  }

  void addSongToQueue(const std::string &uri) {
    if (uri.empty())
      return;

    runMpdCommand([this, uri](struct mpd_connection *conn) {
      bool alreadyInQueue = false;
      if (conn && mpd_send_list_queue_meta(conn)) {
        struct mpd_song *s;
        while ((s = mpd_recv_song(conn)) != NULL) {
          const char *qUri = mpd_song_get_uri(s);
          if (qUri && std::string(qUri) == uri) {
            alreadyInQueue = true;
          }
          mpd_song_free(s);
        }
        mpd_response_finish(conn);
      }

      if (alreadyInQueue) {
        std::cout << "MPD: Song already in queue: " << uri << std::endl;
        m_backend->addTimer(
            std::chrono::milliseconds(1),
            [this](CAtomicSharedPointer<CTimer>, void *) {
              showNotification("Already in que");
            },
            nullptr);
      } else {
        if (mpd_run_add(conn, uri.c_str())) {
          std::cout << "MPD: Added song to queue: " << uri << std::endl;
          m_backend->addTimer(
              std::chrono::milliseconds(1),
              [this](CAtomicSharedPointer<CTimer>, void *) {
                showNotification("Added");
              },
              nullptr);
        } else {
          std::cerr << "MPD: Failed to add song: " << uri << std::endl;
        }
      }
    });
    updateStatus();
  }

  void playSongFromUri(const std::string &uri) {
    if (uri.empty())
      return;

    runMpdCommand([this, uri](struct mpd_connection *conn) {
      int existingId = -1;
      if (conn && mpd_send_list_queue_meta(conn)) {
        struct mpd_song *s;
        while ((s = mpd_recv_song(conn)) != NULL) {
          const char *qUri = mpd_song_get_uri(s);
          if (qUri && std::string(qUri) == uri) {
            existingId = mpd_song_get_id(s);
          }
          mpd_song_free(s);
        }
        mpd_response_finish(conn);
      }

      if (existingId >= 0) {
        mpd_run_play_id(conn, existingId);
        m_backend->addTimer(
            std::chrono::milliseconds(1),
            [this](CAtomicSharedPointer<CTimer>, void *) {
              showNotification("Item already in queue, playing anyway");
            },
            nullptr);
      } else {
        int songId = mpd_run_add_id(conn, uri.c_str());
        if (songId >= 0) {
          mpd_run_play_id(conn, songId);
        }
      }
    });

    m_backend->addTimer(
        std::chrono::milliseconds(100),
        [this](CAtomicSharedPointer<CTimer>, void *) {
          updateStatus();
          if (m_viewMode == VIEW_PLAYLISTS) {
            runMpdCommand([this](struct mpd_connection *conn) {
              rebuildPlaylistsRightItems(conn);
            });
          } else if (m_viewMode == VIEW_DATABASE) {
            runMpdCommand([this](struct mpd_connection *conn) {
              populateDatabaseSongs(conn);
            });
          }
        },
        nullptr);
  }

  void removeSongFromQueue(int songId) {
    runMpdCommand([songId](struct mpd_connection *conn) {
      if (mpd_run_delete_id(conn, songId)) {
        std::cout << "MPD: Removed song ID " << songId << " from queue"
                  << std::endl;
      } else {
        std::cerr << "MPD: Failed to remove song ID " << songId << " from queue"
                  << std::endl;
      }
    });
    m_backend->addTimer(
        std::chrono::milliseconds(1),
        [this](CAtomicSharedPointer<CTimer>, void *) { updateStatus(); },
        nullptr);
  }

  CSharedPointer<CComboboxElement>
  createPlaylistDropdown(struct mpd_connection *conn) {
    std::vector<std::string> playlists;
    if (conn && mpd_send_list_playlists(conn)) {
      struct mpd_playlist *pl;
      while ((pl = mpd_recv_playlist(conn)) != NULL) {
        const char *name = mpd_playlist_get_path(pl);
        if (name) {
          playlists.push_back(std::string(name));
        }
        mpd_playlist_free(pl);
      }
      mpd_response_finish(conn);
    }

    if (playlists.empty()) {
      playlists.push_back("(No Playlists)");
      m_selectedPlaylist = "";
    } else {
      if (m_selectedPlaylist.empty() ||
          std::find(playlists.begin(), playlists.end(), m_selectedPlaylist) ==
              playlists.end()) {
        m_selectedPlaylist = playlists[0];
      }
    }

    size_t currentIdx = 0;
    for (size_t i = 0; i < playlists.size(); ++i) {
      if (playlists[i] == m_selectedPlaylist) {
        currentIdx = i;
        break;
      }
    }

    std::vector<std::string> itemsCopy = playlists;
    auto combo =
        CComboboxBuilder::begin()
            ->items(std::move(itemsCopy))
            ->currentItem(currentIdx)
            ->onChanged([this, playlists](CSharedPointer<CComboboxElement>,
                                          size_t idx) {
              if (idx < playlists.size() &&
                  playlists[idx] != "(No Playlists)") {
                m_selectedPlaylist = playlists[idx];
                m_backend->addTimer(
                    std::chrono::milliseconds(50),
                    [this](CAtomicSharedPointer<CTimer>, void *) {
                      runMpdCommand([this](struct mpd_connection *c) {
                        if (m_viewMode == VIEW_PLAYLISTS)
                          rebuildPlaylistsUI(c);
                        else if (m_viewMode == VIEW_DATABASE)
                          rebuildDatabaseUI(c);
                      });
                    },
                    nullptr);
              }
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {0.3F, 35.0F}))
            ->commence();
    return combo;
  }

  void triggerYtDlpSearch() {
    if (m_ytdlpSearchTitle.empty() || m_ytdlpSearching)
      return;

    m_ytdlpSearching = true;
    m_playlistLoaded = false;

    // Reset playlist detection
    m_ytdlpIsPlaylist = false;
    m_ytdlpPlaylistTitle = "";
    m_ytdlpPlaylistId = "";

    std::string title = m_ytdlpSearchTitle;
    int count = 5;
    try {
      count = std::stoi(m_ytdlpResultCount);
    } catch (...) {
      count = 5;
    }
    if (count < 1)
      count = 1;
    if (count > 50)
      count = 50;

    // Check if the query is a direct playlist URL
    bool isPlaylistUrl = (title.find("list=") != std::string::npos ||
                          title.find("playlist") != std::string::npos);

    runMpdCommand([this](struct mpd_connection *) { rebuildYtDlpUI(); });

    std::thread([this, title, count, isPlaylistUrl]() {
      std::string escapedTitle = escapeShellArg(title);
      std::string cmd;
      if (isPlaylistUrl) {
        cmd = "yt-dlp --flat-playlist -j " + escapedTitle + " 2>/dev/null";
      } else {
        cmd = "yt-dlp --flat-playlist -j \"ytsearch" + std::to_string(count) +
              ":" + escapedTitle + "\" 2>/dev/null";
      }

      FILE *pipe = popen(cmd.c_str(), "r");
      std::vector<YtDlpResult> results;
      std::string detectedPlTitle = "";
      std::string detectedPlId = "";

      if (pipe) {
        char buffer[8192];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
          std::string line(buffer);
          YtDlpResult res;
          res.title = getJsonStringField(line, "title");
          res.uploader = getJsonStringField(line, "uploader");
          if (res.uploader.empty())
            res.uploader = getJsonStringField(line, "channel");
          res.url = getJsonStringField(line, "url");
          if (res.url.empty())
            res.url = getJsonStringField(line, "webpage_url");
          res.id = getJsonStringField(line, "id");
          if (res.url.empty() && !res.id.empty()) {
            res.url = "https://www.youtube.com/watch?v=" + res.id;
          }
          res.duration = getJsonDuration(line);
          if (!res.title.empty() && !res.url.empty()) {
            results.push_back(res);
          }

          if (isPlaylistUrl && detectedPlTitle.empty()) {
            detectedPlTitle = getJsonStringField(line, "playlist_title");
            detectedPlId = getJsonStringField(line, "playlist_id");
          }
        }
        pclose(pipe);
      }

      {
        std::lock_guard<std::mutex> lock(m_ytdlpMutex);
        m_ytdlpResults = results;
        if (isPlaylistUrl) {
          m_ytdlpIsPlaylist = true;
          m_ytdlpPlaylistTitle = detectedPlTitle.empty()
                                     ? "Imported YouTube Playlist"
                                     : detectedPlTitle;
          m_ytdlpPlaylistId = detectedPlId;
        }
        m_ytdlpSearching = false;
        m_ytdlpNeedRebuild = true;
      }
    }).detach();
  }

  void rebuildYtDlpUI(struct mpd_connection *conn = nullptr) {
    if (!m_tabContentWrapper)
      return;

    if (!conn) {
      runMpdCommand([this](struct mpd_connection *c) { rebuildYtDlpUI(c); });
      return;
    }

    m_tabContentWrapper->clearChildren();

    auto tabMainLayout =
        CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    m_tabContentWrapper->addChild(tabMainLayout);

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;

    auto topControlsCol =
        CColumnLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    topControlsCol->setMargin(10);

    // Row 1: Centered Search Bar
    auto row1 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
            ->commence();

    auto leftSpacer1 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.35F, 1.0F}))
            ->commence();
    row1->addChild(leftSpacer1);

    auto titleInput =
        CTextboxBuilder::begin()
            ->placeholder("Search")
            ->defaultText(std::string(m_ytdlpSearchTitle))
            ->onTextEdited(
                [this](CSharedPointer<CTextboxElement>,
                       const std::string &text) { m_ytdlpSearchTitle = text; })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.30F, 1.0F}))
            ->commence();
    row1->addChild(titleInput);

    auto rightSpacer1 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.35F, 1.0F}))
            ->commence();
    row1->addChild(rightSpacer1);

    topControlsCol->addChild(row1);

    // Row 2: Centered "Result count expected" label + input box
    auto row2 =
        CRowLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
            ->commence();

    auto leftSpacer2 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.35F, 1.0F}))
            ->commence();
    row2->addChild(leftSpacer2);

    auto countLabel =
        CTextBuilder::begin()
            ->text("Result count")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(0.8, 0.8, 0.8, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    row2->addChild(countLabel);

    auto countInput =
        CTextboxBuilder::begin()
            ->placeholder("e.g. 5")
            ->defaultText(std::string(m_ytdlpResultCount))
            ->onTextEdited(
                [this](CSharedPointer<CTextboxElement>,
                       const std::string &text) { m_ytdlpResultCount = text; })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.10F, 1.0F}))
            ->commence();
    row2->addChild(countInput);

    auto rightSpacer2 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.35F, 1.0F}))
            ->commence();
    row2->addChild(rightSpacer2);

    topControlsCol->addChild(row2);

    // Row 3: Centered Submit button
    auto row3 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
            ->commence();

    auto leftSpacer3 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.40F, 1.0F}))
            ->commence();
    row3->addChild(leftSpacer3);

    auto submitBtn = CButtonBuilder::begin()
                         ->label("Submit")
                         ->alignText(HT_FONT_ALIGN_CENTER)
                         ->fontFamily(std::string(fontFamily))
                         ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                         ->onMainClick([this](CSharedPointer<CButtonElement>) {
                           triggerYtDlpSearch();
                         })
                         ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                             CDynamicSize::HT_SIZE_ABSOLUTE,
                                             {120.0F, 32.0F}))
                         ->commence();
    row3->addChild(submitBtn);

    auto rightSpacer3 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.40F, 1.0F}))
            ->commence();
    row3->addChild(rightSpacer3);

    topControlsCol->addChild(row3);

    tabMainLayout->addChild(topControlsCol);

    auto scrollArea =
        CScrollAreaBuilder::begin()
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.90F}))
            ->scrollY(true)
            ->commence();

    auto tabContentLayout =
        CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    tabContentLayout->setMargin(5);
    scrollArea->addChild(tabContentLayout);
    tabMainLayout->addChild(scrollArea);

    if (m_ytdlpSearching) {
      auto searchingText =
          CTextBuilder::begin()
              ->text("⏳ Searching YouTube with yt-dlp...")
              ->color([palette] {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      tabContentLayout->addChild(searchingText);
    } else {
      std::vector<YtDlpResult> results;
      {
        std::lock_guard<std::mutex> lock(m_ytdlpMutex);
        results = m_ytdlpResults;
      }

      if (results.empty()) {
        auto emptyText =
            CTextBuilder::begin()
                ->text("No search results. Enter title and result count above, "
                       "then click Go.")
                ->color([palette] {
                  return palette ? palette->m_colors.text
                                 : CHyprColor(0.7, 0.7, 0.7, 1.0);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                    CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                ->commence();
        tabContentLayout->addChild(emptyText);
      } else {
        if (m_ytdlpIsPlaylist) {
          auto plCard = CRectangleBuilder::begin()
                            ->color([palette] {
                              return palette ? palette->m_colors.alternateBase
                                             : CHyprColor(0.2, 0.2, 0.2, 1.0);
                            })
                            ->rounding(palette ? palette->m_vars.smallRounding : 5)
                            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                                {1.0F, 50.0F}))
                            ->commence();

          auto plRowLayout =
              CRowLayoutBuilder::begin()
                  ->gap(15)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_PERCENT,
                                      {1.0F, 1.0F}))
                  ->commence();
          plRowLayout->setMargin(8);

          auto plInfoCol =
              CColumnLayoutBuilder::begin()
                  ->gap(2)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->commence();

          std::string plDisplayTitle =
              "📁 YouTube Playlist: " + m_ytdlpPlaylistTitle;
          if (plDisplayTitle.length() > 65) {
            plDisplayTitle = plDisplayTitle.substr(0, 62) + "...";
          }

          auto plTitleText =
              CTextBuilder::begin()
                  ->text(std::move(plDisplayTitle))
                  ->color([palette] {
                    return palette ? palette->m_colors.accent
                                   : CHyprColor(0.2, 0.8, 0.4, 1.0);
                  })
                  ->fontFamily(std::string(fontFamily))
                  ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->commence();

          std::string plSub = "YouTube Playlist  •  " +
                              std::to_string(results.size()) + " tracks";
          auto plSubText =
              CTextBuilder::begin()
                  ->text(std::move(plSub))
                  ->color([palette] {
                    return palette ? palette->m_colors.text
                                   : CHyprColor(0.6, 0.6, 0.6, 1.0);
                  })
                  ->fontFamily(std::string(fontFamily))
                  ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->commence();

          plInfoCol->addChild(plTitleText);
          plInfoCol->addChild(plSubText);
          plInfoCol->setGrow(true);
          plRowLayout->addChild(plInfoCol);

          // Dedicated Play Button
          auto plPlayBtn =
              CTextBuilder::begin()
                  ->text("▶")
                  ->color([palette] {
                    return palette ? palette->m_colors.accent
                                   : CHyprColor(0.2, 0.8, 0.4, 1.0);
                  })
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                      CDynamicSize::HT_SIZE_ABSOLUTE,
                                      {20.0F, 30.0F}))
                  ->commence();
          plPlayBtn->setReceivesMouse(true);
          plPlayBtn->setMouseButton(
              [this, results](Input::eMouseButton button, bool down) {
                if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                  loadYoutubePlaylist(results, true);
                }
              });
          plRowLayout->addChild(plPlayBtn);

          // Actions Dropdown
          std::vector<std::string> plOptions = {"Actions", "➕ Add to Queue",
                                                "📁 Save Playlist"};
          auto plActionMenu =
              CComboboxBuilder::begin()
                  ->items(std::move(plOptions))
                  ->currentItem(0)
                  ->onChanged([this,
                               results](CSharedPointer<CComboboxElement> combo,
                                        size_t idx) {
                    if (idx == 1) { // ➕ Add to Queue
                      loadYoutubePlaylist(results, false);
                    } else if (idx == 2) { // 📁 Save Playlist
                      loadYoutubePlaylist(results, false, m_ytdlpPlaylistTitle);
                    }
                    if (combo && idx != 0) {
                      combo->setCurrent(0);
                    }
                  })
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                      CDynamicSize::HT_SIZE_ABSOLUTE,
                                      {95.0F, 28.0F}))
                  ->commence();
          plRowLayout->addChild(plActionMenu);

          plCard->addChild(plRowLayout);
          tabContentLayout->addChild(plCard);

          // Add a divider
          auto divider = CRectangleBuilder::begin()
                             ->color([palette] {
                               return palette
                                          ? palette->m_colors.alternateBase
                                          : CHyprColor(0.18, 0.18, 0.18, 1.0);
                             })
                             ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                                 CDynamicSize::HT_SIZE_ABSOLUTE,
                                                 {1.0F, 2.0F}))
                             ->commence();
          tabContentLayout->addChild(divider);
        }

        for (size_t i = 0; i < results.size(); ++i) {
          const auto &res = results[i];

          auto songItem =
              CRectangleBuilder::begin()
                  ->color([palette] {
                    return palette ? palette->m_colors.base
                                   : CHyprColor(0.15, 0.15, 0.15, 1.0);
                  })
                  ->rounding(palette ? palette->m_vars.smallRounding : 5)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_ABSOLUTE,
                                      {1.0F, 46.0F}))
                  ->commence();

          auto rowLayout = CRowLayoutBuilder::begin()
                               ->gap(15)
                               ->size(CDynamicSize(
                                   CDynamicSize::HT_SIZE_PERCENT,
                                   CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
                               ->commence();
          rowLayout->setMargin(6);

          auto infoCol =
              CColumnLayoutBuilder::begin()
                  ->gap(2)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->commence();

          std::string displayTitle = res.title;
          if (displayTitle.length() > 65) {
            displayTitle = displayTitle.substr(0, 62) + "...";
          }

          auto titleText =
              CTextBuilder::begin()
                  ->text(std::move(displayTitle))
                  ->color([palette] {
                    return palette ? palette->m_colors.text
                                   : CHyprColor(1, 1, 1, 1);
                  })
                  ->fontFamily(std::string(fontFamily))
                  ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->commence();

          std::string subStr = res.uploader;
          if (!res.duration.empty()) {
            subStr += "  •  " + res.duration;
          }

          auto subText =
              CTextBuilder::begin()
                  ->text(std::move(subStr))
                  ->color([palette] {
                    return palette ? palette->m_colors.text
                                   : CHyprColor(0.6, 0.6, 0.6, 1.0);
                  })
                  ->fontFamily(std::string(fontFamily))
                  ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->commence();

          infoCol->addChild(titleText);
          infoCol->addChild(subText);
          infoCol->setGrow(true);
          rowLayout->addChild(infoCol);

          std::string targetUrl = res.url;
          std::string resTitle = res.title;
          std::string resUploader = res.uploader;

          // Dedicated Play Button
          auto playTrackBtn =
              CTextBuilder::begin()
                  ->text("▶")
                  ->color([palette] {
                    return palette ? palette->m_colors.accent
                                   : CHyprColor(0.2, 0.8, 0.4, 1.0);
                  })
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                      CDynamicSize::HT_SIZE_ABSOLUTE,
                                      {20.0F, 28.0F}))
                  ->commence();
          playTrackBtn->setReceivesMouse(true);
          playTrackBtn->setMouseButton(
              [this, targetUrl, resTitle,
               resUploader](Input::eMouseButton button, bool down) {
                if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                  std::thread([this, targetUrl, resTitle, resUploader]() {
                    m_backend->addTimer(
                        std::chrono::milliseconds(1),
                        [this](CAtomicSharedPointer<CTimer>, void *) {
                          showNotification("⏳ Resolving YouTube stream...");
                        },
                        nullptr);
                    std::string realUrl = extractDirectStreamUrl(targetUrl);
                    if (!realUrl.empty()) {
                      setUrlTitle(realUrl, resTitle, resUploader);
                      playSongFromUri(realUrl);
                    } else {
                      m_backend->addTimer(
                          std::chrono::milliseconds(1),
                          [this](CAtomicSharedPointer<CTimer>, void *) {
                            showNotification("❌ Failed to resolve stream");
                          },
                          nullptr);
                    }
                  }).detach();
                }
              });
          rowLayout->addChild(playTrackBtn);

          // Actions Dropdown
          std::vector<std::string> songOptions = {"Actions", "➕ Add to Queue",
                                                  "📁 Add to Playlist"};
          auto songActionMenu =
              CComboboxBuilder::begin()
                  ->items(std::move(songOptions))
                  ->currentItem(0)
                  ->onChanged([this, targetUrl, resTitle, resUploader](
                                  CSharedPointer<CComboboxElement> combo,
                                  size_t idx) {
                    if (idx == 1) { // ➕ Add to Queue
                      std::thread([this, targetUrl, resTitle, resUploader]() {
                        m_backend->addTimer(
                            std::chrono::milliseconds(1),
                            [this](CAtomicSharedPointer<CTimer>, void *) {
                              showNotification(
                                  "⏳ Resolving YouTube stream...");
                            },
                            nullptr);
                        std::string realUrl = extractDirectStreamUrl(targetUrl);
                        if (!realUrl.empty()) {
                          setUrlTitle(realUrl, resTitle, resUploader);
                          addSongToQueue(realUrl);
                        } else {
                          m_backend->addTimer(
                              std::chrono::milliseconds(1),
                              [this](CAtomicSharedPointer<CTimer>, void *) {
                                showNotification("❌ Failed to resolve stream");
                              },
                              nullptr);
                        }
                      }).detach();
                    } else if (idx == 2) { // 📁 Add to Playlist
                      showPlaylistSelectionDialog(targetUrl);
                    }
                    if (combo && idx != 0) {
                      combo->setCurrent(0);
                    }
                  })
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                      CDynamicSize::HT_SIZE_ABSOLUTE,
                                      {95.0F, 28.0F}))
                  ->commence();
          rowLayout->addChild(songActionMenu);

          songItem->addChild(rowLayout);
          tabContentLayout->addChild(songItem);
        }
      }
    }

    tabContentLayout->forceReposition();
    m_tabContentWrapper->forceReposition();
  }

  void populateQueueSongs(struct mpd_connection *conn, int activeSongId) {
    if (activeSongId >= 0) {
      m_lastActiveSongId = activeSongId;
    }
    if (!m_queueContentLayout)
      return;

    m_queueContentLayout->clearChildren();
    m_queueSongTexts.clear();

    auto palette = m_palette;
    int rounding = palette ? palette->m_vars.smallRounding : 5;
    std::string fontFamily = m_fontFamily;

    if (!conn || !mpd_send_list_queue_meta(conn)) {
      std::cerr << "MPD: Failed to send list queue command" << std::endl;
      return;
    }

    bool foundAny = false;
    struct mpd_song *s;
    std::unordered_set<int> currentQueueIds;
    while ((s = mpd_recv_song(conn)) != NULL) {
      int songId = mpd_song_get_id(s);
      currentQueueIds.insert(songId);
      unsigned songPos = mpd_song_get_pos(s);

      const char *artist = mpd_song_get_tag(s, MPD_TAG_ARTIST, 0);
      const char *title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
      const char *uri = mpd_song_get_uri(s);
      std::string displayTitle;

      std::string storedTitle, storedUploader;
      if (title && strlen(title) > 0) {
        std::string artistStr = artist ? artist : "Unknown Artist";
        displayTitle = std::string(title) + " - " + artistStr;
      } else if (uri && getUrlTitle(uri, storedTitle, storedUploader)) {
        displayTitle = storedTitle;
        if (!storedUploader.empty()) {
          displayTitle += " - " + storedUploader;
        }
      } else if (uri) {
        std::string uriStr(uri);
        if (uriStr.find("googlevideo.com") != std::string::npos ||
            uriStr.find("http://") == 0 || uriStr.find("https://") == 0) {
          if (uriStr.length() > 50) {
            displayTitle = "🌐 Stream (" + uriStr.substr(0, 35) + "...)";
          } else {
            displayTitle = uriStr;
          }
        } else {
          displayTitle = uriStr;
        }
      } else {
        displayTitle = "Unknown Track";
      }

      if (!m_queueSearchQuery.empty()) {
        std::string query = m_queueSearchQuery;
        std::string titleLower = displayTitle;
        std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(),
                       ::tolower);
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);
        if (titleLower.find(query) == std::string::npos) {
          mpd_song_free(s);
          continue;
        }
      }

      foundAny = true;
      auto songItem = CRectangleBuilder::begin()
                          ->color([palette] {
                            return palette ? palette->m_colors.base
                                           : CHyprColor(0.15, 0.15, 0.15, 1.0);
                          })
                          ->rounding(rounding)
                          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                              CDynamicSize::HT_SIZE_ABSOLUTE,
                                              {1.0F, 40.0F}))
                          ->commence();

      auto rowLayout =
          CRowLayoutBuilder::begin()
              ->gap(10)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->commence();
      rowLayout->setMargin(6);

      std::string indexStr = std::to_string(songPos + 1) + ". ";
      auto songText =
          CTextBuilder::begin()
              ->text(indexStr + displayTitle)
              ->color([this, songId] {
                auto palette = m_palette;
                bool isPlaying = (songId == m_lastActiveSongId);
                if (isPlaying) {
                  return palette ? palette->m_colors.accent
                                 : CHyprColor(0.2, 0.8, 0.4, 1.0);
                }
                return palette ? palette->m_colors.text
                               : CHyprColor(1, 1, 1, 1);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->align(HT_FONT_ALIGN_LEFT)
              ->noEllipsize(false)
              ->interactable(true)
              ->commence();
      songText->setReceivesMouse(true);
      songText->setMouseButton(
          [this, songId](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && !down) {
              playMpdSongId(songId);
            }
          });
      m_queueSongTexts[songId] = songText;

      auto textContainer =
          CRowLayoutBuilder::begin()
              ->gap(0)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->commence();
      textContainer->setGrow(true);
      textContainer->addChild(songText);
      rowLayout->addChild(textContainer);

      std::vector<std::string> songOptions = {"Actions", "🗑️ Remove",
                                              "📁 Add to Playlist"};
      std::string songUriStr = uri ? uri : "";
      auto songActionMenu =
          CComboboxBuilder::begin()
              ->items(std::move(songOptions))
              ->currentItem(0)
              ->onChanged([this, songId,
                           songUriStr](CSharedPointer<CComboboxElement> combo,
                                       size_t idx) {
                if (idx == 1) { // 🗑️ Remove
                  removeSongFromQueue(songId);
                } else if (idx == 2) { // 📁 Add to Playlist
                  if (!songUriStr.empty()) {
                    showPlaylistSelectionDialog(songUriStr);
                  }
                }
                if (combo && idx != 0) {
                  combo->setCurrent(0);
                }
              })
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {95.0F, 28.0F}))
              ->commence();
      songActionMenu->setGrow(false);
      rowLayout->addChild(songActionMenu);

      songItem->addChild(rowLayout);
      m_queueContentLayout->addChild(songItem);

      mpd_song_free(s);
    }
    mpd_response_finish(conn);

    if (!foundAny) {
      auto emptyText =
          CTextBuilder::begin()
              ->text("Queue is empty")
              ->color([palette] {
                return palette ? palette->m_colors.text
                               : CHyprColor(0.6, 0.6, 0.6, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->align(HT_FONT_ALIGN_CENTER)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_queueContentLayout->addChild(emptyText);
    }

    m_queueContentLayout->forceReposition();
    m_tabContentWrapper->forceReposition();
  }

  void rebuildQueueUI(struct mpd_connection *conn, int activeSongId) {
    (void)conn;
    if (!m_tabContentWrapper)
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;

    if (!m_queueContentLayout) {
      m_tabContentWrapper->clearChildren();

      auto tabMainLayout =
          CColumnLayoutBuilder::begin()
              ->gap(10)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->commence();
      m_tabContentWrapper->addChild(tabMainLayout);

      auto topControlsCol =
          CColumnLayoutBuilder::begin()
              ->gap(8)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      topControlsCol->setMargin(10);

      auto topSearchRow =
          CRowLayoutBuilder::begin()
              ->gap(12)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {1.0F, 35.0F}))
              ->commence();

      auto searchBar =
          CTextboxBuilder::begin()
              ->placeholder("Search...")
              ->defaultText(std::string(m_queueSearchQuery))
              ->onTextEdited([this](CSharedPointer<CTextboxElement>,
                                    const std::string &text) {
                m_queueSearchQuery = text;
                m_backend->addTimer(
                    std::chrono::milliseconds(1),
                    [this](CAtomicSharedPointer<CTimer>, void *) {
                      runMpdCommand([this](struct mpd_connection *conn) {
                        struct mpd_status *status = mpd_run_status(conn);
                        int activeSongId = -1;
                        if (status) {
                          activeSongId = mpd_status_get_song_id(status);
                          mpd_status_free(status);
                        }
                        populateQueueSongs(conn, activeSongId);
                      });
                    },
                    nullptr);
              })
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {1.0F, 32.0F}))
              ->commence();
      searchBar->setGrow(true);
      topSearchRow->addChild(searchBar);

      auto addItemBtn =
          CButtonBuilder::begin()
              ->label("➕ Add Item")
              ->alignText(HT_FONT_ALIGN_CENTER)
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->onMainClick([this](CSharedPointer<CButtonElement>) {
                showQueueAddItemDialog();
              })
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {1.0F, 32.0F}))
              ->commence();
      addItemBtn->setGrow(false);
      topSearchRow->addChild(addItemBtn);

      topControlsCol->addChild(topSearchRow);

      tabMainLayout->addChild(topControlsCol);

      auto scrollArea =
          CScrollAreaBuilder::begin()
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.85F}))
              ->scrollY(true)
              ->commence();

      m_queueContentLayout =
          CColumnLayoutBuilder::begin()
              ->gap(10)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_queueContentLayout->setMargin(5);

      scrollArea->addChild(m_queueContentLayout);
      tabMainLayout->addChild(scrollArea);
    }

    m_queueContentLayout->clearChildren();

    auto loadingText =
        CTextBuilder::begin()
            ->text("⏳ Loading Queue...")
            ->color([palette] {
              return palette ? palette->m_colors.accent
                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
            ->align(HT_FONT_ALIGN_CENTER)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_queueContentLayout->addChild(loadingText);

    m_queueContentLayout->forceReposition();
    m_tabContentWrapper->forceReposition();

    m_backend->addTimer(
        std::chrono::milliseconds(1),
        [this, activeSongId](CAtomicSharedPointer<CTimer>, void *) {
          runMpdCommand([this, activeSongId](struct mpd_connection *conn) {
            populateQueueSongs(conn, activeSongId);
          });
        },
        nullptr);
  }

  void populateDatabaseSongs(struct mpd_connection *conn) {
    if (!m_dbContentLayout)
      return;

    m_dbContentLayout->clearChildren();

    auto palette = m_palette;
    int rounding = palette ? palette->m_vars.smallRounding : 5;
    std::string fontFamily = m_fontFamily;

    std::unordered_set<std::string> queueUris;
    if (conn && mpd_send_list_queue_meta(conn)) {
      struct mpd_song *s;
      while ((s = mpd_recv_song(conn)) != NULL) {
        const char *uri = mpd_song_get_uri(s);
        if (uri) {
          queueUris.insert(std::string(uri));
        }
        mpd_song_free(s);
      }
      mpd_response_finish(conn);
    }

    if (!conn || !mpd_send_list_all_meta(conn, NULL)) {
      std::cerr << "MPD: Failed to send list database command" << std::endl;
      return;
    }

    bool foundAny = false;
    struct mpd_entity *entity;
    int trackNum = 1;
    while ((entity = mpd_recv_entity(conn)) != NULL) {
      if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
        const struct mpd_song *s = mpd_entity_get_song(entity);
        const char *uri = mpd_song_get_uri(s);
        if (uri) {
          std::string songUri(uri);

          const char *artist = mpd_song_get_tag(s, MPD_TAG_ARTIST, 0);
          const char *title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
          std::string displayTitle;
          if (title) {
            std::string artistStr = artist ? artist : "Unknown Artist";
            displayTitle = std::string(title) + " - " + artistStr;
          } else {
            displayTitle = songUri;
          }

          if (!m_dbSearchQuery.empty()) {
            std::string query = m_dbSearchQuery;
            std::string titleLower = displayTitle;
            std::transform(titleLower.begin(), titleLower.end(),
                           titleLower.begin(), ::tolower);
            std::transform(query.begin(), query.end(), query.begin(),
                           ::tolower);
            if (titleLower.find(query) == std::string::npos) {
              mpd_entity_free(entity);
              continue;
            }
          }

          foundAny = true;
          bool inQueue = (queueUris.find(songUri) != queueUris.end());
          std::string indexStr = std::to_string(trackNum++) + ". ";

          auto songItem =
              CRectangleBuilder::begin()
                  ->color([palette] {
                    return palette ? palette->m_colors.base
                                   : CHyprColor(0.15, 0.15, 0.15, 1.0);
                  })
                  ->rounding(rounding)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_ABSOLUTE,
                                      {1.0F, 40.0F}))
                  ->commence();

          auto rowLayout = CRowLayoutBuilder::begin()
                               ->gap(10)
                               ->size(CDynamicSize(
                                   CDynamicSize::HT_SIZE_PERCENT,
                                   CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
                               ->commence();
          rowLayout->setMargin(6);

          auto songText = CTextBuilder::begin()
                              ->text(std::move(indexStr + displayTitle))
                              ->color([palette, inQueue] {
                                if (inQueue) {
                                  return palette
                                             ? palette->m_colors.accent
                                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
                                }
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

          auto textContainer =
              CRowLayoutBuilder::begin()
                  ->gap(0)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                      CDynamicSize::HT_SIZE_PERCENT,
                                      {1.0F, 1.0F}))
                  ->commence();
          textContainer->setGrow(true);
          textContainer->addChild(songText);
          rowLayout->addChild(textContainer);

          auto playTrackBtn =
              CTextBuilder::begin()
                  ->text(std::string("▶"))
                  ->color([palette, inQueue] {
                    if (inQueue) {
                      return palette ? palette->m_colors.accent
                                     : CHyprColor(0.2, 0.8, 0.4, 1.0);
                    }
                    return palette ? palette->m_colors.text
                                   : CHyprColor(1, 1, 1, 1);
                  })
                  ->fontFamily(std::string(fontFamily))
                  ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                  ->interactable(true)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                      CDynamicSize::HT_SIZE_ABSOLUTE,
                                      {1.0F, 28.0F}))
                  ->commence();
          playTrackBtn->setReceivesMouse(true);
          playTrackBtn->setMouseButton(
              [this, songUri](Input::eMouseButton button, bool down) {
                if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                  if (!songUri.empty()) {
                    playSongFromUri(songUri);
                  }
                }
              });
          playTrackBtn->setGrow(false);
          rowLayout->addChild(playTrackBtn);

          std::vector<std::string> dbSongOptions = {
              "Actions", "➕ Add to Queue", "📁 Add to Playlist"};
          auto songActionMenu =
              CComboboxBuilder::begin()
                  ->items(std::move(dbSongOptions))
                  ->currentItem(0)
                  ->onChanged(
                      [this, songUri](CSharedPointer<CComboboxElement> combo,
                                      size_t idx) {
                        if (idx == 1) { // ➕ Add to Queue
                          addSongToQueue(songUri);
                        } else if (idx == 2) { // 📁 Add to Playlist
                          if (!songUri.empty()) {
                            showPlaylistSelectionDialog(songUri);
                          }
                        }
                        if (combo && idx != 0) {
                          combo->setCurrent(0);
                        }
                      })
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                      CDynamicSize::HT_SIZE_ABSOLUTE,
                                      {95.0F, 28.0F}))
                  ->commence();
          songActionMenu->setGrow(false);
          rowLayout->addChild(songActionMenu);

          songItem->addChild(rowLayout);
          m_dbContentLayout->addChild(songItem);
        }
      }
      mpd_entity_free(entity);
    }
    mpd_response_finish(conn);

    if (!foundAny) {
      auto emptyText =
          CTextBuilder::begin()
              ->text("No songs found in Database")
              ->color([palette] {
                return palette ? palette->m_colors.text
                               : CHyprColor(0.6, 0.6, 0.6, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->align(HT_FONT_ALIGN_CENTER)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_dbContentLayout->addChild(emptyText);
    }

    m_dbContentLayout->forceReposition();
    m_tabContentWrapper->forceReposition();
  }

  void rebuildDatabaseUI(struct mpd_connection *conn) {
    (void)conn;
    if (!m_tabContentWrapper)
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;

    if (!m_dbContentLayout) {
      m_tabContentWrapper->clearChildren();

      auto tabMainLayout =
          CColumnLayoutBuilder::begin()
              ->gap(10)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->commence();
      m_tabContentWrapper->addChild(tabMainLayout);

      auto topControlsCol =
          CColumnLayoutBuilder::begin()
              ->gap(8)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      topControlsCol->setMargin(10);

      auto topSearchRow =
          CRowLayoutBuilder::begin()
              ->gap(12)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {1.0F, 35.0F}))
              ->commence();

      auto searchBar =
          CTextboxBuilder::begin()
              ->placeholder("Search...")
              ->defaultText(std::string(m_dbSearchQuery))
              ->onTextEdited([this](CSharedPointer<CTextboxElement>,
                                    const std::string &text) {
                m_dbSearchQuery = text;
                m_backend->addTimer(
                    std::chrono::milliseconds(1),
                    [this](CAtomicSharedPointer<CTimer>, void *) {
                      runMpdCommand([this](struct mpd_connection *conn) {
                        populateDatabaseSongs(conn);
                      });
                    },
                    nullptr);
              })
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {1.0F, 32.0F}))
              ->commence();
      searchBar->setGrow(true);
      topSearchRow->addChild(searchBar);

      std::vector<std::string> refreshOptions = {"Update", "Rescan"};
      auto refreshCombo =
          CComboboxBuilder::begin()
              ->items(std::move(refreshOptions))
              ->currentItem(0)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {110.0F, 32.0F}))
              ->onChanged(
                  [this](CSharedPointer<CComboboxElement> combo, size_t idx) {
                    if (idx == 1) { // Update
                      runMpdCommand([this](struct mpd_connection *conn) {
                        if (conn) {
                          mpd_run_update(conn, nullptr);
                          populateDatabaseSongs(conn);
                        }
                      });
                      showNotification("🔄 MPD Update Triggered");
                    } else if (idx == 2) { // Rescan
                      runMpdCommand([this](struct mpd_connection *conn) {
                        if (conn) {
                          mpd_run_rescan(conn, nullptr);
                          populateDatabaseSongs(conn);
                        }
                      });
                      showNotification("🔍 Full Database Rescan Triggered");
                    }
                    if (combo && idx != 0) {
                      combo->setCurrent(0);
                    }
                  })
              ->commence();
      refreshCombo->setGrow(false);
      topSearchRow->addChild(refreshCombo);

      topControlsCol->addChild(topSearchRow);

      tabMainLayout->addChild(topControlsCol);

      auto scrollArea =
          CScrollAreaBuilder::begin()
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.90F}))
              ->scrollY(true)
              ->commence();

      m_dbContentLayout =
          CColumnLayoutBuilder::begin()
              ->gap(10)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_dbContentLayout->setMargin(5);

      scrollArea->addChild(m_dbContentLayout);
      tabMainLayout->addChild(scrollArea);
    }

    m_dbContentLayout->clearChildren();

    auto loadingText =
        CTextBuilder::begin()
            ->text("⏳ Loading Database...")
            ->color([palette] {
              return palette ? palette->m_colors.accent
                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
            ->align(HT_FONT_ALIGN_CENTER)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_dbContentLayout->addChild(loadingText);

    m_dbContentLayout->forceReposition();
    m_tabContentWrapper->forceReposition();

    m_backend->addTimer(
        std::chrono::milliseconds(1),
        [this](CAtomicSharedPointer<CTimer>, void *) {
          runMpdCommand([this](struct mpd_connection *conn) {
            populateDatabaseSongs(conn);
          });
        },
        nullptr);
  }

  void addSettingRow(const std::string &label, const std::string &configKey,
                     bool isDirectory, bool isPathKey = true) {
    (void)isPathKey;
    if (!m_settingsContentLayout)
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;
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

    auto rowLayout =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    rowLayout->setMargin(10);
    rowItem->addChild(rowLayout);

    // Cell 0: Parameter Column (50% equal width)
    auto cell0 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.50F, 1.0F}))
            ->commence();
    auto labelText =
        CTextBuilder::begin()
            ->text(std::string(label))
            ->color([palette] {
              return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->align(HT_FONT_ALIGN_LEFT)
            ->noEllipsize(false)
            ->commence();
    cell0->addChild(labelText);
    rowLayout->addChild(cell0);

    std::string valueStr = m_pendingSettings.count(configKey)
                               ? m_pendingSettings[configKey]
                               : m_mpdSettings[configKey];

    // Cell 1: Value Column (50% equal width)
    auto cell1 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.50F, 1.0F}))
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

  void addSectionHeader(const std::string &title) {
    if (!m_settingsContentLayout)
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;

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

  void addSwitchSettingRow(const std::string &label,
                           const std::string &configKey,
                           const std::string &description = "") {
    (void)description;
    if (!m_settingsContentLayout)
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;
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

    auto rowLayout =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    rowLayout->setMargin(10);
    rowItem->addChild(rowLayout);

    // Cell 0: Parameter Column (50% equal width)
    auto cell0 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.50F, 1.0F}))
            ->commence();
    auto labelText =
        CTextBuilder::begin()
            ->text(std::string(label))
            ->color([palette] {
              return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->align(HT_FONT_ALIGN_LEFT)
            ->noEllipsize(false)
            ->commence();
    cell0->addChild(labelText);
    rowLayout->addChild(cell0);

    std::string curValStr = m_pendingSettings.count(configKey)
                                ? m_pendingSettings[configKey]
                                : m_mpdSettings[configKey];
    bool currentVal = (curValStr != "no");
    size_t currentIdx = currentVal ? 0 : 1;

    std::vector<std::string> comboItems = {"ON", "OFF"};

    // Cell 1: Value Column (50% equal width)
    auto cell1 =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.50F, 1.0F}))
            ->commence();
    auto combobox =
        CComboboxBuilder::begin()
            ->items(std::move(comboItems))
            ->currentItem(currentIdx)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {0.98F, 30.0F}))
            ->onChanged([this, configKey](
                            Hyprutils::Memory::CSharedPointer<CComboboxElement>,
                            size_t idx) {
              m_pendingSettings[configKey] = (idx == 0 ? "yes" : "no");
            })
            ->commence();
    cell1->addChild(combobox);
    rowLayout->addChild(cell1);

    m_settingsContentLayout->addChild(rowItem);
  }

  void rebuildSettingsUI() {
    if (!m_tabContentWrapper)
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;

    m_tabContentWrapper->clearChildren();

    auto settingsMainLayout =
        CColumnLayoutBuilder::begin()
            ->gap(15)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    settingsMainLayout->setMargin(20);
    m_tabContentWrapper->addChild(settingsMainLayout);

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

    m_mpdSettings = parseMpdConfig(getMpdConfPath());
    m_pendingSettings = m_mpdSettings;

    addSettingRow("Music Path", "music_directory", true);
    addSettingRow("Database Path", "db_file", false);
    addSettingRow("Playlist Path", "playlist_directory", true);
    addSettingRow("Log Path", "log_file", false);
    addSettingRow("State Path", "state_file", false);
    addSettingRow("Sticker Path", "sticker_file", false);
    addSettingRow("PID Path", "pid_file", false);

    addSettingRow("Bind to Address", "bind_to_address", false, false);
    addSettingRow("Port", "port", false, false);

    addSwitchSettingRow("Auto Update", "auto_update");
    addSwitchSettingRow("Restore Paused", "restore_paused");

    addSwitchSettingRow("PipeWire / Pulse Server", "audio_output_pulse");
    addSwitchSettingRow("FIFO Visualizer", "audio_output_fifo");

    auto saveRow =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
            ->commence();

    auto leftSpacer =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.40F, 1.0F}))
            ->commence();
    saveRow->addChild(leftSpacer);

    auto saveBtn = CButtonBuilder::begin()
                       ->label("Save")
                       ->alignText(HT_FONT_ALIGN_CENTER)
                       ->fontFamily(std::string(fontFamily))
                       ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                       ->onMainClick([this](CSharedPointer<CButtonElement>) {
                         bool changed = false;
                         for (const auto &[k, v] : m_pendingSettings) {
                           if (m_mpdSettings[k] != v) {
                             m_mpdSettings[k] = v;
                             changed = true;
                           }
                         }
                         if (changed) {
                           saveMpdConfig(getMpdConfPath(), m_mpdSettings);

                           std::string confPath = getMpdConfPath();
                           std::system("killall mpd >/dev/null 2>&1");
                           std::string startCmd =
                               "mpd " + confPath + " >/dev/null 2>&1";
                           int ret = std::system(startCmd.c_str());
                           (void)ret;

                           showNotification("Settings Saved");

                           m_backend->addTimer(
                               std::chrono::milliseconds(50),
                               [this](CAtomicSharedPointer<CTimer>, void *) {
                                 runMpdCommand([](struct mpd_connection *conn) {
                                   if (conn) {
                                     mpd_run_update(conn, nullptr);
                                   }
                                 });
                                 rebuildSettingsUI();
                               },
                               nullptr);
                         } else {
                           showNotification("No changes to save");
                         }
                       })
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                           CDynamicSize::HT_SIZE_ABSOLUTE,
                                           {120.0F, 32.0F}))
                       ->commence();
    saveRow->addChild(saveBtn);

    auto rightSpacer =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.40F, 1.0F}))
            ->commence();
    saveRow->addChild(rightSpacer);

    settingsMainLayout->addChild(saveRow);

    m_settingsContentLayout->forceReposition();
    settingsMainLayout->forceReposition();
    m_tabContentWrapper->forceReposition();
  }

  void rebuildHelpUI() {
    if (!m_tabContentWrapper)
      return;

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;

    m_tabContentWrapper->clearChildren();

    auto helpMainLayout =
        CColumnLayoutBuilder::begin()
            ->gap(15)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    helpMainLayout->setMargin(20);
    m_tabContentWrapper->addChild(helpMainLayout);

    auto scrollArea =
        CScrollAreaBuilder::begin()
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->scrollY(true)
            ->commence();
    helpMainLayout->addChild(scrollArea);

    m_helpContentLayout =
        CColumnLayoutBuilder::begin()
            ->gap(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_helpContentLayout->setMargin(5);
    scrollArea->addChild(m_helpContentLayout);

    std::string helpFilePath = "HELP.md";
    std::ifstream inFile(helpFilePath);
    std::vector<std::string> lines;
    if (inFile.is_open()) {
      std::string line;
      while (std::getline(inFile, line)) {
        lines.push_back(line);
      }
      inFile.close();
    }

    if (lines.empty()) {
      lines = {
          "# HyprMusic - User Guide & Overview",
          "HyprMusic is a native Linux music player designed for Hyprland using Hyprtoolkit.",
          "---",
          "## Key Features & Tabs",
          "### 1. Queue Tab",
          "- View active playback queue, search tracks, shuffle, and clear queue.",
          "### 2. Database Tab",
          "- Browse local music folder structure and search music database.",
          "### 3. Playlists Tab",
          "- Manage saved MPD playlists and view playlist tracks.",
          "### 4. YT-DLP Online Search Tab",
          "- Search YouTube music audio and stream directly into MPD.",
          "### 5. Settings Tab",
          "- Configure MPD paths, audio outputs, and server options.",
          "---",
          "## Playback & Control Bar",
          "- Interactive bottom bar with track details, seek slider, play/pause, and volume."};
    }

    CSharedPointer<CColumnLayoutElement> currentCardLayout = nullptr;

    auto finalizeCard = [this, palette](CSharedPointer<CColumnLayoutElement> cardLayout) {
      if (!cardLayout)
        return;
      auto cardBg =
          CRectangleBuilder::begin()
              ->color([palette] {
                return palette ? palette->m_colors.base
                               : CHyprColor(0.15, 0.15, 0.15, 1.0);
              })
              ->rounding(palette ? palette->m_vars.smallRounding : 5)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      cardBg->addChild(cardLayout);
      m_helpContentLayout->addChild(cardBg);
    };

    for (const auto &rawLine : lines) {
      std::string line = rawLine;
      if (!line.empty() && line.back() == '\r')
        line.pop_back();

      if (line.rfind("# ", 0) == 0) {
        if (currentCardLayout) {
          finalizeCard(currentCardLayout);
          currentCardLayout = nullptr;
        }

        auto titleTxt =
            CTextBuilder::begin()
                ->text(line.substr(2))
                ->color([palette] {
                  return palette ? palette->m_colors.accent
                                 : CHyprColor(0.2, 0.8, 0.4, 1.0);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_H1))
                ->align(HT_FONT_ALIGN_LEFT)
                ->commence();
        m_helpContentLayout->addChild(titleTxt);
      } else if (line.rfind("## ", 0) == 0) {
        if (currentCardLayout) {
          finalizeCard(currentCardLayout);
          currentCardLayout = nullptr;
        }

        auto secTxt =
            CTextBuilder::begin()
                ->text(line.substr(3))
                ->color([palette] {
                  return palette ? palette->m_colors.text
                                 : CHyprColor(1, 1, 1, 1);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_H2))
                ->align(HT_FONT_ALIGN_LEFT)
                ->commence();
        m_helpContentLayout->addChild(secTxt);
      } else if (line.rfind("---", 0) == 0) {
        if (currentCardLayout) {
          finalizeCard(currentCardLayout);
          currentCardLayout = nullptr;
        }

        auto divider =
            CRectangleBuilder::begin()
                ->color([palette] {
                  return palette ? palette->m_colors.alternateBase
                                 : CHyprColor(0.25, 0.25, 0.25, 1.0);
                })
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
                ->commence();
        m_helpContentLayout->addChild(divider);
      } else if (line.rfind("### ", 0) == 0) {
        if (!currentCardLayout) {
          currentCardLayout =
              CColumnLayoutBuilder::begin()
                  ->gap(6)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->commence();
          currentCardLayout->setMargin(12);
        }

        auto h3Txt =
            CTextBuilder::begin()
                ->text(line.substr(4))
                ->color([palette] {
                  return palette ? palette->m_colors.accent
                                 : CHyprColor(0.2, 0.8, 0.4, 1.0);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
                ->align(HT_FONT_ALIGN_LEFT)
                ->commence();
        currentCardLayout->addChild(h3Txt);
      } else if (!line.empty()) {
        if (!currentCardLayout) {
          currentCardLayout =
              CColumnLayoutBuilder::begin()
                  ->gap(6)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->commence();
          currentCardLayout->setMargin(12);
        }

        auto textElem =
            CTextBuilder::begin()
                ->text(std::string(line))
                ->color([palette] {
                  return palette ? palette->m_colors.text
                                 : CHyprColor(0.85, 0.85, 0.85, 1.0);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                ->align(HT_FONT_ALIGN_LEFT)
                ->commence();
        currentCardLayout->addChild(textElem);
      }
    }

    if (currentCardLayout) {
      finalizeCard(currentCardLayout);
      currentCardLayout = nullptr;
    }

    m_helpContentLayout->forceReposition();
    helpMainLayout->forceReposition();
    m_tabContentWrapper->forceReposition();
  }

  void layoutPlaylists() {
    if (!m_playlistsLeftItemsLayout)
      return;

    m_playlistsLeftItemsLayout->clearChildren();

    auto palette = m_palette;
    int rounding = palette ? palette->m_vars.smallRounding : 5;
    std::string fontFamily = m_fontFamily;

    if (m_currentPlaylists.empty()) {
      auto emptyText =
          CTextBuilder::begin()
              ->text("No playlists found")
              ->color([palette] {
                return palette ? palette->m_colors.text
                               : CHyprColor(0.6, 0.6, 0.6, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->align(HT_FONT_ALIGN_CENTER)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_playlistsLeftItemsLayout->addChild(emptyText);
    } else {
      double parentWidth = m_playlistsLeftItemsLayout->size().x;
      if (parentWidth <= 0) {
        parentWidth = 800.0;
      }

      double minPercent = 0.10;
      double gap = 15.0;
      double minSize = std::max(parentWidth * minPercent, 130.0);

      size_t columns = std::max(
          (size_t)1, (size_t)std::floor((parentWidth + gap) / (minSize + gap)));
      double actualSize = (parentWidth - (columns - 1) * gap) / columns;
      if (actualSize < 10)
        actualSize = 130.0;

      CSharedPointer<CRowLayoutElement> currentRow = nullptr;

      for (size_t i = 0; i < m_currentPlaylists.size(); ++i) {
        if (i % columns == 0) {
          currentRow =
              CRowLayoutBuilder::begin()
                  ->gap((size_t)gap)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->commence();
          m_playlistsLeftItemsLayout->addChild(currentRow);
        }

        auto plName = m_currentPlaylists[i];
        bool isSelected = (plName == m_selectedPlaylist);

        auto card =
            CRectangleBuilder::begin()
                ->color([palette, isSelected] {
                  if (isSelected) {
                    return palette ? palette->m_colors.accent
                                   : CHyprColor(0.2, 0.8, 0.4, 1.0);
                  }
                  return palette ? palette->m_colors.alternateBase
                                 : CHyprColor(0.18, 0.18, 0.18, 1.0);
                })
                ->rounding(rounding)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {(float)actualSize, (float)actualSize}))
                ->commence();

        auto textCol =
            CColumnLayoutBuilder::begin()
                ->gap(4)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_AUTO, {0.85F, 1.0F}))
                ->commence();

        auto titleText =
            CTextBuilder::begin()
                ->text(std::string(plName))
                ->color([palette, isSelected] {
                  if (isSelected) {
                    return palette ? palette->m_colors.alternateBase
                                   : CHyprColor(0.18, 0.18, 0.18, 1.0);
                  }
                  return palette ? palette->m_colors.text
                                 : CHyprColor(1.0, 1.0, 1.0, 1.0);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                ->align(HT_FONT_ALIGN_CENTER)
                ->noEllipsize(false)
                ->commence();

        int trackCount = 0;
        if (m_playlistTrackCounts.find(plName) != m_playlistTrackCounts.end()) {
          trackCount = m_playlistTrackCounts[plName];
        }
        std::string subStr = "(" + std::to_string(trackCount) +
                             (trackCount == 1 ? " track)" : " tracks)");

        auto trackCountText =
            CTextBuilder::begin()
                ->text(std::move(subStr))
                ->color([palette, isSelected] {
                  if (isSelected) {
                    auto altBase = palette ? palette->m_colors.alternateBase
                                           : CHyprColor(0.18, 0.18, 0.18, 1.0);
                    auto acc = palette ? palette->m_colors.accent
                                       : CHyprColor(0.2, 0.8, 0.4, 1.0);
                    return altBase.mix(acc, 0.25);
                  }
                  auto txt = palette ? palette->m_colors.text
                                     : CHyprColor(1.0, 1.0, 1.0, 1.0);
                  auto altBase = palette ? palette->m_colors.alternateBase
                                         : CHyprColor(0.18, 0.18, 0.18, 1.0);
                  return txt.mix(altBase, 0.35);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                ->align(HT_FONT_ALIGN_CENTER)
                ->noEllipsize(true)
                ->commence();

        textCol->addChild(titleText);
        textCol->addChild(trackCountText);

        textCol->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
        textCol->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
        textCol->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
        card->addChild(textCol);

        card->setReceivesMouse(true);
        card->setMouseButton(
            [this, plName](Input::eMouseButton button, bool down) {
              if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                m_selectedPlaylist = plName;
                m_playlistsDetailedView = true;
                m_playlistLoaded = false;
                updateStatus();
              }
            });

        std::vector<std::string> options = {"", "▶ Play", "➕ Add to Queue",
                                            "✏️ Rename", "🗑️ Delete"};
        auto actionMenu =
            CComboboxBuilder::begin()
                ->items(std::move(options))
                ->currentItem(0)
                ->onChanged([this,
                             plName](CSharedPointer<CComboboxElement> combo,
                                     size_t idx) {
                  if (idx == 1) {
                    runMpdCommand([plName](struct mpd_connection *conn) {
                      mpd_run_clear(conn);
                      mpd_run_load(conn, plName.c_str());
                      mpd_run_play(conn);
                    });
                    m_backend->addTimer(
                        std::chrono::milliseconds(100),
                        [this](CAtomicSharedPointer<CTimer>, void *) {
                          updateStatus();
                        },
                        nullptr);
                  } else if (idx == 2) {
                    runMpdCommand([this, plName](struct mpd_connection *conn) {
                      std::unordered_set<std::string> queueUris;
                      if (conn && mpd_send_list_queue_meta(conn)) {
                        struct mpd_song *s;
                        while ((s = mpd_recv_song(conn)) != NULL) {
                          const char *qUri = mpd_song_get_uri(s);
                          if (qUri) {
                            queueUris.insert(std::string(qUri));
                          }
                          mpd_song_free(s);
                        }
                        mpd_response_finish(conn);
                      }

                      std::vector<std::string> playlistUris;
                      if (conn &&
                          mpd_send_list_playlist_meta(conn, plName.c_str())) {
                        struct mpd_song *s;
                        while ((s = mpd_recv_song(conn)) != NULL) {
                          const char *pUri = mpd_song_get_uri(s);
                          if (pUri) {
                            playlistUris.push_back(std::string(pUri));
                          }
                          mpd_song_free(s);
                        }
                        mpd_response_finish(conn);
                      }

                      int addedCount = 0;
                      int skippedCount = 0;
                      for (const auto &songUri : playlistUris) {
                        if (queueUris.find(songUri) != queueUris.end()) {
                          skippedCount++;
                        } else {
                          if (mpd_run_add(conn, songUri.c_str())) {
                            addedCount++;
                          }
                        }
                      }

                      m_backend->addTimer(
                          std::chrono::milliseconds(1),
                          [this, addedCount,
                           skippedCount](CAtomicSharedPointer<CTimer>, void *) {
                            if (addedCount > 0 && skippedCount > 0) {
                              showNotification("Some items already in queue, "
                                               "skipped. Rest added.");
                            } else if (addedCount > 0 && skippedCount == 0) {
                              showNotification("Playlist added to queue");
                            } else if (addedCount == 0 && skippedCount > 0) {
                              showNotification(
                                  "All playlist tracks already in queue");
                            } else {
                              showNotification("Playlist is empty");
                            }
                          },
                          nullptr);
                    });
                    m_backend->addTimer(
                        std::chrono::milliseconds(100),
                        [this](CAtomicSharedPointer<CTimer>, void *) {
                          updateStatus();
                        },
                        nullptr);
                  } else if (idx == 3) {
                    showRenameDialog(plName);
                  } else if (idx == 4) {
                    runMpdCommand([plName](struct mpd_connection *conn) {
                      mpd_run_rm(conn, plName.c_str());
                    });
                    if (m_selectedPlaylist == plName) {
                      m_selectedPlaylist = "";
                      m_playlistsDetailedView = false;
                    }
                    m_backend->addTimer(
                        std::chrono::milliseconds(100),
                        [this](CAtomicSharedPointer<CTimer>, void *) {
                          runMpdCommand([this](struct mpd_connection *conn) {
                            rebuildPlaylistsLeftItems(conn);
                          });
                        },
                        nullptr);
                  }
                  if (combo && idx != 0) {
                    combo->setCurrent(0);
                  }
                })
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {40.0F, 40.0F}))
                ->commence();
        actionMenu->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
        actionMenu->setPositionFlag(IElement::HT_POSITION_FLAG_RIGHT, true);
        actionMenu->setPositionFlag(IElement::HT_POSITION_FLAG_TOP, true);
        actionMenu->setMargin(5.0F);
        card->addChild(actionMenu);

        currentRow->addChild(card);
      }

      if (m_currentPlaylists.size() % columns != 0) {
        size_t missing = columns - (m_currentPlaylists.size() % columns);
        for (size_t m = 0; m < missing; ++m) {
          auto spacer =
              CRectangleBuilder::begin()
                  ->color([] { return CHyprColor(0, 0, 0, 0); })
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                      CDynamicSize::HT_SIZE_ABSOLUTE,
                                      {(float)actualSize, (float)actualSize}))
                  ->commence();
          currentRow->addChild(spacer);
        }
      }
    }
    m_playlistsLeftItemsLayout->forceReposition();
  }

  void rebuildPlaylistsLeftItems(struct mpd_connection *conn) {
    if (!m_playlistsLeftItemsLayout)
      return;

    std::vector<std::string> playlists;
    if (conn && mpd_send_list_playlists(conn)) {
      struct mpd_playlist *pl;
      while ((pl = mpd_recv_playlist(conn)) != NULL) {
        const char *name = mpd_playlist_get_path(pl);
        if (name) {
          std::string sName(name);
          if (m_playlistsSearchQuery.empty() ||
              sName.find(m_playlistsSearchQuery) != std::string::npos) {
            playlists.push_back(sName);
          }
        }
        mpd_playlist_free(pl);
      }
      mpd_response_finish(conn);
    }

    if (!m_selectedPlaylist.empty() &&
        std::find(playlists.begin(), playlists.end(), m_selectedPlaylist) ==
            playlists.end()) {
      m_selectedPlaylist = "";
    }

    m_playlistTrackCounts.clear();
    for (const auto &plName : playlists) {
      int count = 0;
      if (conn && mpd_send_list_playlist_meta(conn, plName.c_str())) {
        struct mpd_song *s;
        while ((s = mpd_recv_song(conn)) != NULL) {
          count++;
          mpd_song_free(s);
        }
        mpd_response_finish(conn);
      }
      m_playlistTrackCounts[plName] = count;
    }

    m_currentPlaylists = playlists;
    layoutPlaylists();
  }

  void rebuildPlaylistsRightItems(struct mpd_connection *conn) {
    if (!m_playlistsRightItemsLayout)
      return;

    m_playlistsRightItemsLayout->clearChildren();

    auto palette = m_palette;
    int rounding = palette ? palette->m_vars.smallRounding : 5;
    std::string fontFamily = m_fontFamily;

    std::unordered_set<std::string> queueUris;
    if (conn && mpd_send_list_queue_meta(conn)) {
      struct mpd_song *s;
      while ((s = mpd_recv_song(conn)) != NULL) {
        const char *qUri = mpd_song_get_uri(s);
        if (qUri) {
          queueUris.insert(std::string(qUri));
        }
        mpd_song_free(s);
      }
      mpd_response_finish(conn);
    }

    auto addSongItem = [&](const std::string &displayTitle,
                           const std::string &songUri, unsigned songPos) {
      bool inQueue =
          (!songUri.empty() && queueUris.find(songUri) != queueUris.end());

      auto songItem = CRectangleBuilder::begin()
                          ->color([palette] {
                            return palette ? palette->m_colors.alternateBase
                                           : CHyprColor(0.18, 0.18, 0.18, 1.0);
                          })
                          ->rounding(rounding)
                          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                              CDynamicSize::HT_SIZE_ABSOLUTE,
                                              {1.0F, 40.0F}))
                          ->commence();

      auto songRow =
          CRowLayoutBuilder::begin()
              ->gap(10)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->commence();
      songRow->setMargin(6);

      auto sText =
          CTextBuilder::begin()
              ->text(std::string(displayTitle))
              ->color([palette, inQueue] {
                if (inQueue) {
                  return palette ? palette->m_colors.accent
                                 : CHyprColor(0.2, 0.8, 0.4, 1.0);
                }
                return palette ? palette->m_colors.text
                               : CHyprColor(1, 1, 1, 1);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->align(HT_FONT_ALIGN_LEFT)
              ->noEllipsize(false)
              ->commence();

      auto textContainer =
          CRowLayoutBuilder::begin()
              ->gap(0)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->commence();
      textContainer->setGrow(true);
      textContainer->addChild(sText);
      songRow->addChild(textContainer);

      auto playTrackBtn =
          CTextBuilder::begin()
              ->text(std::string("▶"))
              ->color([palette, inQueue] {
                if (inQueue) {
                  return palette ? palette->m_colors.accent
                                 : CHyprColor(0.2, 0.8, 0.4, 1.0);
                }
                return palette ? palette->m_colors.text
                               : CHyprColor(1, 1, 1, 1);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->interactable(true)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {1.0F, 28.0F}))
              ->commence();
      playTrackBtn->setReceivesMouse(true);
      playTrackBtn->setMouseButton(
          [this, songUri](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && !down) {
              if (!songUri.empty()) {
                playSongFromUri(songUri);
              }
            }
          });
      playTrackBtn->setGrow(false);
      songRow->addChild(playTrackBtn);

      std::vector<std::string> songOptions = {
          "Actions", "➕ Add to Queue", "🗑️ Remove", "📁 Move to Playlist"};
      auto songActionMenu =
          CComboboxBuilder::begin()
              ->items(std::move(songOptions))
              ->currentItem(0)
              ->onChanged([this, songPos,
                           songUri](CSharedPointer<CComboboxElement> combo,
                                    size_t idx) {
                if (idx == 1) { // ➕ Add to Queue
                  if (!songUri.empty()) {
                    addSongToQueue(songUri);
                    m_backend->addTimer(
                        std::chrono::milliseconds(100),
                        [this](CAtomicSharedPointer<CTimer>, void *) {
                          runMpdCommand([this](struct mpd_connection *conn) {
                            rebuildPlaylistsRightItems(conn);
                          });
                        },
                        nullptr);
                  }
                } else if (idx == 2) { // 🗑️ Remove
                  if (!m_selectedPlaylist.empty()) {
                    std::string targetPl = m_selectedPlaylist;
                    runMpdCommand([targetPl,
                                   songPos](struct mpd_connection *conn) {
                      mpd_run_playlist_delete(conn, targetPl.c_str(), songPos);
                    });
                    m_backend->addTimer(
                        std::chrono::milliseconds(100),
                        [this](CAtomicSharedPointer<CTimer>, void *) {
                          runMpdCommand([this](struct mpd_connection *conn) {
                            rebuildPlaylistsRightItems(conn);
                          });
                        },
                        nullptr);
                  }
                } else if (idx == 3) { // 📁 Move to Playlist
                  if (!songUri.empty()) {
                    showPlaylistSelectionDialog(songUri, songPos);
                  }
                }
                if (combo && idx != 0) {
                  combo->setCurrent(0);
                }
              })
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {95.0F, 28.0F}))
              ->commence();
      songActionMenu->setGrow(false);
      songRow->addChild(songActionMenu);

      songItem->addChild(songRow);
      m_playlistsRightItemsLayout->addChild(songItem);
    };

    if (!m_selectedPlaylist.empty()) {
      auto headerRow =
          CRowLayoutBuilder::begin()
              ->gap(10)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      headerRow->setMargin(4);

      auto backCircleBg =
          CRectangleBuilder::begin()
              ->color([palette] {
                return palette ? palette->m_colors.alternateBase
                               : CHyprColor(0.18, 0.18, 0.18, 1.0);
              })
              ->rounding(16)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {32.0F, 32.0F}))
              ->commence();
      backCircleBg->setReceivesMouse(true);
      backCircleBg->setMouseButton(
          [this](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && !down) {
              m_selectedPlaylist = "";
              m_playlistsDetailedView = false;
              m_playlistLoaded = false;
              updateStatus();
            }
          });

      auto backBtnText = CTextBuilder::begin()
                             ->text("◀")
                             ->color([palette] {
                               return palette ? palette->m_colors.accent
                                              : CHyprColor(0.2, 0.8, 0.4, 1.0);
                             })
                             ->fontFamily(std::string(fontFamily))
                             ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                             ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                                 CDynamicSize::HT_SIZE_ABSOLUTE,
                                                 {14.0F, 20.0F}))
                             ->commence();
      backBtnText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
      backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
      backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
      backBtnText->setAbsolutePosition({-1.0F, 0.0F});
      backCircleBg->addChild(backBtnText);
      headerRow->addChild(backCircleBg);

      auto headerText =
          CTextBuilder::begin()
              ->text(std::string(m_selectedPlaylist))
              ->color([palette] {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      headerText->setGrow(true);
      headerRow->addChild(headerText);

      auto addItemBtn =
          CTextBuilder::begin()
              ->text("➕ Add Item")
              ->color([palette] {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->interactable(true)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      addItemBtn->setReceivesMouse(true);
      addItemBtn->setMouseButton([this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          showPlaylistAddItemDialog(m_selectedPlaylist);
        }
      });
      headerRow->addChild(addItemBtn);

      m_playlistsRightItemsLayout->addChild(headerRow);

      bool foundSongs = false;
      if (conn &&
          mpd_send_list_playlist_meta(conn, m_selectedPlaylist.c_str())) {
        struct mpd_entity *entity;
        unsigned songPos = 0;
        int trackNum = 1;
        while ((entity = mpd_recv_entity(conn)) != NULL) {
          if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            foundSongs = true;
            const struct mpd_song *song = mpd_entity_get_song(entity);
            const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
            const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
            const char *uri = mpd_song_get_uri(song);

            std::string displayTitle = std::to_string(trackNum++) + ". ";
            std::string storedTitle, storedUploader;
            if (title && strlen(title) > 0) {
              displayTitle += title;
              if (artist)
                displayTitle += " - " + std::string(artist);
            } else if (uri && getUrlTitle(uri, storedTitle, storedUploader)) {
              displayTitle += storedTitle;
              if (!storedUploader.empty())
                displayTitle += " - " + storedUploader;
            } else if (uri) {
              std::string uriStr(uri);
              if (uriStr.find("googlevideo.com") != std::string::npos ||
                  uriStr.find("http://") == 0 || uriStr.find("https://") == 0) {
                if (uriStr.length() > 50) {
                  displayTitle += "🌐 Stream (" + uriStr.substr(0, 35) + "...)";
                } else {
                  displayTitle += uriStr;
                }
              } else {
                displayTitle += uriStr;
              }
            } else {
              displayTitle += "Unknown Track";
            }

            std::string songUri = uri ? uri : "";
            addSongItem(displayTitle, songUri, songPos++);
          }
          mpd_entity_free(entity);
        }
        mpd_response_finish(conn);
      }

      if (!foundSongs && conn &&
          mpd_send_list_playlist(conn, m_selectedPlaylist.c_str())) {
        struct mpd_pair *pair;
        unsigned songPos = 0;
        int trackNum = 1;
        while ((pair = mpd_recv_pair_named(conn, "file")) != NULL) {
          std::string songUri = pair->value;
          std::string displayTitle =
              std::to_string(trackNum++) + ". " + songUri;
          addSongItem(displayTitle, songUri, songPos++);
          mpd_return_pair(conn, pair);
        }
        mpd_response_finish(conn);
      }
    } else {
      auto hintText =
          CTextBuilder::begin()
              ->text("👈 Select a playlist to view its items")
              ->color([palette] {
                return palette ? palette->m_colors.text
                               : CHyprColor(0.6, 0.6, 0.6, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->align(HT_FONT_ALIGN_CENTER)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_playlistsRightItemsLayout->addChild(hintText);
    }
    m_playlistsRightItemsLayout->forceReposition();
  }

  void rebuildPlaylistsUI(struct mpd_connection *conn) {
    if (!m_tabContentWrapper)
      return;

    m_tabContentWrapper->clearChildren();

    auto palette = m_palette;
    std::string fontFamily = m_fontFamily;

    if (!m_playlistsDetailedView) {
      m_playlistsRightItemsLayout = nullptr;

      auto mainColumn =
          CColumnLayoutBuilder::begin()
              ->gap(10)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->commence();
      mainColumn->setMargin(15);
      m_tabContentWrapper->addChild(mainColumn);

      auto topRow = CRowLayoutBuilder::begin()
                        ->gap(12)
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {1.0F, 35.0F}))
                        ->commence();

      auto searchBar =
          CTextboxBuilder::begin()
              ->placeholder("Search...")
              ->defaultText(std::string(m_playlistsSearchQuery))
              ->onTextEdited([this](CSharedPointer<CTextboxElement>,
                                    const std::string &text) {
                m_playlistsSearchQuery = text;
                runMpdCommand([this](struct mpd_connection *conn) {
                  rebuildPlaylistsLeftItems(conn);
                });
              })
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {1.0F, 32.0F}))
              ->commence();
      searchBar->setGrow(true);
      topRow->addChild(searchBar);

      auto createPlBtn =
          CButtonBuilder::begin()
              ->label("➕ Create New")
              ->alignText(HT_FONT_ALIGN_CENTER)
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->onMainClick([this](CSharedPointer<CButtonElement>) {
                showCreatePlaylistDialog();
              })
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {1.0F, 32.0F}))
              ->commence();
      createPlBtn->setGrow(false);
      topRow->addChild(createPlBtn);

      mainColumn->addChild(topRow);

      auto scrollArea =
          CScrollAreaBuilder::begin()
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->scrollY(true)
              ->commence();

      m_playlistsLeftItemsLayout =
          CColumnLayoutBuilder::begin()
              ->gap(15)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_lastPlaylistsWidth = 0.0;
      m_playlistsLeftItemsLayout->setRepositioned([this]() {
        double currentWidth = m_playlistsLeftItemsLayout->size().x;
        if (currentWidth > 0 &&
            std::abs(currentWidth - m_lastPlaylistsWidth) > 1.0) {
          m_lastPlaylistsWidth = currentWidth;
          layoutPlaylists();
        }
      });
      scrollArea->addChild(m_playlistsLeftItemsLayout);
      mainColumn->addChild(scrollArea);

      m_tabContentWrapper->forceReposition();

      rebuildPlaylistsLeftItems(conn);
    } else {
      m_playlistsLeftItemsLayout = nullptr;

      auto mainColumn =
          CColumnLayoutBuilder::begin()
              ->gap(10)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->commence();
      mainColumn->setMargin(15);
      m_tabContentWrapper->addChild(mainColumn);

      auto scrollArea =
          CScrollAreaBuilder::begin()
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
              ->scrollY(true)
              ->commence();

      m_playlistsRightItemsLayout =
          CColumnLayoutBuilder::begin()
              ->gap(8)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_playlistsRightItemsLayout->setMargin(10);
      scrollArea->addChild(m_playlistsRightItemsLayout);
      mainColumn->addChild(scrollArea);

      m_tabContentWrapper->forceReposition();

      rebuildPlaylistsRightItems(conn);
    }
  }

  void updateStatus() {
    struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);
    if (!conn)
      return;
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
      mpd_connection_free(conn);
      return;
    }

    std::string trackText = "No currently playing songs";
    std::string stateText = "▶";
    int activeSongId = -1;
    unsigned currentQueueVersion = 0;
    int currentVolume = -1;
    unsigned elapsed = 0;
    unsigned total = 100;
    bool hasActiveTrack = false;
    m_isPlaying = false;

    struct mpd_status *status = mpd_run_status(conn);
    if (status) {
      enum mpd_state state = mpd_status_get_state(status);
      activeSongId = mpd_status_get_song_id(status);
      currentQueueVersion = mpd_status_get_queue_version(status);
      currentVolume = mpd_status_get_volume(status);
      m_isPlaying = (state == MPD_STATE_PLAY);

      if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
        stateText = (state == MPD_STATE_PLAY) ? "⏸" : "▶";
        elapsed = mpd_status_get_elapsed_time(status);
        total = mpd_status_get_total_time(status);
        hasActiveTrack = true;

        struct mpd_song *song = mpd_run_current_song(conn);
        if (song) {
          const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
          const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
          const char *uri = mpd_song_get_uri(song);

          std::string storedTitle, storedUploader;
          if (title && strlen(title) > 0) {
            std::string artistStr = artist ? artist : "Unknown Artist";
            trackText = std::string(title) + " - " + artistStr;
          } else if (uri && getUrlTitle(uri, storedTitle, storedUploader)) {
            trackText = storedTitle;
            if (!storedUploader.empty()) {
              trackText += " - " + storedUploader;
            }
          } else if (uri) {
            std::string uriStr(uri);
            if (uriStr.find("googlevideo.com") != std::string::npos ||
                uriStr.find("http://") == 0 || uriStr.find("https://") == 0) {
              if (uriStr.length() > 50) {
                trackText = "🌐 Stream (" + uriStr.substr(0, 35) + "...)";
              } else {
                trackText = uriStr;
              }
            } else {
              trackText = uriStr;
            }
          } else {
            trackText = "Unknown track";
          }
          mpd_song_free(song);
        } else {
          trackText = "Unknown track";
        }

        if (state == MPD_STATE_PAUSE) {
          trackText = "⏸  " + trackText;
        }
      } else {
        trackText = "No currently playing songs";
        stateText = "▶";
      }

      if (m_viewMode == VIEW_QUEUE) {
        if (!m_playlistLoaded || m_lastQueueVersion != currentQueueVersion) {
          m_lastQueueVersion = currentQueueVersion;
          m_lastActiveSongId = activeSongId;
          m_playlistLoaded = true;
          rebuildQueueUI(conn, activeSongId);
        } else if (m_lastActiveSongId != activeSongId) {
          int oldSongId = m_lastActiveSongId;
          m_lastActiveSongId = activeSongId;
          if (m_queueSongTexts.find(oldSongId) != m_queueSongTexts.end() &&
              m_queueSongTexts[oldSongId]) {
            m_queueSongTexts[oldSongId]->recheckColor();
          }
          if (m_queueSongTexts.find(activeSongId) != m_queueSongTexts.end() &&
              m_queueSongTexts[activeSongId]) {
            m_queueSongTexts[activeSongId]->recheckColor();
          }
          if (m_queueContentLayout) {
            m_queueContentLayout->forceReposition();
          }
        }
      } else if (m_viewMode == VIEW_DATABASE) {
        if (!m_playlistLoaded) {
          m_playlistLoaded = true;
          rebuildDatabaseUI(conn);
        }
      } else if (m_viewMode == VIEW_PLAYLISTS) {
        if (!m_playlistLoaded) {
          m_playlistLoaded = true;
          rebuildPlaylistsUI(conn);
        }
      } else if (m_viewMode == VIEW_YTDLP) {
        bool needRebuild = false;
        {
          std::lock_guard<std::mutex> lock(m_ytdlpMutex);
          if (m_ytdlpNeedRebuild) {
            needRebuild = true;
            m_ytdlpNeedRebuild = false;
          }
        }
        if (!m_playlistLoaded || needRebuild) {
          m_playlistLoaded = true;
          rebuildYtDlpUI(conn);
        }
      } else if (m_viewMode == VIEW_SETTINGS) {
        if (!m_playlistLoaded) {
          m_playlistLoaded = true;
          rebuildSettingsUI();
        }
      } else if (m_viewMode == VIEW_HELP) {
        if (!m_playlistLoaded) {
          m_playlistLoaded = true;
          rebuildHelpUI();
        }
      }

      mpd_status_free(status);
    }

    mpd_connection_free(conn);

    if (m_nowPlayingText) {
      m_nowPlayingText->rebuild()
          ->text(std::move(trackText))
          ->align(HT_FONT_ALIGN_LEFT)
          ->noEllipsize(false)
          ->commence();
    }

    auto formatTime = [](unsigned int seconds) -> std::string {
      unsigned int mins = seconds / 60;
      unsigned int secs = seconds % 60;
      char buf[32];
      snprintf(buf, sizeof(buf), "%u:%02u", mins, secs);
      return std::string(buf);
    };

    if (m_timeText) {
      std::string timeStr = "0:00 / 0:00";
      if (hasActiveTrack) {
        timeStr = formatTime(elapsed) + " / " + formatTime(total);
      }
      m_timeText->rebuild()
          ->text(std::move(timeStr))
          ->align(HT_FONT_ALIGN_LEFT)
          ->commence();
      m_timeText->setGrow(false);
    }
    if (m_seekBar && !m_seekBar->sliding()) {
      m_isUpdatingSeekBar = true;
      float fraction = 0.0f;
      if (total > 0) {
        fraction = static_cast<float>(elapsed) / static_cast<float>(total);
      }
      m_seekBar->rebuild()
          ->min(0.0f)
          ->max(1.0f)
          ->val(fraction)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 10.0F}))
          ->onChanged([this](CSharedPointer<CSliderElement>, float val) {
            if (m_isUpdatingSeekBar)
              return;
            runMpdCommand([val](struct mpd_connection *conn) {
              struct mpd_status *status = mpd_run_status(conn);
              if (status) {
                unsigned total = mpd_status_get_total_time(status);
                if (total > 0) {
                  float seconds = val * static_cast<float>(total);
                  mpd_run_seek_current(conn, seconds, false);
                }
                mpd_status_free(status);
              }
            });
          })
          ->commence();
      m_seekBar->setGrow(true);
      m_isUpdatingSeekBar = false;
    }
    if (m_pauseBtn) {
      auto imgBtn =
          Hyprutils::Memory::dynamicPointerCast<CImageElement>(m_pauseBtn);
      if (imgBtn) {
        auto iconFactory = m_backend->systemIcons();
        auto iconDesc =
            iconFactory ? iconFactory->lookupIcon(stateText == "⏸"
                                                      ? "media-playback-pause"
                                                      : "media-playback-start")
                        : nullptr;
        if (iconDesc) {
          imgBtn->rebuild()->icon(iconDesc)->commence();
        }
      } else {
        auto textBtn =
            Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_pauseBtn);
        if (textBtn) {
          textBtn->rebuild()->text(std::move(stateText))->commence();
        }
      }
    }

    if (m_volumeSlider && !m_volumeSlider->sliding()) {
      m_isUpdatingVolumeSlider = true;
      float fraction = 0.0f;
      if (currentVolume >= 0) {
        fraction = static_cast<float>(currentVolume) / 100.0f;
      }
      m_volumeSlider->rebuild()
          ->min(0.0f)
          ->max(1.0f)
          ->val(fraction)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {0.8F, 0.25F}))
          ->commence();
      m_isUpdatingVolumeSlider = false;
    }
  }

  void setupTimer() {
    m_backend->addTimer(
        std::chrono::seconds(1),
        [this](CAtomicSharedPointer<CTimer>, void *) {
          updateStatus();
          setupTimer();
        },
        nullptr);
  }
};

int main() {
  try {
    HyprMusicApp app;
    app.run();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }
}