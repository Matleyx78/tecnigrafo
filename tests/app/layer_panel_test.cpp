// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "layer_panel.hpp"

#include "action_registry.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "property_widgets.hpp"
#include "tg/edit/editor.hpp"
#include "tg/edit/layer_rules.hpp"

#include <QAbstractItemModel>
#include <QAction>
#include <QCoreApplication>
#include <QLineEdit>
#include <QModelIndex>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTableView>
#include <QToolButton>
#include <QVariant>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace tg::app {
namespace {

constexpr int kNameColumn = static_cast<int>(LayerColumn::Name);

/// The panel on a drawing of its own, with its own editor. The questions of
/// the panel are answered by the test, never by a box that waits.
class LayerPanelTest : public testing::Test {
protected:
    LayerPanelTest()
        : m_document(core::Document::newDrawing()), m_editor(m_document), m_panel(m_editor) {
        m_panel.setConfirm([this](const QString& text) {
            m_questions.push_back(text);
            return m_answer;
        });
    }

    [[nodiscard]] LayerPanel& panel() { return m_panel; }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] QTableView* table() const {
        return m_panel.findChild<QTableView*>(QStringLiteral("layerTable"));
    }

    [[nodiscard]] QAbstractItemModel* model() const { return table()->model(); }

    [[nodiscard]] QPushButton* button(const char* name) const {
        return m_panel.findChild<QPushButton*>(QString::fromLatin1(name));
    }

    /// A layer called `name`; fails the test if there is none.
    [[nodiscard]] const core::Layer* layer(const std::string& name) {
        const core::Layer* found = m_document.findLayer(name);
        EXPECT_NE(found, nullptr) << "no layer " << name;
        return found;
    }

    /// Adds a layer outside the panel, as a command would, and shows it.
    core::LayerId add(const std::string& name) {
        core::Layer fresh;
        fresh.name = name;
        fresh.linetype = m_document.continuousLinetype();
        m_editor.beginTransaction("Test");
        const auto id = m_editor.addLayer(fresh);
        m_editor.commit();
        m_panel.refresh();
        return id.value_or(core::LayerId{});
    }

    /// The text of the cell of `id` in `column`.
    [[nodiscard]] QString cellText(core::LayerId id, LayerColumn column) const {
        const int row = rowOf(id);
        if (row < 0) {
            return {};
        }
        return model()->index(row, static_cast<int>(column)).data().toString();
    }

    [[nodiscard]] int rowOf(core::LayerId id) const {
        for (int row = 0; row < model()->rowCount(); ++row) {
            if (model()->index(row, 0).data(LayerTableModel::kLayerIdRole).value<core::LayerId>() ==
                id) {
                return row;
            }
        }
        return -1;
    }

    [[nodiscard]] QWidget* cellWidget(core::LayerId id, LayerColumn column) const {
        return table()->indexWidget(model()->index(rowOf(id), static_cast<int>(column)));
    }

    /// Makes the change, undoes it and redoes it, checking `value` each time
    /// in the document and in the panel.
    template <typename Value>
    void changeUndoRedo(const std::function<bool()>& change, const std::function<Value()>& value) {
        const Value before = value();
        ASSERT_TRUE(change());
        const Value after = value();
        EXPECT_NE(before, after);
        EXPECT_EQ(m_editor.undoName(), "Layer");
        ASSERT_TRUE(m_editor.undo());
        m_panel.refresh();
        EXPECT_EQ(value(), before);
        ASSERT_TRUE(m_editor.redo());
        m_panel.refresh();
        EXPECT_EQ(value(), after);
    }

    /// What the stub of the questions answers.
    void answerWith(bool answer) { m_answer = answer; }

    [[nodiscard]] const std::vector<QString>& questions() const { return m_questions; }

private:
    bool m_answer{true};
    std::vector<QString> m_questions;
    core::Document m_document;
    edit::Editor m_editor;
    LayerPanel m_panel;
};

