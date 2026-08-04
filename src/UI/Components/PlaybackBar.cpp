#include "PlaybackBar.hpp"
#include "../../Utils/FormatUtils.hpp"
#include <algorithm>
#include "../../Utils/ArtworkUtils.hpp"
#include <hyprtoolkit/system/Icons.hpp>

namespace UI::Components {

	PlaybackBar::PlaybackBar(const PlaybackBarContext &ctx) : m_ctx(ctx) {}

	void PlaybackBar::build(CSharedPointer<CColumnLayoutElement> parentColumn) {
		auto palette = m_ctx.palette;
		std::string fontFamily = m_ctx.fontFamily;

		// Outer Playback Section container (20% of parentColumn)
		auto playbackSection =
			CRectangleBuilder::begin()
			->color([palette] {
					return palette ? palette->m_colors.background
					: CHyprColor(0.15, 0.15, 0.15, 1.0);
					})
		->rounding(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.20F}))
			->commence();

		auto playbackLayout =
			CColumnLayoutBuilder::begin()
			->gap(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
			->commence();
		playbackSection->addChild(playbackLayout);

	// 1. Song info section (100% width, 30% height of PlaybackSection)
		auto songInfoSection = CRectangleBuilder::begin()
			->color([palette] { return palette ? palette->m_colors.background : CHyprColor(0.15, 0.15, 0.15, 1.0); })
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.30F}))
			->commence();

		// Horizontal Row Layout to manage the 10-80-10 spacing
		auto infoRow = CRowLayoutBuilder::begin()
			->gap(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
			->commence();

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

		// ==========================================
		// Left 10%: Album Art Container
		// ==========================================
		m_artContainer = CRectangleBuilder::begin()
			->color([] { return CHyprColor(0, 0, 0, 0); })
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.1F, 1.0F}))
			->commence();

		m_artContainer->setReceivesMouse(true);
		m_artContainer->setMouseButton(onPlayerNavClick); 
		infoRow->addChild(m_artContainer);

	// ==========================================
		// Middle 80%: Song Label
		// ==========================================
	// ==========================================
		// Middle 80%: Song Label
		// ==========================================
		//
	// ==========================================
		// Middle 80%: Song Label via CenteredTextLabel
		// ==========================================
		CenteredTextLabelContext txtCtx{
			.text = "Track 1 - Unknown Artist",
			.palette = palette,
			.fontFamily = fontFamily,
			.fontSize = CFontSize(CFontSize::HT_FONT_TEXT),
			.color = [palette] { return palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0); },
			.widthPercent = 1.0f // Fills the 80% container fully
		};

		m_nowPlayingLabel = std::make_unique<CenteredTextLabel>(txtCtx);
		auto labelElem = m_nowPlayingLabel->build();
		labelElem->setPositionMode(IElement::HT_POSITION_AUTO);
		labelElem->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);

		// Container for the middle 80% cell
		auto textContainer = CRectangleBuilder::begin()
			->color([] { return CHyprColor(0, 0, 0, 0); })
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.80F, 1.0F}))
			->commence();

		labelElem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
		labelElem->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
		textContainer->addChild(labelElem);

		textContainer->setReceivesMouse(true);
		textContainer->setMouseButton(onPlayerNavClick);
