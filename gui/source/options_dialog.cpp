// options_dialog.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "options_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <algorithm>

namespace xge
{
	OptionsDialog::OptionsDialog(const SessionOptions& current, QWidget* parent) :
		QDialog(parent),
		video(new QComboBox),
		xml(new QComboBox)
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

		auto* note = new QLabel(tr(
			"Changing the video library keeps the game as it is: it stays paused, ready to play.\n"
			"Changing the XML parser reads the game file again, so the game starts over."));
		note->setWordWrap(true);

		auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

		auto* layout = new QVBoxLayout(this);
		layout->addLayout(form);
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

	SessionOptions OptionsDialog::options() const
	{
		SessionOptions chosen;
		chosen.video = static_cast<VideoBackend>(video->currentData().toInt());
		chosen.xml = static_cast<XmlBackend>(xml->currentData().toInt());
		return chosen;
	}
}
