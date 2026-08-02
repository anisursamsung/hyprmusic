#include "PlayerView.hpp"
#include "../../Utils/ArtworkUtils.hpp"
#include <hyprtoolkit/system/Icons.hpp>
#include "../Components/Visualizer.hpp"
#include <algorithm>
#include <filesystem>

namespace UI::Views {

PlayerView::PlayerView(const PlayerViewContext &ctx) : m_ctx(ctx) {}

static CSharedPointer<IElement> buildCardElement(CSharedPointer<CPalette> palette,
                                                 CSharedPointer<IBackend> backend,
                                                 const std::string &artPath) {
  int rounding = 0;
  std::string artPathStr = artPath;
  if (artPathStr.empty()) {
    artPathStr = Utils::getDefaultArtworkPath();
  }
  if (!artPathStr.empty()) {
    auto img = CImageBuilder::begin()
                   ->path(std::string(artPathStr))
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 1.0F}))
                   ->rounding(rounding)
                   ->fitMode(IMAGE_FIT_MODE_CONTAIN)
                   ->commence();
    img->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    img->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    return img;
  }

  auto card = CRectangleBuilder::begin()
                  ->color([palette] {
                    return palette ? palette->m_colors.alternateBase
                                   : CHyprColor(0.12, 0.12, 0.12, 1.0);
                  })
                  ->rounding(rounding)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_PERCENT,
                                      {1.0F, 1.0F}))
                  ->commence();
  card->setGrow(true, true);
  card->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  card->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  auto iconFactory = backend ? backend->systemIcons() : nullptr;
  CSharedPointer<ISystemIconDescription> musicIcon;
  if (iconFactory) {
    musicIcon = iconFactory->lookupIcon("media-optical-audio-symbolic");
    if (!musicIcon)
      musicIcon = iconFactory->lookupIcon("audio-x-generic");
  }

  CSharedPointer<IElement> iconElem;
  if (musicIcon) {
    iconElem = CImageBuilder::begin()
                   ->icon(musicIcon)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {0.6F, 0.6F}))
                   ->fitMode(IMAGE_FIT_MODE_CONTAIN)
                   ->commence();
  } else {
    iconElem = CTextBuilder::begin()
                   ->text(std::string("🎵"))
                   ->color([palette] {
                     return palette ? palette->m_colors.text
                                    : CHyprColor(1.0, 1.0, 1.0, 1.0);
                   })
                   ->fontSize(CFontSize(CFontSize::HT_FONT_H1))
                   ->align(HT_FONT_ALIGN_CENTER)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                       CDynamicSize::HT_SIZE_AUTO,
                                       {1.0F, 1.0F}))
                   ->commence();
  }
  iconElem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  iconElem->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
  card->addChild(iconElem);
  return card;
}

static CSharedPointer<IElement> buildVignetteOverlay(CSharedPointer<CPalette> palette) {
  auto overlay =
      CRectangleBuilder::begin()
          ->color([palette] {
            if (palette) {
              auto bg = palette->m_colors.background;
              return CHyprColor(bg.r, bg.g, bg.b, 0.50F);
            }
            return CHyprColor(0.12F, 0.12F, 0.12F, 0.50F);
          })
          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {1.0F, 1.0F}))
          ->commence();
  overlay->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  overlay->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
  return overlay;
}

static CSharedPointer<IElement> buildBackgroundElement(CSharedPointer<CPalette> palette,
                                                       const std::string &artPath) {
  std::string artPathStr = artPath;
  if (artPathStr.empty()) {
    artPathStr = Utils::getDefaultArtworkPath();
  }
  if (!artPathStr.empty()) {
    auto img = CImageBuilder::begin()
                   ->path(std::string(artPathStr))
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 1.0F}))
                   ->rounding(0)
                   ->fitMode(IMAGE_FIT_MODE_COVER)
                   ->commence();
    img->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    img->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    return img;
  }

  auto card = CRectangleBuilder::begin()
                  ->color([palette] {
                    return palette ? palette->m_colors.background
                                   : CHyprColor(0.12, 0.12, 0.12, 1.0);
                  })
                  ->rounding(0)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_PERCENT,
                                      {1.0F, 1.0F}))
                  ->commence();
  card->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  card->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
  return card;
}



static CSharedPointer<IElement> buildMenuButton(CSharedPointer<CPalette> palette,
                                                 CSharedPointer<IBackend> /* backend */,
                                                 const std::string &fontFamily,
                                                 std::function<void()> onClick) {
  // Create a wider pill-shaped button instead of a circle
  auto btnContainer =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
           })
          ->rounding(16) 
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE,
                              {110.0F, 32.0F}))
          ->commence();

  // Create the text element
  auto textElem = CTextBuilder::begin()
                  ->text("Visualizer")
                  ->color([palette] {
                    return palette ? palette->m_colors.background : CHyprColor(0.12, 0.12, 0.12, 1.0);
                  })
                  ->fontFamily(std::string(fontFamily))
                  ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                  ->align(HT_FONT_ALIGN_CENTER)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_AUTO,
                                      {1.0F, 1.0F}))
                  ->commence();

  textElem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  textElem->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  btnContainer->addChild(textElem);
  btnContainer->setReceivesMouse(true);
  btnContainer->setMouseButton([onClick](Input::eMouseButton button, bool down) {
    if (button == Input::MOUSE_BUTTON_LEFT && !down) {
      if (onClick) onClick();
    }
  });

  // Anchor it to the top right corner
  btnContainer->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  btnContainer->setPositionFlag(IElement::HT_POSITION_FLAG_RIGHT, true);
  btnContainer->setPositionFlag(IElement::HT_POSITION_FLAG_TOP, true);
  btnContainer->setAbsolutePosition({-20.0F, 20.0F});

  return btnContainer;
}

