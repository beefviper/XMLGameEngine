// options_dialog.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "session_options.h"

#include <QDialog>

class QCheckBox;
class QComboBox;

namespace xge
{
	// The Options window: which library draws the game, which one reads the
	// game file, and whether to be asked before the game moves to a window of
	// its own. Nothing changes until it is closed with OK.
	class OptionsDialog : public QDialog
	{
		Q_OBJECT

	public:
		OptionsDialog(const SessionOptions& current, bool warnBeforeTwoWindows, QWidget* parent = nullptr);

		// What the dropdowns say now.
		SessionOptions options() const;

		// Whether the box asking to be warned is checked.
		bool warnBeforeTwoWindows() const;

	private:
		QComboBox* video;
		QComboBox* xml;
		QCheckBox* warn;
	};
}
