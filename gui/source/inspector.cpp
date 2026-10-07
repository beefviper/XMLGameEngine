// inspector.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "inspector.h"

#include "game.h"
#include "game_session.h"

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <map>
#include <utility>
#include <sstream>
#include <string>

namespace xge
{
	namespace
	{
		// Item data: the index of the row's Field, and whether the editors of
		// a parent's rows have been made.
		constexpr int kFieldRole = Qt::UserRole;
		constexpr int kBuiltRole = Qt::UserRole + 1;

		QString text(const std::string& value)
		{
			return QString::fromStdString(value);
		}

		QString numberText(double value)
		{
			return QString::number(value, 'g', 9);
		}

		std::string shapeName(ShapeKind kind)
		{
			switch (kind)
			{
			case ShapeKind::Circle: return "circle";
			case ShapeKind::Rectangle: return "rectangle";
			case ShapeKind::Text: return "text";
			case ShapeKind::Image: return "image";
			case ShapeKind::Line: return "drawn";
			case ShapeKind::Unknown: break;
			}
			return "unknown";
		}

		QString commandsText(const std::vector<Command>& commands)
		{
			std::ostringstream out;
			for (std::size_t i = 0; i < commands.size(); ++i)
			{
				out << commands[i] << (i + 1 < commands.size() ? "; " : "");
			}
			return text(out.str());
		}
	}

	Inspector::Inspector(GameSession& gameSession, QWidget* parent) :
		QWidget(parent),
		session(gameSession),
		playButton(new QPushButton(tr("Pause"), this)),
		stepButton(new QPushButton(tr("Step"), this)),
		resetButton(new QPushButton(tr("Reset"), this)),
		statusLabel(new QLabel(this)),
		tree(new QTreeWidget(this))
	{
		statusLabel->setTextFormat(Qt::RichText);

		// Clicking a button must not take the keyboard away from the game.
		for (QPushButton* button : { playButton, stepButton, resetButton })
		{
			button->setFocusPolicy(Qt::NoFocus);
		}

		playButton->setToolTip(tr("Run or freeze the simulation itself, not the game's own pause screen"));
		stepButton->setToolTip(tr("Advance the frozen simulation by one frame"));
		resetButton->setToolTip(tr("Put every object back as the game loaded"));

		auto* controls = new QHBoxLayout;
		controls->addWidget(playButton);
		controls->addWidget(stepButton);
		controls->addWidget(resetButton);
		controls->addStretch();

		tree->setColumnCount(2);
		tree->setHeaderLabels({ tr("Name"), tr("Value") });
		tree->setUniformRowHeights(true);

		// Every other row is tinted (the color comes from the theme, theme.h),
		// so a name can be followed across to its value.
		tree->setAlternatingRowColors(true);
		tree->setIndentation(18);
		tree->header()->setStretchLastSection(true);
		tree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
		tree->setColumnWidth(0, 200);

		auto* layout = new QVBoxLayout(this);
		layout->setContentsMargins(6, 6, 6, 6);
		layout->addLayout(controls);
		layout->addWidget(statusLabel);
		layout->addWidget(tree, 1);

		connect(playButton, &QPushButton::clicked, &session, &GameSession::togglePlay);
		connect(stepButton, &QPushButton::clicked, &session, &GameSession::step);
		connect(resetButton, &QPushButton::clicked, &session, &GameSession::reset);
		connect(&session, &GameSession::playingChanged, this, &Inspector::updateControls);
		connect(&session, &GameSession::aboutToUnload, this, &Inspector::unload);
		connect(&session, &GameSession::loaded, this, &Inspector::rebuild);
		connect(tree, &QTreeWidget::itemExpanded, this, &Inspector::ensureEditors);

		// The values change every frame while the game runs; look again ten
		// times a second rather than on every frame.
		refreshTimer.setInterval(100);
		connect(&refreshTimer, &QTimer::timeout, this, &Inspector::refreshValues);
		refreshTimer.start();

		updateControls();
	}

	Game& Inspector::game() const
	{
		return *session.currentGame();
	}

	void Inspector::unload()
	{
		// The rows read the game through their getters: they go before it does.
		tree->clear();
		fields.clear();
		updateControls();
	}