TEST_F(LayerPanelTest, ShowsEveryLayerWithItsColumns) {
    const auto walls = add("Walls");
    EXPECT_EQ(model()->rowCount(), static_cast<int>(document().layers().size()));
    EXPECT_EQ(model()->columnCount(), static_cast<int>(LayerColumn::Count));
    EXPECT_EQ(cellText(walls, LayerColumn::Name), QStringLiteral("Walls"));
    EXPECT_EQ(cellText(walls, LayerColumn::Lineweight), QStringLiteral("default"));
    EXPECT_NE(cellWidget(walls, LayerColumn::Color), nullptr);
    EXPECT_NE(cellWidget(walls, LayerColumn::Linetype), nullptr);
    EXPECT_NE(cellWidget(walls, LayerColumn::Lineweight), nullptr);
    // The current layer is marked, and only it.
    const auto zero = document().layerZero();
    EXPECT_TRUE(model()->index(rowOf(zero), 0).data(Qt::DecorationRole).isValid());
    EXPECT_FALSE(model()->index(rowOf(walls), 0).data(Qt::DecorationRole).isValid());
}

TEST_F(LayerPanelTest, NewLayerTakesTheFirstFreeNameAndIsSelected) {
    ASSERT_TRUE(panel().newLayer());
    const core::Layer* first = layer("Layer1");
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(panel().selectedLayer(), first->id);
    EXPECT_EQ(first->linetype, document().continuousLinetype());
    add("Layer3");
    ASSERT_TRUE(panel().newLayer());
    EXPECT_NE(document().findLayer("Layer2"), nullptr);
    // The first free name: Layer2 is taken now, Layer3 was already.
    ASSERT_TRUE(panel().newLayer());
    EXPECT_NE(document().findLayer("Layer4"), nullptr);
    EXPECT_EQ(editor().undoName(), "Layer");
}

TEST_F(LayerPanelTest, NewLayerUndoesAndRedoes) {
    const auto count = [this] {
        return static_cast<int>(document().layers().size());
    };
    changeUndoRedo<int>([this] { return panel().newLayer(); }, count);
    EXPECT_EQ(model()->rowCount(), count());
}

TEST_F(LayerPanelTest, RenameChangesTheNameAndUndoes) {
    const auto walls = add("Walls");
    const auto name = [&] {
        return document().layer(walls)->name;
    };
    changeUndoRedo<std::string>([&] { return panel().renameLayer(walls, QStringLiteral("Doors")); },
                                name);
    EXPECT_EQ(cellText(walls, LayerColumn::Name), QStringLiteral("Doors"));
    EXPECT_TRUE(questions().empty());
}

TEST_F(LayerPanelTest, EditingTheNameInTheTableRenames) {
    const auto walls = add("Walls");
    const QModelIndex index = model()->index(rowOf(walls), kNameColumn);
    EXPECT_TRUE(index.flags().testFlag(Qt::ItemIsEditable));
    EXPECT_FALSE(model()
                     ->index(rowOf(document().layerZero()), kNameColumn)
                     .flags()
                     .testFlag(Qt::ItemIsEditable));
    // The model only asks; the change waits for the event loop.
    model()->setData(index, QStringLiteral("Doors"), Qt::EditRole);
    QCoreApplication::sendPostedEvents(nullptr, 0);
    EXPECT_EQ(document().layer(walls)->name, "Doors");
    EXPECT_EQ(cellText(walls, LayerColumn::Name), QStringLiteral("Doors"));
}

TEST_F(LayerPanelTest, RefusedRenamesChangeNothingAndSayWhy) {
    const auto walls = add("Walls");
    add("Doors");
    const QSignalSpy spy(&panel(), &LayerPanel::messageShown);
    const std::size_t steps = editor().undoNames().size();

    EXPECT_FALSE(panel().renameLayer(document().layerZero(), QStringLiteral("Zero")));
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("Cannot rename layer \"0\"."));
    EXPECT_FALSE(panel().renameLayer(walls, QStringLiteral("doors")));
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("Layer \"doors\" already exists."));
    EXPECT_FALSE(panel().renameLayer(walls, QStringLiteral("bad/name")));
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("Invalid layer name."));
    EXPECT_FALSE(panel().renameLayer(walls, QStringLiteral("")));
    EXPECT_EQ(spy.size(), 4);

    EXPECT_EQ(document().layer(walls)->name, "Walls");
    EXPECT_EQ(editor().undoNames().size(), steps);
}

