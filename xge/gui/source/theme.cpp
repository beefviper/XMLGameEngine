// theme.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#include "theme.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QString>
#include <QStyleFactory>

namespace xge
{
	namespace
	{
		// `from` moved `part` (0 to 1) of the way to `to`.
		QColor mix(const QColor& from, const QColor& to, double part)
		{
			const float amount = static_cast<float>(part);

			return QColor::fromRgbF(
				from.redF() + (to.redF() - from.redF()) * amount,
				from.greenF() + (to.greenF() - from.greenF()) * amount,
				from.blueF() + (to.blueF() - from.blueF()) * amount);
		}

		// Lighter on a dark theme, darker on a light one: the way a border or
		// a shadow goes against the background it sits on.
		QColor towardsEdge(const QColor& color, bool dark, double part)
		{
			return mix(color, dark ? Qt::white : Qt::black, part);
		}

		QString name(const QColor& color)
		{
			return color.name(QColor::HexRgb);
		}

		QString gradient(const QColor& top, const QColor& bottom)
		{
			return QStringLiteral("qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2)").arg(name(top), name(bottom));
		}
	}

	void applyTheme(QApplication& app)
	{
		// Fusion draws the same everywhere, and its spin boxes, check boxes and
		// combo boxes already have the sunken, shaded look; the sheet below
		// adds to the controls it would leave plain.
		if (QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion")))
		{
			app.setStyle(fusion);
		}

		QPalette palette = app.palette();
		const QColor window = palette.color(QPalette::Window);
		const QColor base = palette.color(QPalette::Base);
		const bool dark = window.lightness() < 128;

		// No color of its own: the selection, hover and focus are shades of
		// grey, so nothing depends on (or clashes with) the system's accent
		// color. A darker grey on a light theme and a lighter one on a dark.
		const QColor accent = dark ? QColor(0x86, 0x86, 0x86) : QColor(0x5c, 0x5c, 0x5c);
		palette.setColor(QPalette::Highlight, accent);
		palette.setColor(QPalette::HighlightedText, Qt::white);

		// Every other row a slightly different grey.
		const QColor stripe = mix(base, accent, dark ? 0.16 : 0.09);
		palette.setColor(QPalette::AlternateBase, stripe);
		app.setPalette(palette);

		const QColor border = towardsEdge(window, dark, dark ? 0.45 : 0.38);
		const QColor softBorder = towardsEdge(window, dark, dark ? 0.25 : 0.16);
		const QColor rowLine = mix(base, towardsEdge(base, dark, 1.0), 0.07);

		// Raised buttons: light on top, shaded at the bottom, a darker border.
		const QColor buttonTop = window.lighter(dark ? 140 : 118);
		const QColor buttonBottom = window.darker(dark ? 120 : 110);
		const QColor hoverTop = mix(buttonTop, accent, 0.10);
		const QColor hoverBottom = mix(buttonBottom, accent, 0.10);
		const QColor pressedTop = window.darker(dark ? 130 : 112);
		const QColor pressedBottom = window.darker(dark ? 105 : 100);
		const QColor disabledText = palette.color(QPalette::Disabled, QPalette::ButtonText);

		// Column headings: shaded like a button, with a divider between columns.
		const QColor headerTop = window.lighter(dark ? 125 : 108);
		const QColor headerBottom = window.darker(dark ? 112 : 104);

		// The text boxes that edit a value in a row stand out from the row itself.
		// (Number boxes are left to the style: their arrows are drawn by it, and
		// a sheet that touches the box's border takes them away.)
		const QColor inputBackground = dark ? base.lighter(165) : QColor(Qt::white);

		const QColor hover = mix(base, accent, dark ? 0.30 : 0.18);
		const QColor selectedTop = accent.lighter(112);
		const QColor selectedBottom = accent.darker(108);

		const QString sheet = QStringLiteral(R"(
QPushButton {
	background: %1;
	border: 1px solid %2;
	border-radius: 3px;
	padding: 4px 16px;
	min-width: 56px;
}
QPushButton:hover {
	background: %3;
	border-color: %4;
}
QPushButton:pressed {
	background: %5;
	border-color: %4;
	padding-top: 5px;
	padding-bottom: 3px;
}
QPushButton:disabled {
	background: %6;
	border-color: %7;
	color: %8;
}
QPushButton:default {
	border-color: %4;
}
QHeaderView::section {
	background: %9;
	border: none;
	border-right: 1px solid %7;
	border-bottom: 1px solid %2;
	padding: 4px 8px;
	font-weight: bold;
}
QTreeView {
	border: 1px solid %2;
	alternate-background-color: %10;
	show-decoration-selected: 1;
}
QTreeView::item {
	min-height: 26px;
	border-bottom: 1px solid %11;
}
QTreeView::item:hover {
	background: %12;
}
QTreeView::item:selected {
	background: %13;
	color: palette(highlighted-text);
}
QTreeView QLineEdit {
	background: %14;
	border: 1px solid %2;
	border-radius: 2px;
	padding: 1px 3px;
	selection-background-color: palette(highlight);
}
QTreeView QLineEdit:focus {
	border: 1px solid %4;
}
QSplitter::handle {
	background: %6;
	border-left: 1px solid %7;
	border-right: 1px solid %7;
}
QSplitter::handle:hover {
	background: %12;
}
QToolTip {
	border: 1px solid %2;
	padding: 3px;
}
)")
			.arg(gradient(buttonTop, buttonBottom))                // 1
			.arg(name(border))                                     // 2
			.arg(gradient(hoverTop, hoverBottom))                  // 3
			.arg(name(accent))                                     // 4
			.arg(gradient(pressedTop, pressedBottom))              // 5
			.arg(name(window))                                     // 6
			.arg(name(softBorder))                                 // 7
			.arg(name(disabledText))                               // 8
			.arg(gradient(headerTop, headerBottom))                // 9
			.arg(name(stripe))                                     // 10
			.arg(name(rowLine))                                    // 11
			.arg(name(hover))                                      // 12
			.arg(gradient(selectedTop, selectedBottom))            // 13
			.arg(name(inputBackground));                           // 14

		app.setStyleSheet(sheet);
	}
}