	void Inspector::updateControls()
	{
		const bool loaded = session.isLoaded();
		playButton->setEnabled(loaded);
		resetButton->setEnabled(loaded);
		stepButton->setEnabled(loaded && !session.isPlaying());
		playButton->setText(session.isPlaying() ? tr("Pause") : tr("Play"));
	}

	void Inspector::refreshValues()
	{
		if (!session.isLoaded())
		{
			statusLabel->setText(tr("No game loaded").toHtmlEscaped());
			return;
		}

		QString frame = QString::number(session.frames());
		if (session.isPlaying() && session.fps() > 0)
		{
			frame += tr(" (%1fps)").arg(session.fps(), 0, 'f', 1);
		}

		// Playing in green and Paused in red, so which it is can be seen at a
		// glance; both read on a light or a dark theme.
		const QString mode = session.isPlaying()
			? QStringLiteral("<b style=\"color:#2fa84f\">%1</b>").arg(tr("Playing").toHtmlEscaped())
			: QStringLiteral("<b style=\"color:#e5483f\">%1</b>").arg(tr("Paused").toHtmlEscaped());

		statusLabel->setText(tr("State: %1   Frame: %2   %3")
			.arg(text(game().getCurrentState().name).toHtmlEscaped())
			.arg(frame.toHtmlEscaped())
			.arg(mode));

		for (auto& field : fields)
		{
			// Only rows that are on screen, and never the one being typed in.
			if (!field.widget || !field.widget->isVisible() || field.widget->hasFocus())
			{
				continue;
			}

			const QSignalBlocker blocker(field.widget);

			switch (field.kind)
			{
			case Field::Kind::Float:
				static_cast<QDoubleSpinBox*>(field.widget)->setValue(field.getNumber());
				break;
			case Field::Kind::Int:
				static_cast<QSpinBox*>(field.widget)->setValue(static_cast<int>(field.getNumber()));
				break;
			case Field::Kind::Bool:
				static_cast<QCheckBox*>(field.widget)->setChecked(field.getNumber() != 0);
				break;
			case Field::Kind::Text:
				static_cast<QLineEdit*>(field.widget)->setText(field.getText());
				break;
			}
		}
	}

	void Inspector::rebuild()
	{
		tree->clear();
		fields.clear();
		updateControls();

		if (!session.isLoaded())
		{
			return;
		}

		buildWindow();
		buildObjects();
		buildStates();

		// Window and Objects open; their rows get editors as they do.
		tree->expandItem(tree->topLevelItem(0));
		tree->expandItem(tree->topLevelItem(1));
		refreshValues();
	}

	void Inspector::edited()
	{
		if (!session.isPlaying())
		{
			session.redraw();
		}
	}

	void Inspector::ensureEditors(QTreeWidgetItem* parent)
	{
		if (parent->data(0, kBuiltRole).toBool())
		{
			return;
		}
		parent->setData(0, kBuiltRole, true);

		for (int i = 0; i < parent->childCount(); ++i)
		{
			QTreeWidgetItem* child = parent->child(i);
			const QVariant index = child->data(0, kFieldRole);
			if (index.isValid())
			{
				makeEditor(child, static_cast<std::size_t>(index.toInt()));
			}
		}
	}

