// options_dialog.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "options_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <algorithm>

namespace xge
{
	OptionsDialog::OptionsDialog(const SessionOptions& current, bool warnBeforeTwoWindows, bool startGameOnLoad, QWidget* parent) :
		QDialog(parent),
		video(new QComboBox),
		xml(new QComboBox),
		warn(new QCheckBox(tr("&Ask before the game moves to a window of its own"))),
		start(new QCheckBox(tr("&Start Game on Load")))
	{
		setWindowTitle(tr("Options"));

		for (const VideoBackend backend : allVideoBackends())
		{
			video->addItem(videoBackendTitle(backend), static_cast<int>(backend));
		}
		video->setCurrentIndex(video->findData(static_cast<int>(current.video)));

		for (const XmlBackend backend : allXmlBackends())
		{
			xml->addItem(xmlBackendTitle(backend), static_cast<int>(backend));
		}
		xml->setCurrentIndex(xml->findData(static_cast<int>(current.xml)));

		auto* form = new QFormLayout;
		form->addRow(tr("&Video:"), video);
		form->addRow(tr("&XML parser:"), xml);

		warn->setChecked(warnBeforeTwoWindows);
		start->setChecked(startGameOnLoad);
		start->setToolTip(tr("Unchecked, a game opens paused so you can get ready, and starts when you press Play."));

		auto* note = new QLabel(tr(
			"The game waits while this dialog is open, and carries on when it closes.\n"
			"Every video library but the Qt renderer draws in a window of its own.\n"
			"Changing the video library keeps the game as it is.\n"
			"Changing the XML parser reads the game file again, so the game starts over."));
		note->setWordWrap(true);

		auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

		auto* layout = new QVBoxLayout(this);
		layout->addLayout(form);
		layout->addWidget(warn);
		layout->addWidget(start);
		layout->addWidget(note);
		layout->addWidget(buttons);

		// Sized up front: the wrapped note makes the layout's minimum depend on
		// the width, and a dialog first shown smaller than that minimum makes
		// Windows complain about the geometry Qt asked for.
		// Wide enough for the note's longest line, so it does not wrap.
		int widest = 0;
		for (const QString& line : note->text().split('\n'))
		{
			widest = std::max(widest, note->fontMetrics().horizontalAdvance(line));
		}
		setMinimumWidth(widest + layout->contentsMargins().left() + layout->contentsMargins().right() + 24);
		resize(sizeHint().expandedTo(minimumSize()));
	}

	bool OptionsDialog::warnBeforeTwoWindows() const
	{
		return warn->isChecked();
	}

	bool OptionsDialog::startGameOnLoad() const
	{
		return start->isChecked();
	}

	SessionOptions OptionsDialog::options() const
	{
		SessionOptions chosen;
		chosen.video = static_cast<VideoBackend>(video->currentData().toInt());
		chosen.xml = static_cast<XmlBackend>(xml->currentData().toInt());
		return chosen;
	}
}