TEST_F(LayerPanelTest, RenameWarnsWhenOtherDataNamesTheLayer) {
    const auto walls = add("Walls");
    core::RawDrawing raw;
    raw.sections = {{"OBJECTS", {{1, "Walls"}}}};
    document().setRaw(raw);
    ASSERT_TRUE(edit::namedInOtherUnhandledData(document(), walls));

    answerWith(false);
    EXPECT_FALSE(panel().renameLayer(walls, QStringLiteral("Doors")));
    ASSERT_EQ(questions().size(), 1U);
    EXPECT_TRUE(questions()[0].contains(QStringLiteral("Walls")));
    EXPECT_TRUE(questions()[0].contains(QStringLiteral("keeps the old name")));
    EXPECT_EQ(document().layer(walls)->name, "Walls");

    answerWith(true);
    EXPECT_TRUE(panel().renameLayer(walls, QStringLiteral("Doors")));
    EXPECT_EQ(questions().size(), 2U);
    EXPECT_EQ(document().layer(walls)->name, "Doors");
    ASSERT_TRUE(editor().undo());
    panel().refresh();
    EXPECT_EQ(document().layer(walls)->name, "Walls");
}

TEST_F(LayerPanelTest, DeleteRemovesAnUnusedLayerAndUndoes) {
    const auto walls = add("Walls");
    add("Doors");
    const auto count = [this] {
        return static_cast<int>(document().layers().size());
    };
    changeUndoRedo<int>([&] { return panel().deleteLayer(walls); }, count);
    EXPECT_EQ(document().layer(walls), nullptr);
    EXPECT_EQ(rowOf(walls), -1);
}

TEST_F(LayerPanelTest, DeleteIsRefusedForLayerZeroTheCurrentAndUsedLayers) {
    const auto walls = add("Walls");
    const auto used = add("Used");
    editor().beginTransaction("Test");
    const auto line = editor().add(core::Line{{0, 0}, {1, 0}});
    auto properties = document().current();
    properties.layer = used;
    ASSERT_TRUE(editor().setProperties(line, properties));
    editor().commit();
    panel().refresh();
    ASSERT_TRUE(panel().makeCurrent(walls));

    const QSignalSpy spy(&panel(), &LayerPanel::messageShown);
    const std::size_t steps = editor().undoNames().size();
    EXPECT_FALSE(panel().deleteLayer(document().layerZero()));
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("Cannot delete layer \"0\"."));
    EXPECT_FALSE(panel().deleteLayer(walls));
    EXPECT_TRUE(panel().lastMessage().contains(QStringLiteral("CURRENT")));
    EXPECT_FALSE(panel().deleteLayer(used));
    EXPECT_TRUE(panel().lastMessage().contains(QStringLiteral("in use")));
    EXPECT_FALSE(panel().deleteLayer(core::LayerId{}));
    EXPECT_EQ(spy.size(), 4);
    EXPECT_EQ(editor().undoNames().size(), steps);
    EXPECT_NE(document().layer(walls), nullptr);
    EXPECT_NE(document().layer(used), nullptr);
}

TEST_F(LayerPanelTest, MakeCurrentChangesTheCurrentLayerAndUndoes) {
    const auto walls = add("Walls");
    const auto current = [this] {
        return document().current().layer.value;
    };
    changeUndoRedo<std::uint64_t>([&] { return panel().makeCurrent(walls); }, current);
    EXPECT_EQ(document().current().layer, walls);
    EXPECT_TRUE(model()->index(rowOf(walls), 0).data(Qt::DecorationRole).isValid());
    EXPECT_FALSE(
        model()->index(rowOf(document().layerZero()), 0).data(Qt::DecorationRole).isValid());
}

TEST_F(LayerPanelTest, AFrozenLayerCannotBecomeCurrentAndTheCurrentCannotFreeze) {
    const auto walls = add("Walls");
    ASSERT_TRUE(panel().setState(walls, LayerState::Frozen, true));
    EXPECT_FALSE(panel().makeCurrent(walls));
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("Cannot set current to a frozen layer."));
    EXPECT_EQ(document().current().layer, document().layerZero());

    EXPECT_FALSE(panel().setState(document().layerZero(), LayerState::Frozen, true));
    EXPECT_TRUE(panel().lastMessage().contains(QStringLiteral("CURRENT")));
    EXPECT_FALSE(document().layer(document().layerZero())->frozen);
}

