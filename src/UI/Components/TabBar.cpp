#include "TabBar.hpp"
#include <hyprtoolkit/element/ScrollArea.hpp>
namespace UI::Components {

	TabBar::TabBar(CSharedPointer<CPalette> palette, const std::string &fontFamily,
			std::function<void(Core::eViewMode)> onSwitchMode)
		: m_palette(palette), m_fontFamily(fontFamily), m_onSwitchMode(onSwitchMode) {}

	CSharedPointer<CRectangleElement> TabBar::build() {
		auto palette = m_palette;

		// Outer container (50px tall)
		auto tabsSection =
			CRectangleBuilder::begin()
			->color([palette] {
					return palette ? palette->m_colors.background
					: CHyprColor(0.15, 0.15, 0.15, 1.0);
					})
		->rounding(0)
			->size(CDynamicSize(
						CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_ABSOLUTE,
						{1.0F, 50.0F}))
		->commence();

		tabsSection->setGrow(false);

		//
		// Horizontal Scroll Area
		//
		auto scrollArea =
			CScrollAreaBuilder::begin()
			->scrollX(true)
			->scrollY(false)
			->blockUserScroll(false)
			->size(CDynamicSize(
						CDynamicSize::HT_SIZE_PERCENT,
						CDynamicSize::HT_SIZE_PERCENT,
						{1.0F, 1.0F}))
		->commence();
		//
		// Row of tabs
		//
		m_tabsRow =
			CRowLayoutBuilder::begin()
			->gap(12)
			->size(CDynamicSize(
						CDynamicSize::HT_SIZE_AUTO,
						CDynamicSize::HT_SIZE_ABSOLUTE,
						{1.0F, 32.0F}))
		->commence();

		m_tabsRow->setGrow(false);

		// Vertically center only.
		// Do NOT horizontally center when using a ScrollArea.
		m_tabsRow->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
		m_tabsRow->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);

		populateTabs();

		scrollArea->addChild(m_tabsRow);
		tabsSection->addChild(scrollArea);

		return tabsSection;
	}

	void TabBar::populateTabs() {
		if (!m_tabsRow) return;
		m_tabsRow->clearChildren(); // Wipe old UI to bypass text caching bugs

		auto palette = m_palette;
		std::string fontFamily = m_fontFamily;

		struct TabDef {
			std::string label;
			Core::eViewMode mode;
		};

		std::vector<TabDef> tabs = {
			{"Player", Core::eViewMode::VIEW_PLAYER},
			{"Queue", Core::eViewMode::VIEW_QUEUE},
			{"Database", Core::eViewMode::VIEW_DATABASE},
			{"Playlists", Core::eViewMode::VIEW_PLAYLISTS},
			{"YT DLP", Core::eViewMode::VIEW_YTDLP},
			{"Settings", Core::eViewMode::VIEW_SETTINGS},
			{"Help", Core::eViewMode::VIEW_HELP},
		};

		for (const auto &tab : tabs) {
			auto mode = tab.mode;
			bool isActive = (mode == m_activeMode); // Check state during creation

			// Pill Background with Ghost Border
			auto pill = 
				CRectangleBuilder::begin()
				->color([] { return CHyprColor(0, 0, 0, 0); }) // Ghost style transparent
				->borderThickness(1)
				->borderColor([palette, isActive] {
						if (isActive) return palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
						return palette ? palette->m_colors.alternateBase : CHyprColor(0.18, 0.18, 0.18, 1.0);
						})
			->rounding(16)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
							CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
				->commence();

			pill->setReceivesMouse(true);
			pill->setGrow(false);
			pill->setMouseButton([this, mode](Input::eMouseButton button, bool down) {
					if (button == Input::MOUSE_BUTTON_LEFT && !down) {
					if (m_onSwitchMode) m_onSwitchMode(mode);
					}
					});

			auto textLayout = 
				CRowLayoutBuilder::begin()
				->gap(0)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
							CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
				->commence();

			auto padLeft = CRectangleBuilder::begin()
				->color([] { return CHyprColor(0, 0, 0, 0); })
				->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, 
							CDynamicSize::HT_SIZE_PERCENT, {16.0F, 1.0F}))
				->commence();
			padLeft->setGrow(false);
			textLayout->addChild(padLeft);

			auto tabBtnText =
				CTextBuilder::begin()
				->text(std::string(tab.label))
				->color([palette, isActive] {
						// Color is assigned correctly the moment the element is generated
						if (isActive) return palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
						return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
						})
			->fontFamily(std::string(fontFamily))
				->fontSize(CFontSize(CFontSize::HT_FONT_H3))
				->align(HT_FONT_ALIGN_CENTER)
				->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
							CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
				->commence();
			textLayout->addChild(tabBtnText);

			auto padRight = CRectangleBuilder::begin()
				->color([] { return CHyprColor(0, 0, 0, 0); })
				->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, 
							CDynamicSize::HT_SIZE_PERCENT, {16.0F, 1.0F}))
				->commence();
			padRight->setGrow(false);
			textLayout->addChild(padRight);

			pill->addChild(textLayout);





			m_tabsRow->addChild(pill);
		}
	}

	void TabBar::updateActiveTab(Core::eViewMode activeMode) {
		m_activeMode = activeMode;
		populateTabs(); // Recreate elements with fresh active/inactive colors
		if (m_tabsRow) m_tabsRow->forceReposition(); // Force UI to redraw
	}

} // namespace UI::Components