void PlayerView::toggleVisualizer() {
	m_showVisualizer = !m_showVisualizer;

	m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
			std::string artPath = Utils::resolveTrackArtwork(conn, m_lastSongUri);
			if (m_tabContentWrapper) {
			m_tabContentWrapper->clearChildren();

			m_coverCardElementBackground = buildBackgroundElement(m_ctx.palette, artPath);
			m_vignetteOverlay = buildVignetteOverlay(m_ctx.palette);
			m_coverCardElement = buildCardElement(m_ctx.palette, m_ctx.backend, artPath);

			m_menuBtn = buildMenuButton(m_ctx.palette, m_ctx.backend, m_ctx.fontFamily, [this]() {
					toggleVisualizer();
					});


			// Conditionally add the correct centered element
			if (m_showVisualizer) {
			// Instantiate the visualizer if it doesn't exist
			if (!m_visualizer) {
			m_visualizer = std::make_shared<UI::Components::Visualizer>(m_ctx.backend, m_ctx.palette);
			}
			m_tabContentWrapper->addChild(m_visualizer->build());
			} else {
				// Kill the visualizer thread and free memory when returning to album art
				m_visualizer.reset(); 

				m_tabContentWrapper->addChild(m_coverCardElementBackground);
				m_tabContentWrapper->addChild(m_vignetteOverlay);
				m_tabContentWrapper->addChild(m_coverCardElement);
			}

			m_tabContentWrapper->addChild(m_menuBtn);
			m_tabContentWrapper->forceReposition();
			}
	});
}

void PlayerView::rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn) {
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  m_tabContentWrapper->clearChildren();

  std::string currentUri = "";
  if (conn) {
    struct mpd_song *s = mpd_run_current_song(conn);
    if (s) {
      const char *u = mpd_song_get_uri(s);
      if (u)
        currentUri = u;
      mpd_song_free(s);
    }
  }
  m_lastSongUri = currentUri;

  std::string artPath = Utils::resolveTrackArtwork(conn, currentUri);
  m_coverCardElementBackground = buildBackgroundElement(m_ctx.palette, artPath);
  m_vignetteOverlay = buildVignetteOverlay(m_ctx.palette);
  m_coverCardElement = buildCardElement(m_ctx.palette, m_ctx.backend, artPath);
  
  m_menuBtn = buildMenuButton(m_ctx.palette, m_ctx.backend, m_ctx.fontFamily, [this]() {
    toggleVisualizer();
  });

 

// Conditionally add the correct centered element
  if (m_showVisualizer) {
    // Instantiate the visualizer if it doesn't exist
    if (!m_visualizer) {
      m_visualizer = std::make_shared<UI::Components::Visualizer>(m_ctx.backend, m_ctx.palette);
    }
    m_tabContentWrapper->addChild(m_visualizer->build());
  } else {
    // Kill the visualizer thread and free memory when returning to album art
    m_visualizer.reset(); 
    
    m_tabContentWrapper->addChild(m_coverCardElementBackground);
    m_tabContentWrapper->addChild(m_vignetteOverlay);
    m_tabContentWrapper->addChild(m_coverCardElement);
  }

  
  m_tabContentWrapper->addChild(m_menuBtn);
  m_tabContentWrapper->forceReposition();
}

void PlayerView::updateTrackInfo(const std::string & /*trackText*/, bool /*hasActiveTrack*/,
                               unsigned /*elapsed*/, unsigned /*total*/, const std::string &songUri) {
  if (m_lastSongUri != songUri) {
    m_lastSongUri = songUri;

    m_ctx.runMpdCommand([this, songUri](struct mpd_connection *conn) {
      std::string artPath = Utils::resolveTrackArtwork(conn, songUri);
      if (m_tabContentWrapper) {
        m_tabContentWrapper->clearChildren();

        m_coverCardElementBackground = buildBackgroundElement(m_ctx.palette, artPath);
        m_vignetteOverlay = buildVignetteOverlay(m_ctx.palette);
        m_coverCardElement = buildCardElement(m_ctx.palette, m_ctx.backend, artPath);
        
        m_menuBtn = buildMenuButton(m_ctx.palette, m_ctx.backend, m_ctx.fontFamily, [this]() {
          toggleVisualizer();
        });

      // Conditionally add the correct centered element
  if (m_showVisualizer) {
    // Instantiate the visualizer if it doesn't exist
    if (!m_visualizer) {
      m_visualizer = std::make_shared<UI::Components::Visualizer>(m_ctx.backend, m_ctx.palette);
    }
    m_tabContentWrapper->addChild(m_visualizer->build());
  } else {
    // Kill the visualizer thread and free memory when returning to album art
    m_visualizer.reset(); 
    
    m_tabContentWrapper->addChild(m_coverCardElementBackground);
    m_tabContentWrapper->addChild(m_vignetteOverlay);
    m_tabContentWrapper->addChild(m_coverCardElement);
  }




        m_tabContentWrapper->addChild(m_menuBtn);
        m_tabContentWrapper->forceReposition();
      }
    });
  }
}

} // namespace UI::Views