TEST_F(LayerPanelTest, EveryStateChangesAndUndoes) {
    const auto walls = add("Walls");

    struct Case {
        LayerState state;
        bool initial;
    };

    for (const Case& item : {Case{LayerState::On, true}, Case{LayerState::Frozen, false},
                             Case{LayerState::Locked, false}, Case{LayerState::Plot, true}}) {
        SCOPED_TRACE(static_cast<int>(item.state));
        const auto value = [&] {
            return layerStateValue(*document().layer(walls), item.state);
        };
        ASSERT_EQ(value(), item.initial);
        changeUndoRedo<bool>([&] { return panel().setState(walls, item.state, !item.initial); },
                             value);
        // Back to the start for the next case.
        ASSERT_TRUE(panel().setState(walls, item.state, item.initial));
    }
}

TEST_F(LayerPanelTest, ClickingAStateCellTogglesIt) {
    const auto walls = add("Walls");
    const int row = rowOf(walls);
    const auto click = [&](LayerColumn column) {
        Q_EMIT table() -> clicked(model()->index(row, static_cast<int>(column)));
    };
    click(LayerColumn::Locked);
    EXPECT_TRUE(document().layer(walls)->locked);
    click(LayerColumn::Locked);
    EXPECT_FALSE(document().layer(walls)->locked);
    click(LayerColumn::Frozen);
    EXPECT_TRUE(document().layer(walls)->frozen);
    click(LayerColumn::On);
    EXPECT_FALSE(document().layer(walls)->on);
    click(LayerColumn::Plot);
    EXPECT_FALSE(document().layer(walls)->plot);
    // The icon follows the state.
    EXPECT_TRUE(model()
                    ->index(row, static_cast<int>(LayerColumn::Frozen))
                    .data(Qt::DecorationRole)
                    .isValid());
    click(LayerColumn::Status);
    EXPECT_NE(document().current().layer, walls) << "a frozen layer cannot become current";
}

TEST_F(LayerPanelTest, TurningTheCurrentLayerOffAsksFirst) {
    const auto zero = document().layerZero();
    answerWith(false);
    EXPECT_FALSE(panel().setState(zero, LayerState::On, false));
    ASSERT_EQ(questions().size(), 1U);
    EXPECT_TRUE(questions()[0].contains(QStringLiteral("CURRENT")));
    EXPECT_TRUE(document().layer(zero)->on);
    answerWith(true);
    EXPECT_TRUE(panel().setState(zero, LayerState::On, false));
    EXPECT_FALSE(document().layer(zero)->on);
    // Turning another layer off asks nothing.
    const auto walls = add("Walls");
    EXPECT_TRUE(panel().setState(walls, LayerState::On, false));
    EXPECT_EQ(questions().size(), 2U);
}

TEST_F(LayerPanelTest, ColorChangesAndUndoes) {
    const auto walls = add("Walls");
    const auto color = [&] {
        return core::formatColor(document().layer(walls)->color);
    };
    changeUndoRedo<std::string>([&] { return panel().setColor(walls, core::Color::fromIndex(1)); },
                                color);
    EXPECT_EQ(document().layer(walls)->color, core::Color::fromIndex(1));
    EXPECT_FALSE(panel().setColor(walls, core::Color::byLayer()));
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("Requires a color index from 1 to 255."));
    EXPECT_FALSE(panel().setColor(walls, core::Color::byBlock()));
    EXPECT_EQ(document().layer(walls)->color, core::Color::fromIndex(1));
}

