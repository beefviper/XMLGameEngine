// inspector.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include <QString>
#include <QTimer>
#include <QWidget>

#include <cstddef>
#include <functional>
#include <vector>

class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace xge
{
	class Game;
	class GameSession;

	// The right-hand side of the application: the play, pause and step
	// controls over a tree of everything in the game, laid out like the XML
	// (window, objects with their groups and grids, states). Each value that
	// can be changed has an editor on its row - a spin box for a number, a
	// check box for a yes/no, a text box for text - and a change takes effect
	// in the running game at once. Values that only describe the game (a
	// state's input, the game's size) are shown but not editable.
	class Inspector : public QWidget
	{
		Q_OBJECT

	public:
		explicit Inspector(GameSession& session, QWidget* parent = nullptr);

	private:
		// One row that can be edited: how to read it from the game, how to
		// write it back, and the editor made for it once its parent is opened.
		struct Field
		{
			enum class Kind { Float, Int, Bool, Text };

			Kind kind{ Kind::Float };
			std::function<double()> getNumber;
			std::function<void(double)> setNumber;
			std::function<QString()> getText;
			std::function<void(const QString&)> setText;
			double step{ 1 };
			int decimals{ 2 };
			QWidget* widget{ nullptr };
		};

		GameSession& session;
		QPushButton* playButton;
		QPushButton* stepButton;
		QPushButton* resetButton;
		QLabel* statusLabel;
		QTreeWidget* tree;
		QTimer refreshTimer;
		std::vector<Field> fields;

		Game& game() const;

		void rebuild();
		void unload();
		void updateControls();
		void refreshValues();

		// Editors are made when a row's parent is first opened: a game can
		// have thousands of values and most are never looked at.
		void ensureEditors(QTreeWidgetItem* parent);
		void makeEditor(QTreeWidgetItem* item, std::size_t index);

		// An edit was made: show it now if the game is not running.
		void edited();

		QTreeWidgetItem* addNode(QTreeWidgetItem* parent, const QString& name, const QString& summary = QString());
		void addInfo(QTreeWidgetItem* parent, const QString& name, const QString& value);
		void addFloat(QTreeWidgetItem* parent, const QString& name, std::function<double()> get, std::function<void(double)> set, double step = 1, int decimals = 2);
		void addInt(QTreeWidgetItem* parent, const QString& name, std::function<double()> get, std::function<void(double)> set);
		void addBool(QTreeWidgetItem* parent, const QString& name, std::function<double()> get, std::function<void(double)> set);
		void addText(QTreeWidgetItem* parent, const QString& name, std::function<QString()> get, std::function<void(const QString&)> set);

		void buildWindow();
		void buildObjects();
		void buildObject(QTreeWidgetItem* parent, std::size_t index);
		void buildSprite(QTreeWidgetItem* parent, std::size_t index);
		void buildStates();
	};
}