infoRow->addChild(textContainer);	

		// Navigate to Player
			// ==========================================
		// Right 10%: List Icon Container
		// ==========================================
		auto listIconContainer = CRectangleBuilder::begin()
			->color([] { return CHyprColor(0, 0, 0, 0); })
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.1F, 1.0F}))
			->commence();

		auto listIconFactory = m_ctx.backend->systemIcons();
		CSharedPointer<ISystemIconDescription> listIconDesc;
		if (listIconFactory) {
			listIconDesc = listIconFactory->lookupIcon("view-list-symbolic");
		}

		CSharedPointer<IElement> listIconElem;
		if (listIconDesc) {
			listIconElem = CImageBuilder::begin()
				->icon(listIconDesc)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {18.0F, 18.0F}))
				->fitMode(IMAGE_FIT_MODE_CONTAIN)
				->commence();
		} else {
			listIconElem = CTextBuilder::begin()
				->text("☰")
				->color([palette] { return palette ? palette->m_colors.text : CHyprColor(1.0, 1.0, 1.0, 1.0); })
				->fontFamily(std::string(fontFamily))
				->fontSize(CFontSize(CFontSize::HT_FONT_H3))
				->align(HT_FONT_ALIGN_CENTER)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
				->commence();
		}

		listIconElem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
		listIconElem->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
		listIconContainer->addChild(listIconElem);

		listIconContainer->setReceivesMouse(true);
		listIconContainer->setMouseButton(onQueueNavClick);
		infoRow->addChild(listIconContainer);

		songInfoSection->addChild(infoRow);
		playbackLayout->addChild(songInfoSection);



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
		seekBarRow->setMargin(8);

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

		m_seekBar =
			CSliderBuilder::begin()
			->min(0.0f)
			->max(1.0f)
			->val(0.0f)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
						CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 18.0F}))
			->onChanged([this](CSharedPointer<CSliderElement>, float val) {
					if (m_isUpdatingSeekBar)
					return;
					m_ctx.runMpdCommand([val](struct mpd_connection *conn) {
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
				auto cursorPos = m_ctx.window->cursorPos();
				auto sliderSize = m_seekBar->size();
				if (sliderSize.x > 0.0) {
				float pct = std::clamp(static_cast<float>(cursorPos.x / sliderSize.x),
						0.0f, 1.0f);
				m_ctx.runMpdCommand([this, pct](struct mpd_connection *conn) {
						struct mpd_status *status = mpd_run_status(conn);
						if (status) {
						unsigned total = mpd_status_get_total_time(status);
						if (total > 0) {
						float seconds = pct * static_cast<float>(total);
						mpd_run_seek_current(conn, seconds, false);
						m_isUpdatingSeekBar = true;
						m_seekBar->rebuild()->val(pct)->commence();
						m_seekBar->setGrow(true);
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
		playbackLayout->addChild(seekBarSection);

	// 3. Controls section (40% of PlaybackSection)
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
			->gap(12) // Gap between the Settings icon and the main row
			->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
			->commence();
		controlsLayout->setMargin(8);

		auto iconFactory = m_ctx.backend->systemIcons();

	// ==========================================
		// Left: Settings Icon in Circular Rectangle
		// ==========================================
		auto settingsWrapper = CRectangleBuilder::begin()
			->color([] { return CHyprColor(0, 0, 0, 0); })
			->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
						CDynamicSize::HT_SIZE_PERCENT, {48.0F, 1.0F}))
			->commence();

		auto settingsBg = CRectangleBuilder::begin()
			->color([palette] { 
					// Solid background to make it look like a real circular button
					return palette ? palette->m_colors.base 
					: CHyprColor(0.18, 0.18, 0.18, 1.0); 
					})
			->borderThickness(1)
			->borderColor([palette] {
					return palette ? palette->m_colors.alternateBase
					: CHyprColor(0.30, 0.30, 0.30, 1.0);
					})
			->rounding(16) // Exactly half of the 32px size for a perfect circle
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
							CDynamicSize::HT_SIZE_PERCENT, {0.55F, 0.55F})) // Scaled down slightly to fit inside the circle
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

		controlsLayout->addChild(settingsWrapper);		// ==========================================
		// Rest of the Area: 4-Cell Row Layout
		// ==========================================
		auto mainControlsRow = CRowLayoutBuilder::begin()
			->gap(0)
			->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
						CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
			->commence();
		mainControlsRow->setGrow(true); // Expands to take the rest of the area

		auto addControlColumn = [&](const std::string &iconName, const std::string &fallbackLabel,
				std::function<void(Input::eMouseButton, bool)> &&onClick) {
			auto col = CRectangleBuilder::begin()
				->color([] { return CHyprColor(0, 0, 0, 0); })
				->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
							CDynamicSize::HT_SIZE_PERCENT, {0.25F, 1.0F})) // 1/4 of remaining space
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
								CDynamicSize::HT_SIZE_PERCENT, {0.6F, 0.6F}))
					->fitMode(IMAGE_FIT_MODE_CONTAIN)
					->commence();
			} else {
				btn = CTextBuilder::begin()
					->text(std::string(fallbackLabel))
					->color([palette] {
							return palette ? palette->m_colors.text
							: CHyprColor(1.0, 1.0, 1.0, 1.0);
							})
					->fontFamily(std::string(fontFamily))
					->fontSize(CFontSize(CFontSize::HT_FONT_H3))
					->align(HT_FONT_ALIGN_CENTER)
					->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
								CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
					->interactable(true)
					->commence();
			}

			btn->setReceivesMouse(true);
			btn->setMouseButton(std::move(onClick));
			btn->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
			btn->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

			col->addChild(btn);
			mainControlsRow->addChild(col);
			return btn;
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

		// 4. Volume Column (Takes the final 1/4 slot)
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
			volRow->setMargin(6);

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

			m_volumeSlider = CSliderBuilder::begin()
				->min(0.0f)
				->max(1.0f)
				->val(1.0f)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
							CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 16.0F}))
				->onChanged([this](CSharedPointer<CSliderElement>, float val) {
						if (m_isUpdatingVolumeSlider) return;
						int vol = std::clamp(static_cast<int>(val * 100.0f), 0, 100);
						if (vol > 0) {
							m_isMuted = false;
							m_lastUnmutedVolume = vol;
							updateVolumeIconState(false);
						} else {
							m_isMuted = true;
							updateVolumeIconState(true);
						}
						m_ctx.runMpdCommand([vol](struct mpd_connection *conn) { mpd_run_set_volume(conn, vol); });
				})
				->commence();
			m_volumeSlider->setReceivesMouse(true);
			m_volumeSlider->setGrow(true);

			m_volumeSlider->setMouseButton([this](Input::eMouseButton button, bool down) {
					if (button == Input::MOUSE_BUTTON_LEFT && down) {
						auto cursorPos = m_ctx.window->cursorPos();
						auto sliderSize = m_volumeSlider->size();
						if (sliderSize.x > 0.0) {
							float pct = std::clamp(static_cast<float>(cursorPos.x / sliderSize.x), 0.0f, 1.0f);
							int vol = std::clamp(static_cast<int>(pct * 100.0f), 0, 100);
							if (vol > 0) {
								m_isMuted = false;
								m_lastUnmutedVolume = vol;
								updateVolumeIconState(false);
							} else {
								m_isMuted = true;
								updateVolumeIconState(true);
							}
							m_ctx.runMpdCommand([this, pct, vol](struct mpd_connection *conn) {
									mpd_run_set_volume(conn, vol);
									m_isUpdatingVolumeSlider = true;
									m_volumeSlider->rebuild()
										->min(0.0f)
										->max(1.0f)
										->val(pct)
										->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
													CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 16.0F}))
										->commence();
									m_volumeSlider->setGrow(true);
									m_isUpdatingVolumeSlider = false;
							});
						}
					}
			});

			volRow->addChild(m_volIcon);
			volRow->addChild(m_volumeSlider);
			volRow->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
			volRow->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

			volCol->addChild(volRow);
			mainControlsRow->addChild(volCol);
		}

		controlsLayout->addChild(mainControlsRow);
		controlsSection->addChild(controlsLayout);
		playbackLayout->addChild(controlsSection);		parentColumn->addChild(playbackSection);
	}

	void PlaybackBar::updateTrackInfo(const std::string &trackText,
			bool hasActiveTrack, unsigned elapsed,
			unsigned total) {
if (m_nowPlayingLabel) {
			std::string textToDisplay =
				hasActiveTrack ? trackText : "No currently playing songs";
				
			// Uses the helper method which completely recreates the internal text node, 
			// eliminating any lingering truncation/ghosting pixels from previous songs.
			m_nowPlayingLabel->updateText(textToDisplay);
		}
		if (m_timeText) {
			std::string timeStr = "0:00 / 0:00";
			if (hasActiveTrack && total > 0) {
				timeStr = Utils::formatTime(elapsed) + " / " + Utils::formatTime(total);
			}
			m_timeText->rebuild()->text(std::string(timeStr))->commence();
		}

		if (m_seekBar && !m_seekBar->sliding()) {
			m_isUpdatingSeekBar = true;
			float progress = 0.0f;
			if (hasActiveTrack && total > 0) {
				progress = std::clamp(
						static_cast<float>(elapsed) / static_cast<float>(total), 0.0f, 1.0f);
			}
			m_seekBar->rebuild()
				->min(0.0f)
				->max(1.0f)
				->val(progress)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
							CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 16.0F}))
				->commence();
			m_seekBar->setGrow(true);
			m_isUpdatingSeekBar = false;
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
				->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
							CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 16.0F}))
				->commence();
			m_volumeSlider->setGrow(true);
			m_isUpdatingVolumeSlider = false;
		}
	}

	void PlaybackBar::updatePlayPauseState(const std::string &stateText) {
	m_isPlaying = (stateText == "media-playback-pause");
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


void PlaybackBar::updateAlbumArt(const std::string &songUri) {
		// Stop execution if the song hasn't changed
		if (m_lastSongUri == songUri) return;
		m_lastSongUri = songUri;

		std::string artPath = Utils::getDefaultArtworkPath();

		// If a song is playing, resolve its actual artwork path
		if (!songUri.empty()) {
			m_ctx.runMpdCommand([&artPath, songUri](struct mpd_connection *conn) {
				artPath = Utils::resolveTrackArtwork(conn, songUri);
			});
		}

		// Destroy the old image and create a brand new one to bypass texture caching
		if (m_artContainer) {
			m_artContainer->clearChildren();

			m_albumArt = CImageBuilder::begin()
				->path(std::string(artPath))
				->fitMode(IMAGE_FIT_MODE_COVER) // Fill the 10% container fully
				->rounding(0)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
				->commence();

			// Absolute positioning is not needed when completely filling the container
			m_artContainer->addChild(m_albumArt);
			
			// Force the UI engine to redraw the container with the new child
			m_artContainer->forceReposition();
		}
	}

} // namespace UI::Components