TEST_F(LayerPanelTest, LinetypeChangesAndUndoes) {
    const auto walls = add("Walls");
    core::Linetype custom;
    custom.name = "Custom";
    custom.description = "Test pattern";
    custom.pattern.elements = {0.5, -0.25};
    editor().beginTransaction("Test");
    const auto type = editor().addLinetype(custom);
    editor().commit();
    ASSERT_TRUE(type.has_value());
    const core::LinetypeId customId = type.value_or(core::LinetypeId{});
    panel().refresh();

    const auto linetype = [&] {
        return document().layer(walls)->linetype.value;
    };
    changeUndoRedo<std::uint64_t>([&] { return panel().setLinetype(walls, customId); }, linetype);
    EXPECT_FALSE(panel().setLinetype(walls, document().byLayerLinetype()));
    EXPECT_FALSE(panel().setLinetype(walls, document().byBlockLinetype()));
    EXPECT_FALSE(panel().setLinetype(walls, core::LinetypeId{9999}));
    EXPECT_EQ(document().layer(walls)->linetype, customId);
}

TEST_F(LayerPanelTest, LineweightChangesAndUndoes) {
    const auto walls = add("Walls");
    const auto lineweight = [&] {
        return document().layer(walls)->lineweight;
    };
    changeUndoRedo<int>([&] { return panel().setLineweight(walls, 50); }, lineweight);
    EXPECT_EQ(cellText(walls, LayerColumn::Lineweight), QStringLiteral("0.50"));
    EXPECT_FALSE(panel().setLineweight(walls, core::lineweight::kByLayer));
    EXPECT_FALSE(panel().setLineweight(walls, core::lineweight::kByBlock));
    EXPECT_FALSE(panel().setLineweight(walls, 7));
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("Requires a valid lineweight."));
    EXPECT_EQ(document().layer(walls)->lineweight, 50);
}

TEST_F(LayerPanelTest, TheControlsOfTheCellsChangeTheLayerAndFollowIt) {
    const auto walls = add("Walls");
    auto* lineweight = qobject_cast<LineweightCombo*>(cellWidget(walls, LayerColumn::Lineweight));
    ASSERT_NE(lineweight, nullptr);
    const int index = lineweight->findData(QVariant::fromValue(50));
    ASSERT_GE(index, 0);
    lineweight->setCurrentIndex(index);
    ASSERT_TRUE(QMetaObject::invokeMethod(lineweight, "activated", Qt::DirectConnection,
                                          Q_ARG(int, index)));
    EXPECT_EQ(document().layer(walls)->lineweight, 50);
    EXPECT_EQ(editor().undoName(), "Layer");

    ASSERT_TRUE(editor().undo());
    panel().refresh();
    lineweight = qobject_cast<LineweightCombo*>(cellWidget(walls, LayerColumn::Lineweight));
    ASSERT_NE(lineweight, nullptr);
    EXPECT_EQ(lineweight->currentData().toInt(), core::lineweight::kDefault);
}

TEST_F(LayerPanelTest, ButtonsAndActionsWorkOnTheSelectedLayer) {
    QAction newAction;
    QAction deleteAction;
    QAction currentAction;
    panel().setActions(&newAction, &deleteAction, &currentAction);
    EXPECT_TRUE(newAction.isEnabled());
    EXPECT_FALSE(deleteAction.isEnabled());
    EXPECT_FALSE(currentAction.isEnabled());
    EXPECT_FALSE(button("layerDelete")->isEnabled());

    newAction.trigger();
    const core::Layer* first = layer("Layer1");
    ASSERT_NE(first, nullptr);
    const auto firstId = first->id;
    EXPECT_TRUE(deleteAction.isEnabled());
    EXPECT_TRUE(currentAction.isEnabled());
    EXPECT_TRUE(button("layerDelete")->isEnabled());

    currentAction.trigger();
    EXPECT_EQ(document().current().layer, firstId);
    // The current layer cannot be deleted; the button says why and keeps it.
    button("layerDelete")->click();
    EXPECT_NE(document().layer(firstId), nullptr);
    EXPECT_TRUE(panel().lastMessage().contains(QStringLiteral("CURRENT")));

    button("layerNew")->click();
    const core::Layer* second = layer("Layer2");
    ASSERT_NE(second, nullptr);
    const auto secondId = second->id;
    EXPECT_EQ(panel().selectedLayer(), secondId);
    deleteAction.trigger();
    EXPECT_EQ(document().layer(secondId), nullptr);
    panel().selectLayer(document().layerZero());
    button("layerMakeCurrent")->click();
    EXPECT_EQ(document().current().layer, document().layerZero());
}

