// options_dialog.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "session_options.h"

#include <QDialog>

class QComboBox;

namespace xge
{
	// The Options window: which library draws the game and which one reads the
	// game file. Nothing changes until it is closed with OK.
	class OptionsDialog : public QDialog
	{
		Q_OBJECT

	public:
		OptionsDialog(const SessionOptions& current, QWidget* parent = nullptr);

		// What the dropdowns say now.
		SessionOptions options() const;

	private:
		QComboBox* video;
		QComboBox* xml;
	};
}