	void Inspector::makeEditor(QTreeWidgetItem* item, std::size_t index)
	{
		Field& field = fields[index];
		QWidget* editor = nullptr;

		switch (field.kind)
		{
		case Field::Kind::Float:
		{
			auto* spin = new QDoubleSpinBox;
			spin->setRange(-1e9, 1e9);
			spin->setDecimals(field.decimals);
			spin->setSingleStep(field.step);
			spin->setValue(field.getNumber());
			connect(spin, &QDoubleSpinBox::valueChanged, this, [this, index](double value)
				{
					fields[index].setNumber(value);
					edited();
				});
			editor = spin;
			break;
		}
		case Field::Kind::Int:
		{
			auto* spin = new QSpinBox;
			spin->setRange(-1000000000, 1000000000);
			spin->setValue(static_cast<int>(field.getNumber()));
			connect(spin, &QSpinBox::valueChanged, this, [this, index](int value)
				{
					fields[index].setNumber(value);
					edited();
				});
			editor = spin;
			break;
		}
		case Field::Kind::Bool:
		{
			auto* box = new QCheckBox;
			box->setChecked(field.getNumber() != 0);
			connect(box, &QCheckBox::toggled, this, [this, index](bool on)
				{
					fields[index].setNumber(on ? 1 : 0);
					edited();
				});
			editor = box;
			break;
		}
		case Field::Kind::Text:
		{
			auto* line = new QLineEdit;
			line->setText(field.getText());
			connect(line, &QLineEdit::editingFinished, this, [this, index, line]()
				{
					// Qt reports a finished edit whenever the box loses the
					// focus, typed in or not (switching the video library moves
					// the focus, for one): only a change is an edit.
					if (line->text() == fields[index].getText())
					{
						return;
					}

					fields[index].setText(line->text());
					edited();
				});
			editor = line;
			break;
		}
		}

		// The editor's own number column would otherwise run the whole width
		// of the panel; the value reads better against the left edge.
		editor->setFocusPolicy(Qt::StrongFocus);
		tree->setItemWidget(item, 1, editor);
		field.widget = editor;
	}

	QTreeWidgetItem* Inspector::addNode(QTreeWidgetItem* parent, const QString& name, const QString& summary)
	{
		auto* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree);
		item->setText(0, name);
		item->setText(1, summary);

		// The sections (Window, Objects, States) in bold, so where one ends
		// and the next begins can be seen at a glance.
		if (!parent)
		{
			QFont bold = item->font(0);
			bold.setBold(true);
			item->setFont(0, bold);
			item->setFont(1, bold);
		}