TEST_F(LayerPanelTest, WhileACommandRunsThePanelChangesNothing) {
    const auto walls = add("Walls");
    QAction newAction;
    panel().setActions(&newAction, nullptr, nullptr);
    panel().setEditingEnabled(false);
    EXPECT_FALSE(panel().editingEnabled());
    EXPECT_FALSE(newAction.isEnabled());
    EXPECT_FALSE(button("layerNew")->isEnabled());
    EXPECT_FALSE(panel().newLayer());
    EXPECT_FALSE(panel().setState(walls, LayerState::Locked, true));
    EXPECT_FALSE(panel().renameLayer(walls, QStringLiteral("Doors")));
    EXPECT_EQ(document().layer(walls)->name, "Walls");
    EXPECT_FALSE(document().layer(walls)->locked);
    panel().setEditingEnabled(true);
    EXPECT_TRUE(panel().newLayer());
}

TEST_F(LayerPanelTest, FollowsChangesMadeElsewhere) {
    const auto walls = add("Walls");
    editor().beginTransaction("Elsewhere");
    ASSERT_TRUE(editor().removeLayer(walls));
    editor().commit();
    panel().refresh();
    EXPECT_EQ(rowOf(walls), -1);
    ASSERT_TRUE(editor().undo());
    panel().refresh();
    EXPECT_GE(rowOf(walls), 0);
    EXPECT_EQ(cellText(walls, LayerColumn::Name), QStringLiteral("Walls"));
}

TEST_F(LayerPanelTest, LayerControlsChooseTheCurrentLayerAndFollowThePanel) {
    const auto walls = add("Walls");
    const LayerControls controls(panel());
    auto* combo = controls.findChild<LayerCombo*>(QStringLiteral("layerCombo"));
    ASSERT_NE(combo, nullptr);
    EXPECT_EQ(combo->count(), static_cast<int>(document().layers().size()));
    EXPECT_EQ(combo->currentData().value<core::LayerId>(), document().layerZero());

    const int index = combo->findData(QVariant::fromValue(walls));
    ASSERT_GE(index, 0);
    combo->setCurrentIndex(index);
    ASSERT_TRUE(
        QMetaObject::invokeMethod(combo, "activated", Qt::DirectConnection, Q_ARG(int, index)));
    EXPECT_EQ(document().current().layer, walls);

    // A change made in the panel reaches the controls.
    ASSERT_TRUE(panel().renameLayer(walls, QStringLiteral("Doors")));
    EXPECT_EQ(combo->currentData().value<core::LayerId>(), walls);
    EXPECT_TRUE(combo->currentText().contains(QStringLiteral("Doors")));
    ASSERT_TRUE(panel().newLayer());
    EXPECT_EQ(combo->count(), static_cast<int>(document().layers().size()));

    // The buttons change the state of the current layer.
    auto* locked = controls.findChild<QToolButton*>(QStringLiteral("layerLockedButton"));
    ASSERT_NE(locked, nullptr);
    locked->click();
    EXPECT_TRUE(document().layer(walls)->locked);
    locked->click();
    EXPECT_FALSE(document().layer(walls)->locked);

    panel().setEditingEnabled(false);
    EXPECT_FALSE(controls.isEnabled());
    panel().setEditingEnabled(true);
    EXPECT_TRUE(controls.isEnabled());
}

/// The window, with the panel it builds.
class LayerWindowTest : public testing::Test {
protected:
    LayerWindowTest() : m_window(MainWindow::InitialDrawing::Empty) {
        m_window.setScriptMode();
        m_panel = m_window.findChild<LayerPanel*>(QStringLiteral("layerPanel"));
        m_panel->setConfirm([](const QString&) { return true; });
    }

    [[nodiscard]] LayerPanel* panel() const { return m_panel; }

    [[nodiscard]] QLineEdit* input() const {
        return m_window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }

    void enter(const std::string& text) {
        events::type(input(), text);
        events::press(input(), Qt::Key_Return);
    }

