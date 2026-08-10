#include "PlaybackBar.hpp"
#include "../../Utils/FormatUtils.hpp"
#include <algorithm>
#include <cmath>
#include "../../Utils/ArtworkUtils.hpp"
#include <hyprtoolkit/system/Icons.hpp>

namespace UI::Components {

	PlaybackBar::PlaybackBar(const PlaybackBarContext &ctx) : m_ctx(ctx) {}

	void PlaybackBar::build(CSharedPointer<CColumnLayoutElement> parentColumn) {
		auto palette = m_ctx.palette;
		std::string fontFamily = m_ctx.fontFamily;
		auto navCallback = m_ctx.onNavigationClick;
		auto onPlayerNavClick = [navCallback](Input::eMouseButton button, bool down) {
			if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
				navCallback(Core::eViewMode::VIEW_PLAYER);
			}
		};
		auto onQueueNavClick = [navCallback](Input::eMouseButton button, bool down) {
			if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
				navCallback(Core::eViewMode::VIEW_QUEUE);
			}
		};
		auto onPlaylistNavClick = [navCallback](Input::eMouseButton button, bool down) {
			if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
				navCallback(Core::eViewMode::VIEW_PLAYLISTS);
			}
		};
		auto onDatabaseNavClick = [navCallback](Input::eMouseButton button, bool down) {
			if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
				navCallback(Core::eViewMode::VIEW_DATABASE);
			}
		};
		auto onYtdlpNavClick = [navCallback](Input::eMouseButton button, bool down) {
			if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
				navCallback(Core::eViewMode::VIEW_YTDLP);
			}
		};
		auto onVisNavClick = [navCallback](Input::eMouseButton button, bool down) {
			if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
				navCallback(Core::eViewMode::VIEW_VISUALIZER);
			}
		};

		// Outer Playback Section container (20% of parentColumn)
		auto playbackSection =
			CRowLayoutBuilder::begin()
			->gap(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.2F}))
			->commence();

		auto leftLayout =
			CRectangleBuilder::begin()
			->color([] { return CHyprColor(0, 0, 0, 0); })
			->rounding(8)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT, {0.2F, 1.0F}))
			->commence();

		leftLayout->setMargin(8);

		CardViewConfig cardCfg{
			.palette = palette,
			.fontFamily = fontFamily,
			.imagePath = Utils::getDefaultArtworkPath(),
			.title = "No currently playing songs",
			.subtitle = "",
			.text = "No currently playing songs",
			.onClick = [onPlayerNavClick]() {
				onPlayerNavClick(Input::MOUSE_BUTTON_LEFT, false);
			}
		};
		m_cardView = std::make_unique<CardView>(cardCfg);
		leftLayout->addChild(m_cardView->build());

		auto rightLayout =
			CColumnLayoutBuilder::begin()
			->gap(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT, {0.8F, 1.0F}))
			->commence();

		playbackSection->addChild(leftLayout);
		playbackSection->addChild(rightLayout);

		// 1. Navigation bar section (100% width, 30% height of PlaybackSection)
		m_navigationBar = CRectangleBuilder::begin()
			->color([] { return CHyprColor(0, 0, 0, 0); })
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.30F}))
			->commence();

		// Horizontal Row Layout for navigation tabs
		auto navRow = CRowLayoutBuilder::begin()
			->gap(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
			->commence();

		auto addNavButton = [&](const std::string &iconName, const std::string &fallbackLabel,
					std::function<void(Input::eMouseButton, bool)> &&onClick) {
			auto res = createIconButton(iconName, fallbackLabel, 0.20F, std::move(onClick));
			navRow->addChild(res.container);
		};

		// 1. Queue / List Icon
		addNavButton("music-queue-symbolic", "☰", std::move(onQueueNavClick));

		// 2. Database / Library Icon
		addNavButton("library-music-symbolic", "🗄️", std::move(onDatabaseNavClick));

		// 3. Playlist Icon
		addNavButton("music-playlist-symbolic", "🎶", std::move(onPlaylistNavClick));

		// 4. YT-DLP Icon
		addNavButton("folder-download-symbolic", "📥", std::move(onYtdlpNavClick));

		// 5. Mini Visualizer Container
		auto miniVisContainer = CRectangleBuilder::begin()
			->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.20F, 1.0F}))
			->commence();

		auto miniVisRow = CRowLayoutBuilder::begin()
			->gap(3)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
			->commence();
		miniVisRow->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
		miniVisRow->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

		m_miniVisBars.clear();
		float defaultHeights[4] = {12.0f, 6.0f, 15.0f, 8.0f};
		for (int i = 0; i < 4; ++i) {
			auto bar = CRectangleBuilder::begin()
				->color([this, palette] {
					if (m_activeViewMode == Core::eViewMode::VIEW_VISUALIZER) {
						return palette ? palette->m_colors.accent : CHyprColor(0.35F, 0.65F, 1.0F, 1.0F);
					}
					return palette ? palette->m_colors.text : CHyprColor(0.6F, 0.6F, 0.6F, 1.0F);
				})
				->rounding(2)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {3.0F, defaultHeights[i]}))
				->commence();
			m_miniVisBars.push_back(bar);
			miniVisRow->addChild(bar);
		}
		miniVisContainer->addChild(miniVisRow);
		miniVisContainer->setReceivesMouse(true);
		miniVisContainer->setMouseButton(onVisNavClick);
		navRow->addChild(miniVisContainer);

		m_navigationBar->addChild(navRow);
		rightLayout->addChild(m_navigationBar);

		// 2. Seek bar section (30% of PlaybackSection)
		auto seekBarSection =
			CRectangleBuilder::begin()
			->color([palette] {
					return palette ? palette->m_colors.background
					: CHyprColor(0.15, 0.15, 0.15, 1.0);
					})
		->rounding(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.30F}))
			->commence();

		auto seekBarRow =
			CRowLayoutBuilder::begin()
			->gap(12)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
			->commence();
		seekBarRow->setMargin(10);

		m_timeText =
			CTextBuilder::begin()
			->text(std::string("0:00 / 0:00"))
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

		// Custom Seekbar Initialization
		CustomSeekBar::Context seekCtx{
			.window = m_ctx.window,
				.palette = palette,
				.onSeek = [this](float pct) {
					m_ctx.runMpdCommand([pct](struct mpd_connection *conn) {
							struct mpd_status *status = mpd_run_status(conn);
							if (status) {
							unsigned total = mpd_status_get_total_time(status);
							if (total > 0) {
							float seconds = pct * static_cast<float>(total);
							mpd_run_seek_current(conn, seconds, false);
							}
							mpd_status_free(status);
							}
							});
				},
				.size = CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {0.0F, 28.0F})
		};

		m_customSeekBar = std::make_unique<CustomSeekBar>(seekCtx);
		auto seekBarElem = m_customSeekBar->build();
		seekBarElem->setGrow(true); // Now stretches properly since width type is absolute

		seekBarRow->addChild(seekBarElem);
		seekBarRow->addChild(m_timeText);
		seekBarSection->addChild(seekBarRow);
		rightLayout->addChild(seekBarSection);

		// 3. Controls section (40% of PlaybackSection)
		auto iconFactory = m_ctx.backend ? m_ctx.backend->systemIcons() : nullptr;

		auto controlsSection =
			CRectangleBuilder::begin()
			->color([palette] {
					return palette ? palette->m_colors.background
					: CHyprColor(0.15, 0.15, 0.15, 1.0);
					})
		->rounding(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.40F}))
			->commence();

		auto controlsLayout =
			CRowLayoutBuilder::begin()
			->gap(12)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
			->commence();
		controlsLayout->setMargin(8);

		auto mainControlsRow = CRowLayoutBuilder::begin()
			->gap(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
						CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
			->commence();
		mainControlsRow->setGrow(true);

		auto addControlColumn = [&](const std::string &iconName, const std::string &fallbackLabel,
				std::function<void(Input::eMouseButton, bool)> &&onClick) {
			auto res = createIconButton(iconName, fallbackLabel, 0.25F, std::move(onClick));
			mainControlsRow->addChild(res.container);
			return res.iconElem;
		};

		// 1. Skip Backward
		addControlColumn("media-skip-backward", "⏮", [this](Input::eMouseButton button, bool down) {
				if (button == Input::MOUSE_BUTTON_LEFT && !down) {
				if (m_ctx.prevTrack) m_ctx.prevTrack();
				}
				});

		// 2. Play / Pause
		m_pauseBtn = addControlColumn("media-playback-start", "▶", [this](Input::eMouseButton button, bool down) {
				if (button == Input::MOUSE_BUTTON_LEFT && !down) {
				if (m_ctx.togglePlayPause) m_ctx.togglePlayPause();
				}
				});

		// 3. Skip Forward
		addControlColumn("media-skip-forward", "⏭", [this](Input::eMouseButton button, bool down) {
				if (button == Input::MOUSE_BUTTON_LEFT && !down) {
				if (m_ctx.nextTrack) m_ctx.nextTrack();
				}
				});

		// 4. Volume Column
		{
			auto volCol = CRectangleBuilder::begin()
				->color([] { return CHyprColor(0, 0, 0, 0); })
				->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
							CDynamicSize::HT_SIZE_PERCENT, {0.25F, 1.0F}))
				->commence();

			auto volRow = CRowLayoutBuilder::begin()
				->gap(8)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
							CDynamicSize::HT_SIZE_PERCENT, {0.85F, 1.0F}))
				->commence();
			CSharedPointer<ISystemIconDescription> iconDesc;
			if (iconFactory) {
				iconDesc = iconFactory->lookupIcon(m_isMuted ? "audio-volume-muted"
						: "audio-volume-high");
			}

			if (iconDesc) {
				m_volIcon = CImageBuilder::begin()
					->icon(iconDesc)
					->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
								CDynamicSize::HT_SIZE_ABSOLUTE, {18.0F, 18.0F}))
					->fitMode(IMAGE_FIT_MODE_CONTAIN)
					->commence();
			} else {
				m_volIcon = CTextBuilder::begin()
					->text(std::string(m_isMuted ? "🔇" : "🔊"))
					->color([palette] {
							return palette ? palette->m_colors.text
							: CHyprColor(1.0, 1.0, 1.0, 1.0);
							})
				->fontFamily(std::string(fontFamily))
					->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
					->align(HT_FONT_ALIGN_CENTER)
					->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
								CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
					->interactable(true)
					->commence();
			}
			m_volIcon->setGrow(false);
			m_volIcon->setReceivesMouse(true);
			m_volIcon->setMouseButton([this](Input::eMouseButton button, bool down) {
					if (button == Input::MOUSE_BUTTON_LEFT && !down) {
					if (m_isMuted) {
					int targetVol = (m_lastUnmutedVolume > 0) ? m_lastUnmutedVolume : 50;
					m_isMuted = false;
					m_ctx.runMpdCommand([targetVol](struct mpd_connection *conn) {
							mpd_run_set_volume(conn, targetVol);
							});
					updateVolume(targetVol);
					} else {
					m_isMuted = true;
					m_ctx.runMpdCommand([](struct mpd_connection *conn) { mpd_run_set_volume(conn, 0); });
					updateVolume(0);
					}
					}
					});

			CustomSeekBar::Context volCtx{
				.window = m_ctx.window,
				.palette = palette,
				.onSeek = [this](float pct) {
					int vol = std::clamp(static_cast<int>(pct * 100.0f), 0, 100);
					if (vol > 0) {
						m_isMuted = false;
						m_lastUnmutedVolume = vol;
						updateVolumeIconState(false);
					} else {
						m_isMuted = true;
						updateVolumeIconState(true);
					}
					m_ctx.runMpdCommand([vol](struct mpd_connection *conn) {
						mpd_run_set_volume(conn, vol);
					});
				},
				.size = CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {0.0F, 10.0F}),
				.rounding = 2
			};
			m_customVolumeBar = std::make_unique<CustomSeekBar>(volCtx);
			auto volElem = m_customVolumeBar->build();
			volElem->setGrow(true);

			volRow->addChild(m_volIcon);
			volRow->addChild(volElem);
			volRow->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
			volRow->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

			volCol->addChild(volRow);
			mainControlsRow->addChild(volCol);
		}

		controlsLayout->addChild(mainControlsRow);

		// Settings Icon Wrapper
		auto settingsWrapper = CRectangleBuilder::begin()
			->color([] { return CHyprColor(0, 0, 0, 0); })
			->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
						CDynamicSize::HT_SIZE_PERCENT, {48.0F, 1.0F}))
			->commence();

		auto settingsBg = CRectangleBuilder::begin()
			->color([palette] { 
					return palette ? palette->m_colors.base 
					: CHyprColor(0.18, 0.18, 0.18, 1.0); 
					})
		->borderThickness(1)
			->borderColor([palette] {
					return palette ? palette->m_colors.alternateBase
					: CHyprColor(0.30, 0.30, 0.30, 1.0);
					})
		->rounding(16)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
						CDynamicSize::HT_SIZE_ABSOLUTE, {32.0F, 32.0F}))
			->commence();
		settingsBg->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
		settingsBg->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

		CSharedPointer<ISystemIconDescription> settingsIconDesc;
		if (iconFactory) {
			settingsIconDesc = iconFactory->lookupIcon("preferences-system");
		}

		CSharedPointer<IElement> settingsIconBtn;
		if (settingsIconDesc) {
			settingsIconBtn = CImageBuilder::begin()
				->icon(settingsIconDesc)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
							CDynamicSize::HT_SIZE_PERCENT, {0.55F, 0.55F}))
				->fitMode(IMAGE_FIT_MODE_CONTAIN)
				->commence();
		} else {
			settingsIconBtn = CTextBuilder::begin()
				->text("⚙")
				->color([palette] {
						return palette ? palette->m_colors.text
						: CHyprColor(1.0, 1.0, 1.0, 1.0);
						})
			->fontFamily(std::string(fontFamily))
				->fontSize(CFontSize(CFontSize::HT_FONT_H3))
				->align(HT_FONT_ALIGN_CENTER)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
							CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
				->commence();
		}
		settingsIconBtn->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
		settingsIconBtn->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

		settingsBg->addChild(settingsIconBtn);
		settingsWrapper->addChild(settingsBg);

		settingsWrapper->setReceivesMouse(true);
		settingsWrapper->setMouseButton([this](Input::eMouseButton button, bool down) {
				if (button == Input::MOUSE_BUTTON_LEFT && !down) {
				if (m_ctx.onNavigationClick) {
				m_ctx.onNavigationClick(Core::eViewMode::VIEW_SETTINGS);
				}
				}
				});

		controlsLayout->addChild(settingsWrapper);	
		controlsSection->addChild(controlsLayout);
		rightLayout->addChild(controlsSection);	
		parentColumn->addChild(playbackSection);
	}

	void PlaybackBar::updateTrackInfo(const std::string &title,
			const std::string &artist,
			bool hasActiveTrack, unsigned elapsed,
			unsigned total) {
		if (m_cardView) {
			if (hasActiveTrack) {
				m_cardView->updateInfo(title, artist);
			} else {
				m_cardView->updateInfo("No currently playing songs", "");
			}
		}
		if (m_timeText) {
			std::string timeStr = "0:00 / 0:00";
			if (hasActiveTrack && total > 0) {
				timeStr = Utils::formatTime(elapsed) + " / " + Utils::formatTime(total);
			}
			m_timeText->rebuild()->text(std::string(timeStr))->commence();
		}

		if (m_customSeekBar) {
			float progress = 0.0f;
			if (hasActiveTrack && total > 0) {
				progress = std::clamp(static_cast<float>(elapsed) / static_cast<float>(total), 0.0f, 1.0f);
			}
			m_customSeekBar->updateProgress(progress, hasActiveTrack);
		}
	}

	void PlaybackBar::updateVolumeIconState(bool muted) {
		if (!m_volIcon)
			return;

		auto imgBtn = Hyprutils::Memory::dynamicPointerCast<CImageElement>(m_volIcon);
		if (imgBtn) {
			auto iconFactory = m_ctx.backend->systemIcons();
			auto iconDesc = iconFactory
				? iconFactory->lookupIcon(muted ? "audio-volume-muted"
						: "audio-volume-high")
				: nullptr;
			if (iconDesc) {
				imgBtn->rebuild()->icon(iconDesc)->commence();
			}
		} else {
			auto textBtn =
				Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_volIcon);
			if (textBtn) {
				std::string iconChar = muted ? "🔇" : "🔊";
				textBtn->rebuild()->text(std::move(iconChar))->commence();
			}
		}
	}

	void PlaybackBar::updateVolume(int currentVolume) {
		if (currentVolume >= 0) {
			if (currentVolume == 0) {
				m_isMuted = true;
				updateVolumeIconState(true);
			} else {
				m_isMuted = false;
				m_lastUnmutedVolume = currentVolume;
				updateVolumeIconState(false);
			}
		}

		if (m_customVolumeBar) {
			float fraction = 0.0f;
			if (currentVolume >= 0) {
				fraction = static_cast<float>(currentVolume) / 100.0f;
			}
			m_customVolumeBar->updateProgress(fraction, true);
		}
	}

	void PlaybackBar::updatePlayPauseState(const std::string &stateText) {
		bool wasPlaying = m_isPlaying;
		m_isPlaying = (stateText == "media-playback-pause");

		updateMiniVisBars();
		if (m_isPlaying && (!wasPlaying || !m_isMiniVisAnimating)) {
			scheduleMiniVisAnimation();
		}

		if (!m_pauseBtn)
			return;

		auto imgBtn =
			Hyprutils::Memory::dynamicPointerCast<CImageElement>(m_pauseBtn);
		if (imgBtn) {
			auto iconFactory = m_ctx.backend->systemIcons();
			auto iconDesc =
				iconFactory
				? iconFactory->lookupIcon(stateText)
				: nullptr;
			if (iconDesc) {
				imgBtn->rebuild()->icon(iconDesc)->commence();
			}
		} else {
			auto textBtn =
				Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_pauseBtn);
			if (textBtn) {
				textBtn->rebuild()->text(std::string(m_isPlaying ? "⏸" : "▶"))->commence();
			}
		}
	}

	void PlaybackBar::applyAlbumArt(const std::string &artPath) {
		if (m_cardView) {
			m_cardView->updateImage(artPath);
		}
	}

	void PlaybackBar::updateAlbumArt(const std::string &songUri) {
		if (m_lastSongUri == songUri) return;
		m_lastSongUri = songUri;

		std::string artPath = Utils::getDefaultArtworkPath();

		if (songUri.empty()) {
			applyAlbumArt(artPath);
			return;
		}

		m_ctx.runMpdCommand([this, artPath](struct mpd_connection *conn) mutable {
				if (conn) {
				artPath = Utils::resolveTrackArtwork(conn, m_lastSongUri);
				}
				applyAlbumArt(artPath);
				});
	}

	PlaybackBar::IconButtonResult PlaybackBar::createIconButton(
		const std::string &iconName,
		const std::string &fallbackLabel,
		float containerWidthPct,
		std::function<void(Input::eMouseButton, bool)> onClick) {

		auto palette = m_ctx.palette;
		std::string fontFamily = m_ctx.fontFamily;

		auto btnContainer = CRectangleBuilder::begin()
			->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {containerWidthPct, 1.0F}))
			->commence();

		CSharedPointer<IElement> iconElem;
		CSharedPointer<ISystemIconDescription> iconDesc;
		auto iconFactory = m_ctx.backend ? m_ctx.backend->systemIcons() : nullptr;
		if (iconFactory && !iconName.empty()) {
			iconDesc = iconFactory->lookupIcon(iconName);
		}

		CSharedPointer<CTextElement> textLabelElem;
		if (iconDesc) {
			iconElem = CImageBuilder::begin()
				->icon(iconDesc)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.6F, 0.6F}))
				->fitMode(IMAGE_FIT_MODE_CONTAIN)
				->commence();
		} else {
			textLabelElem = CTextBuilder::begin()
				->text(std::string(fallbackLabel))
				->color([palette] {
					return palette ? palette->m_colors.text : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
				})
				->fontFamily(std::string(fontFamily))
				->fontSize(CFontSize(CFontSize::HT_FONT_H3))
				->align(HT_FONT_ALIGN_CENTER)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
				->interactable(true)
				->commence();
			iconElem = textLabelElem;
		}

		iconElem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
		iconElem->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
		btnContainer->addChild(iconElem);

		btnContainer->setReceivesMouse(true);
		btnContainer->setMouseButton(std::move(onClick));

		return {btnContainer, textLabelElem, iconElem};
	}

	void PlaybackBar::updateMiniVisBars() {
		if (m_miniVisBars.size() < 4) return;

		auto palette = m_ctx.palette;
		if (m_isPlaying) {
			m_miniVisAnimPhase += 0.25f;
			float phases[4] = {0.0f, 1.2f, 2.4f, 3.6f};
			float multipliers[4] = {1.0f, 0.75f, 0.9f, 0.8f};

			for (size_t i = 0; i < 4; ++i) {
				float val = std::abs(std::sin(m_miniVisAnimPhase + phases[i])) * multipliers[i];
				float height = 3.0f + val * 13.0f; // Height range: 3px to 16px
				m_miniVisBars[i]->rebuild()
					->color([palette] { return palette ? palette->m_colors.accent : CHyprColor(0.2F, 0.8F, 0.4F, 1.0F); })
					->rounding(2)
					->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {3.0F, height}))
					->commence();
			}
		} else {
			// Static resting placeholder state when paused: unequal bars of various heights
			float defaultHeights[4] = {12.0f, 6.0f, 15.0f, 8.0f};
			for (size_t i = 0; i < 4; ++i) {
				m_miniVisBars[i]->rebuild()
					->color([palette] { return palette ? palette->m_colors.text : CHyprColor(0.6F, 0.6F, 0.6F, 1.0F); })
					->rounding(2)
					->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {3.0F, defaultHeights[i]}))
					->commence();
			}
		}
	}

	void PlaybackBar::scheduleMiniVisAnimation() {
		if (!m_ctx.backend || m_isMiniVisAnimating) return;

		m_isMiniVisAnimating = true;

		m_ctx.backend->addTimer(
			std::chrono::milliseconds(50), // ~20 FPS, virtually 0 CPU usage
			[this](CAtomicSharedPointer<CTimer>, void *) {
				updateMiniVisBars();

				if (m_isPlaying) {
					m_isMiniVisAnimating = false;
					scheduleMiniVisAnimation();
				} else {
					m_isMiniVisAnimating = false;
				}
			},
			nullptr);
	}

} // namespace UI::Components