		return item;
	}

	void Inspector::addInfo(QTreeWidgetItem* parent, const QString& name, const QString& value)
	{
		auto* item = addNode(parent, name, value);
		item->setToolTip(1, value);
	}

	void Inspector::addFloat(QTreeWidgetItem* parent, const QString& name, std::function<double()> get, std::function<void(double)> set, double step, int decimals)
	{
		Field field;
		field.kind = Field::Kind::Float;
		field.getNumber = std::move(get);
		field.setNumber = std::move(set);
		field.step = step;
		field.decimals = decimals;
		fields.push_back(std::move(field));
		addNode(parent, name)->setData(0, kFieldRole, static_cast<int>(fields.size() - 1));
	}

	void Inspector::addInt(QTreeWidgetItem* parent, const QString& name, std::function<double()> get, std::function<void(double)> set)
	{
		Field field;
		field.kind = Field::Kind::Int;
		field.getNumber = std::move(get);
		field.setNumber = std::move(set);
		fields.push_back(std::move(field));
		addNode(parent, name)->setData(0, kFieldRole, static_cast<int>(fields.size() - 1));
	}

	void Inspector::addBool(QTreeWidgetItem* parent, const QString& name, std::function<double()> get, std::function<void(double)> set)
	{
		Field field;
		field.kind = Field::Kind::Bool;
		field.getNumber = std::move(get);
		field.setNumber = std::move(set);
		fields.push_back(std::move(field));
		addNode(parent, name)->setData(0, kFieldRole, static_cast<int>(fields.size() - 1));
	}

	void Inspector::addText(QTreeWidgetItem* parent, const QString& name, std::function<QString()> get, std::function<void(const QString&)> set)
	{
		Field field;
		field.kind = Field::Kind::Text;
		field.getText = std::move(get);
		field.setText = std::move(set);
		fields.push_back(std::move(field));
		addNode(parent, name)->setData(0, kFieldRole, static_cast<int>(fields.size() - 1));
	}

	void Inspector::buildWindow()
	{
		const WindowDesc& desc = game().getWindowDesc();

		auto* node = addNode(nullptr, tr("Window"), text(desc.name));
		addInfo(node, tr("size"), QString("%1 x %2").arg(desc.width).arg(desc.height));
		addText(node, tr("background"),
			[this]() { return text(game().getWindowDesc().background); },
			[this](const QString& value) { game().getWindowDesc().background = value.toStdString(); });
		addInfo(node, tr("fullscreen"), text(desc.fullscreen));
		addInfo(node, tr("framerate"), QString::number(desc.framerate));
	}

	void Inspector::buildObjects()
	{
		auto& objects = game().getCurrentObjects();
		auto* root = addNode(nullptr, tr("Objects"), tr("%1 objects").arg(objects.size()));

		// A <group> is written once in the file and read as one object per
		// member; show it the way it was written.
		std::map<std::string, QTreeWidgetItem*> containers;
		std::map<QTreeWidgetItem*, int> members;

		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			const Object& object = objects[i];
			QTreeWidgetItem* parent = root;

			if (!object.groupName.empty())
			{
				auto found = containers.find(object.groupName);
				if (found == containers.end())
				{
					found = containers.emplace(object.groupName, addNode(root, text(object.groupName))).first;
					found->second->setToolTip(0, tr("group"));
				}
				parent = found->second;
				++members[parent];
			}

			buildObject(parent, i);
		}

		for (const auto& [item, count] : members)
		{
			item->setText(1, tr("%1 members").arg(count));
		}
	}

	void Inspector::buildObject(QTreeWidgetItem* parent, std::size_t i)
	{
		auto object = [this, i]() -> Object& { return game().getCurrentObjects()[i]; };
		const Object& first = object();

		QString summary = text(shapeName(first.shapeKind));
		if (!first.objClass.empty())
		{
			summary += "  [" + text(first.objClass) + "]";
		}

		auto* node = addNode(parent, text(first.name), summary);

		addText(node, tr("class"),
			[object]() { return text(object().objClass); },
			[object](const QString& value) { object().objClass = value.toStdString(); });
		addBool(node, tr("visible"),
			[object]() { return object().isVisible ? 1.0 : 0.0; },
			[object](double value) { object().isVisible = value != 0; });

		buildSprite(node, i);

		auto* position = addNode(node, tr("position"));
		addFloat(position, "x", [object]() { return static_cast<double>(object().position.x); }, [object](double v) { object().position.x = static_cast<float>(v); });
		addFloat(position, "y", [object]() { return static_cast<double>(object().position.y); }, [object](double v) { object().position.y = static_cast<float>(v); });

		auto* velocity = addNode(node, tr("velocity"));
		addFloat(velocity, "x", [object]() { return static_cast<double>(object().velocity.x); }, [object](double v) { object().velocity.x = static_cast<float>(v); }, 0.1, 3);
		addFloat(velocity, "y", [object]() { return static_cast<double>(object().velocity.y); }, [object](double v) { object().velocity.y = static_cast<float>(v); }, 0.1, 3);

		auto* acceleration = addNode(node, tr("acceleration"));
		addFloat(acceleration, "x", [object]() { return static_cast<double>(object().acceleration.x); }, [object](double v) { object().acceleration.x = static_cast<float>(v); }, 0.01, 4);
		addFloat(acceleration, "y", [object]() { return static_cast<double>(object().acceleration.y); }, [object](double v) { object().acceleration.y = static_cast<float>(v); }, 0.01, 4);

		auto* collision = addNode(node, tr("collision"), first.collisionData.enabled ? tr("on") : tr("off"));
		addBool(collision, tr("enabled"),
			[object]() { return object().collisionData.enabled ? 1.0 : 0.0; },
			[object](double value) { object().collisionData.enabled = value != 0; });
		addInfo(collision, tr("type"), first.collisionData.type == CollisionType::Pixel ? tr("pixel") : tr("box"));
		addInfo(collision, tr("rules about other objects"), QString::number(first.collisionData.basic.size()));
		addInfo(collision, tr("top"), commandsText(first.collisionData.top));
		addInfo(collision, tr("bottom"), commandsText(first.collisionData.bottom));
		addInfo(collision, tr("left"), commandsText(first.collisionData.left));
		addInfo(collision, tr("right"), commandsText(first.collisionData.right));

		if (!first.variable.empty())
		{
			auto* variables = addNode(node, tr("variables"), tr("%1").arg(first.variable.size()));
			for (const auto& entry : first.variable)
			{
				const std::string variableName = entry.first;
				addFloat(variables, text(variableName),
					[object, variableName]() { return static_cast<double>(object().variable[variableName]); },
					[this, object, variableName](double v) { game().setVariable(object().name, variableName, static_cast<float>(v)); },
					1, 2);
			}
		}

		if (!first.action.empty())
		{
			auto* actions = addNode(node, tr("actions"), tr("%1").arg(first.action.size()));
			for (const auto& entry : first.action)
			{
				addInfo(actions, text(entry.first), commandsText(entry.second));
			}
		}

		if (!first.boundVariableOwner.empty())
		{
			addInfo(node, tr("shows"), text(first.boundVariableOwner + "." + first.boundVariableName));
		}
	}

	void Inspector::buildSprite(QTreeWidgetItem* parent, std::size_t i)
	{
		auto object = [this, i]() -> Object& { return game().getCurrentObjects()[i]; };
		const Object& first = object();
		const auto& params = first.spriteParams;

		auto* sprite = addNode(parent, tr("sprite"), text(shapeName(first.shapeKind)));

		// A sprite parameter that is a number, and one that is text; either
		// way the object's picture is built again from them (visualDirty).
		auto addParamNumber = [&](const QString& name, std::size_t at, bool whole)
		{
			if (at >= params.size())
			{
				return;
			}

			auto get = [object, at]() { return std::stod(object().spriteParams.at(at)); };
			auto set = [object, at](double v)
			{
				object().spriteParams.at(at) = numberText(v).toStdString();
				object().visualDirty = true;
			};

			if (whole)
			{
				addInt(sprite, name, get, set);
			}
			else
			{
				addFloat(sprite, name, get, set, 1, 1);
			}
		};

		auto addParamText = [&](const QString& name, std::size_t at)
		{
			if (at >= params.size())
			{
				return;
			}

			addText(sprite, name,
				[object, at]() { return text(object().spriteParams.at(at)); },
				[object, at](const QString& value)
				{
					object().spriteParams.at(at) = value.toStdString();
					object().visualDirty = true;
				});
		};

		switch (first.shapeKind)
		{
		case ShapeKind::Circle:
			addParamNumber(tr("radius"), 1, false);
			addParamText(tr("color"), 3);
			break;
		case ShapeKind::Rectangle:
			addParamNumber(tr("width"), 1, false);
			addParamNumber(tr("height"), 2, false);
			addParamText(tr("color"), 3);
			break;
		case ShapeKind::Text:
			addParamText(tr("content"), 1);
			addParamNumber(tr("size"), 2, true);
			addParamText(tr("color"), 3);
			break;
		case ShapeKind::Image:
			if (params.size() > 1) { addInfo(sprite, tr("file"), text(params[1])); }
			if (params.size() > 2) { addInfo(sprite, tr("flip"), text(params[2])); }
			break;
		case ShapeKind::Line:
			addInfo(sprite, tr("drawn from"), tr("pixels"));
			break;
		case ShapeKind::Unknown:
			break;
		}

		addInfo(sprite, tr("size on screen"), QString("%1 x %2").arg(first.size.x).arg(first.size.y));
	}

	void Inspector::buildStates()
	{
		const auto& states = game().getStates();
		auto* root = addNode(nullptr, tr("States"), tr("%1 states").arg(states.size()));

		for (const State& state : states)
		{
			QString shown;
			for (const auto& name : state.show)
			{
				shown += (shown.isEmpty() ? "" : ", ") + text(name);
			}

			auto* node = addNode(root, text(state.name), tr("shows %1").arg(shown.isEmpty() ? tr("nothing") : shown));

			for (const auto& [key, commands] : state.input)
			{
				addInfo(node, tr("key %1").arg(text(keyCodeToString(key))), commandsText(commands));
			}

			int number = 0;
			for (const Condition& condition : state.conditions)
			{
				std::ostringstream when;
				if (condition.remaining)
				{
					when << "remaining <= " << *condition.remaining;
				}
				else if (condition.atMost)
				{
					when << condition.variableName << " <= " << *condition.atMost;
				}
				else
				{
					when << condition.variableName << " >= " << condition.value;
				}

				if (!condition.filterClass.empty()) { when << ", class " << condition.filterClass; }
				if (!condition.filterObject.empty()) { when << ", object " << condition.filterObject; }
				when << ": ";

				addInfo(node, tr("condition %1").arg(++number), text(when.str()) + commandsText(condition.commands));
			}
		}
	}
}