    [[nodiscard]] QStringList names() const {
        QStringList result;
        const QAbstractItemModel* model =
            panel()->findChild<QTableView*>(QStringLiteral("layerTable"))->model();
        for (int row = 0; row < model->rowCount(); ++row) {
            result.append(model->index(row, kNameColumn).data().toString());
        }
        return result;
    }

    [[nodiscard]] MainWindow& window() { return m_window; }

private:
    MainWindow m_window;
    LayerPanel* m_panel{nullptr};
};

// A window costs much more than the panel alone, so one test checks several
// things in a row.
TEST_F(LayerWindowTest, ThePanelFollowsTheWindowAndWaitsForCommands) {
    ASSERT_NE(panel(), nullptr);
    EXPECT_EQ(panel()->windowTitle(), QStringLiteral("Layers"));
    for (const char* id : {"layer.panel", "layer.new", "layer.delete", "layer.current"}) {
        EXPECT_NE(window().actionRegistry().action(QString::fromLatin1(id)), nullptr) << id;
    }
    auto* combo = window().findChild<LayerCombo*>(QStringLiteral("layerCombo"));
    ASSERT_NE(combo, nullptr);

    // It follows -LAYER typed in the command line, and so does the ribbon.
    EXPECT_EQ(names(), QStringList{QStringLiteral("0")});
    enter("-LAYER");
    enter("New");
    enter("Walls,Doors");
    enter("");
    EXPECT_TRUE(names().contains(QStringLiteral("Walls")));
    EXPECT_TRUE(names().contains(QStringLiteral("Doors")));
    EXPECT_EQ(names().size(), static_cast<int>(window().document().layers().size()));
    EXPECT_EQ(combo->count(), static_cast<int>(window().document().layers().size()));

    // A change of the panel is undone and redone by the actions of the window.
    ASSERT_TRUE(panel()->newLayer());
    EXPECT_TRUE(names().contains(QStringLiteral("Layer1")));
    QAction* undo = window().actionRegistry().action(QStringLiteral("edit.undo"));
    ASSERT_NE(undo, nullptr);
    EXPECT_TRUE(undo->isEnabled());
    EXPECT_TRUE(undo->text().contains(QStringLiteral("Layer")));
    undo->trigger();
    EXPECT_FALSE(names().contains(QStringLiteral("Layer1")));
    EXPECT_EQ(window().document().findLayer("Layer1"), nullptr);
    QAction* redo = window().actionRegistry().action(QStringLiteral("edit.redo"));
    ASSERT_NE(redo, nullptr);
    redo->trigger();
    EXPECT_TRUE(names().contains(QStringLiteral("Layer1")));

    // A refusal also goes to the history, as -LAYER writes it.
    const QSignalSpy spy(panel(), &LayerPanel::messageShown);
    EXPECT_FALSE(panel()->deleteLayer(window().document().layerZero()));
    ASSERT_EQ(spy.size(), 1);
    const QString text = spy.at(0).at(0).toString();
    EXPECT_EQ(text, QStringLiteral("Cannot delete layer \"0\"."));
    const auto* history =
        window().commandLine()->findChild<QPlainTextEdit*>(QStringLiteral("history"));
    ASSERT_NE(history, nullptr);
    EXPECT_TRUE(history->toPlainText().contains(text));

    // While a command runs the panel changes nothing.
    enter("LINE");
    ASSERT_TRUE(window().runner().isActive());
    EXPECT_FALSE(panel()->editingEnabled());
    EXPECT_FALSE(panel()->newLayer());
    EXPECT_FALSE(window().actionRegistry().action(QStringLiteral("layer.new"))->isEnabled());
    events::press(input(), Qt::Key_Escape);
    EXPECT_TRUE(panel()->editingEnabled());

    // LAYER, its alias and the action show the panel.
    window().show();
    for (const char* command : {"LAYER", "LA"}) {
        SCOPED_TRACE(command);
        panel()->hide();
        ASSERT_FALSE(panel()->isVisible());
        enter(command);
        EXPECT_TRUE(panel()->isVisible());
        EXPECT_FALSE(window().runner().isActive());
    }
    panel()->hide();
    window().actionRegistry().action(QStringLiteral("layer.panel"))->trigger();
    EXPECT_TRUE(panel()->isVisible());
}

}  // namespace
}  // namespace tg::app
