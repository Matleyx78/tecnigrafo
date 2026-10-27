// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "properties_panel.hpp"

#include "action_registry.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "property_widgets.hpp"
#include "quick_select_dialog.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/quick_select.hpp"
#include "tg/edit/editor.hpp"

#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QGroupBox>
#include <QLineEdit>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::app {
namespace {

using core::EntityId;

const core::Color kRed = core::Color::fromIndex(1);
const core::Color kBlue = core::Color::fromIndex(5);

/// A user's choice in a drop-down list: the setters of the controls emit
/// nothing, only activation does.
void activate(QComboBox& combo, int index) {
    ASSERT_GE(index, 0);
    combo.setCurrentIndex(index);
    ASSERT_TRUE(
        QMetaObject::invokeMethod(&combo, "activated", Qt::DirectConnection, Q_ARG(int, index)));
}

/// Chooses the item of `combo` whose data is `value`.
template <typename Value>
void choose(QComboBox& combo, Value value) {
    activate(combo, combo.findData(QVariant::fromValue(value)));
}

/// The panel on a drawing of its own, with its own editor and runner.
class PropertiesPanelTest : public testing::Test {
protected:
    PropertiesPanelTest()
        : m_document(core::Document::newDrawing()),
          m_editor(m_document),
          m_runner(m_registry, m_editor),
          m_panel(m_editor, m_runner) {}

    [[nodiscard]] PropertiesPanel& panel() { return m_panel; }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] cmd::CommandRunner& runner() { return m_runner; }

    template <typename Widget>
    [[nodiscard]] Widget* child(const char* name) const {
        return m_panel.findChild<Widget*>(QString::fromLatin1(name));
    }

    EntityId add(core::Entity entity) {
        m_editor.beginTransaction("Test");
        const EntityId id = m_editor.add(std::move(entity));
        m_editor.commit();
        return id;
    }

    EntityId addLine(double x) { return add(core::Line{{x, 0.0}, {x + 10.0, 5.0}}); }

    EntityId addCircle(double x) { return add(core::Circle{{x, 0.0}, 2.0}); }

    core::LayerId addLayer(const std::string& name, bool locked = false, bool frozen = false) {
        core::Layer layer;
        layer.name = name;
        layer.linetype = m_document.continuousLinetype();
        layer.locked = locked;
        layer.frozen = frozen;
        m_editor.beginTransaction("Test");
        const auto id = m_editor.addLayer(layer);
        m_editor.commit();
        EXPECT_TRUE(id.has_value());
        return id.value_or(core::LayerId{});
    }

    void setProperties(EntityId id, const core::EntityProperties& properties) {
        m_editor.beginTransaction("Test");
        EXPECT_TRUE(m_editor.setProperties(id, properties));
        m_editor.commit();
    }

    [[nodiscard]] const core::EntityProperties& propertiesOf(EntityId id) {
        const core::EntityRecord* record = m_document.find(id);
        EXPECT_NE(record, nullptr);
        static const core::EntityProperties kNone;
        return record != nullptr ? record->properties : kNone;
    }

    /// Makes `ids` the selection, as a click would, and shows it.
    void select(const std::vector<EntityId>& ids) {
        ASSERT_TRUE(m_runner.replaceSelection(ids));
        m_panel.refresh();
    }

    /// Undoes the last change, which must be one of the panel.
    void undoProperties() {
        EXPECT_EQ(m_editor.undoName(), "Properties");
        ASSERT_TRUE(m_editor.undo());
        m_panel.refresh();
    }

private:
    core::Document m_document;
    edit::Editor m_editor;
    cmd::CommandRegistry m_registry;
    cmd::CommandRunner m_runner;
    PropertiesPanel m_panel;
};

TEST_F(PropertiesPanelTest, WithoutSelectionShowsAndChangesTheCurrentValues) {
    EXPECT_EQ(panel().objectName(), QStringLiteral("propertiesPanel"));
    EXPECT_EQ(panel().windowTitle(), QStringLiteral("Properties"));
    EXPECT_EQ(panel().typeLabels(), QStringList{QStringLiteral("No selection")});
    EXPECT_TRUE(panel().geometry().isEmpty());
    const core::CurrentProperties start = document().current();
    EXPECT_EQ(panel().shown().color, start.color);
    EXPECT_EQ(panel().shown().layer, start.layer);
    EXPECT_EQ(panel().shown().linetype, start.linetype);
    EXPECT_EQ(panel().shown().lineweight, start.lineweight);
    EXPECT_EQ(panel().shown().linetypeScale, start.linetypeScale);

    // The control shows the current colour, and choosing one changes it.
    auto* color = child<ColorCombo>("propertiesColor");
    ASSERT_NE(color, nullptr);
    choose(*color, kRed);
    EXPECT_EQ(document().current().color, kRed);
    EXPECT_EQ(panel().shown().color, kRed);
    undoProperties();
    EXPECT_EQ(document().current().color, start.color);

    const core::LayerId walls = addLayer("Walls");
    panel().refresh();
    ASSERT_TRUE(panel().setLayer(walls));
    EXPECT_EQ(document().current().layer, walls);
    undoProperties();
    EXPECT_EQ(document().current().layer, start.layer);

    ASSERT_TRUE(panel().setLinetype(document().continuousLinetype()));
    EXPECT_EQ(document().current().linetype, document().continuousLinetype());
    ASSERT_TRUE(panel().setLinetypeScale(2.0));
    EXPECT_EQ(document().current().linetypeScale, 2.0);
    ASSERT_TRUE(panel().setLineweight(50));
    EXPECT_EQ(document().current().lineweight, 50);
    undoProperties();
    undoProperties();
    undoProperties();
    EXPECT_EQ(document().current(), start);

    // A frozen layer cannot become current: said, and nothing changes.
    const core::LayerId frozen = addLayer("Frozen", false, true);
    panel().refresh();
    const QSignalSpy messages(&panel(), &PropertiesPanel::messageShown);
    EXPECT_FALSE(panel().setLayer(frozen));
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("Cannot set current to a frozen layer."));
    EXPECT_EQ(messages.size(), 1);
    EXPECT_EQ(document().current().layer, start.layer);
}

TEST_F(PropertiesPanelTest, ASingleEntityShowsItsTypeValuesAndGeometry) {
    const EntityId line = addLine(0.0);
    select({line});
    EXPECT_EQ(panel().typeLabels(), QStringList{QStringLiteral("Line")});
    EXPECT_EQ(panel().targets(), std::vector<EntityId>{line});
    EXPECT_EQ(panel().shown().color, core::Color::byLayer());
    EXPECT_EQ(panel().shown().layer, document().layerZero());
    EXPECT_EQ(panel().shown().linetypeScale, 1.0);
    EXPECT_EQ(child<QLineEdit>("propertiesLinetypeScale")->text(), QStringLiteral("1.0000"));

    const QList<QPair<QString, QString>> expected{
        {QStringLiteral("Start X"), QStringLiteral("0.0000")},
        {QStringLiteral("Start Y"), QStringLiteral("0.0000")},
        {QStringLiteral("End X"), QStringLiteral("10.0000")},
        {QStringLiteral("End Y"), QStringLiteral("5.0000")},
        {QStringLiteral("Length"), QStringLiteral("11.1803")}};
    EXPECT_EQ(panel().geometry(), expected);
    EXPECT_TRUE(child<QGroupBox>("propertiesGeometry")->isVisibleTo(&panel()));

    const EntityId circle = addCircle(20.0);
    select({circle});
    EXPECT_EQ(panel().typeLabels(), QStringList{QStringLiteral("Circle")});
    const QList<QPair<QString, QString>> circleRows{
        {QStringLiteral("Center X"), QStringLiteral("20.0000")},
        {QStringLiteral("Center Y"), QStringLiteral("0.0000")},
        {QStringLiteral("Radius"), QStringLiteral("2.0000")}};
    EXPECT_EQ(panel().geometry(), circleRows);

    // Two entities: no geometry.
    select({line, circle});
    EXPECT_TRUE(panel().geometry().isEmpty());
    EXPECT_FALSE(child<QGroupBox>("propertiesGeometry")->isVisibleTo(&panel()));
}

TEST_F(PropertiesPanelTest, AMixedSelectionShowsVaries) {
    const EntityId line = addLine(0.0);
    const EntityId circle = addCircle(20.0);
    core::EntityProperties red = propertiesOf(line);
    red.color = kRed;
    red.linetypeScale = 2.0;
    setProperties(line, red);
    core::EntityProperties blue = propertiesOf(circle);
    blue.color = kBlue;
    setProperties(circle, blue);

    select({line, circle});
    EXPECT_EQ(panel().typeLabels(), (QStringList{QStringLiteral("All (2)"), QStringLiteral("Line"),
                                                 QStringLiteral("Circle")}));
    EXPECT_FALSE(panel().shown().color.has_value());
    EXPECT_FALSE(panel().shown().linetypeScale.has_value());
    // The shared ones are shown.
    EXPECT_EQ(panel().shown().layer, document().layerZero());
    EXPECT_EQ(panel().shown().lineweight, core::lineweight::kByLayer);
    // The P21 controls show their "Varies" item; the scale shows *VARIES*.
    EXPECT_FALSE(child<ColorCombo>("propertiesColor")->currentData().isValid());
    EXPECT_TRUE(child<LayerCombo>("propertiesLayer")->currentData().isValid());
    EXPECT_EQ(child<QLineEdit>("propertiesLinetypeScale")->text(), QStringLiteral("*VARIES*"));

    // One colour for both: no longer varies.
    ASSERT_TRUE(panel().setColor(kRed));
    EXPECT_EQ(propertiesOf(line).color, kRed);
    EXPECT_EQ(propertiesOf(circle).color, kRed);
    EXPECT_EQ(panel().shown().color, kRed);
    undoProperties();
    EXPECT_EQ(propertiesOf(circle).color, kBlue);
    EXPECT_FALSE(panel().shown().color.has_value());
}

TEST_F(PropertiesPanelTest, EveryPropertyChangesAndUndoes) {
    const EntityId first = addLine(0.0);
    const EntityId second = addCircle(20.0);
    const core::LayerId walls = addLayer("Walls");
    select({first, second});
    const core::EntityProperties before = propertiesOf(first);

    // Through the controls of the section General.
    choose(*child<ColorCombo>("propertiesColor"), kBlue);
    EXPECT_EQ(propertiesOf(first).color, kBlue);
    EXPECT_EQ(propertiesOf(second).color, kBlue);
    undoProperties();
    EXPECT_EQ(propertiesOf(second).color, before.color);

    choose(*child<LayerCombo>("propertiesLayer"), walls);
    EXPECT_EQ(propertiesOf(first).layer, walls);
    EXPECT_EQ(propertiesOf(second).layer, walls);
    undoProperties();
    EXPECT_EQ(propertiesOf(first).layer, before.layer);

    choose(*child<LinetypeCombo>("propertiesLinetype"), document().continuousLinetype());
    EXPECT_EQ(propertiesOf(first).linetype, document().continuousLinetype());
    undoProperties();
    EXPECT_EQ(propertiesOf(first).linetype, before.linetype);

    choose(*child<LineweightCombo>("propertiesLineweight"), 50);
    EXPECT_EQ(propertiesOf(second).lineweight, 50);
    undoProperties();
    EXPECT_EQ(propertiesOf(second).lineweight, before.lineweight);

    auto* scale = child<QLineEdit>("propertiesLinetypeScale");
    scale->setText(QStringLiteral("2.5"));
    scale->setModified(true);
    ASSERT_TRUE(QMetaObject::invokeMethod(scale, "editingFinished", Qt::DirectConnection));
    EXPECT_EQ(propertiesOf(first).linetypeScale, 2.5);
    EXPECT_EQ(propertiesOf(second).linetypeScale, 2.5);
    undoProperties();
    EXPECT_EQ(propertiesOf(first).linetypeScale, 1.0);

    // Refused: said, and nothing changes.
    EXPECT_FALSE(panel().setLinetypeScale(0.0));
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("Requires a positive number."));
    scale->setText(QStringLiteral("abc"));
    scale->setModified(true);
    ASSERT_TRUE(QMetaObject::invokeMethod(scale, "editingFinished", Qt::DirectConnection));
    EXPECT_EQ(scale->text(), QStringLiteral("1.0000"));
    EXPECT_EQ(propertiesOf(first), before);

    // A value the entities already have changes nothing: no undo step.
    const std::string undoName = editor().undoName();
    EXPECT_FALSE(panel().setColor(before.color));
    EXPECT_EQ(editor().undoName(), undoName);
}

TEST_F(PropertiesPanelTest, TheFilterByTypeLimitsWhatIsShownAndChanged) {
    const EntityId line = addLine(0.0);
    const EntityId smallCircle = addCircle(20.0);
    const EntityId largeCircle = addCircle(40.0);
    select({line, smallCircle, largeCircle});
    EXPECT_EQ(panel().typeLabels(), (QStringList{QStringLiteral("All (3)"), QStringLiteral("Line"),
                                                 QStringLiteral("Circle (2)")}));
    EXPECT_EQ(panel().targets().size(), 3U);

    auto* types = child<QComboBox>("propertiesType");
    activate(*types, 2);
    EXPECT_EQ(panel().typeFilter(), "CIRCLE");
    EXPECT_EQ(panel().targets(), (std::vector<EntityId>{smallCircle, largeCircle}));
    ASSERT_TRUE(panel().setColor(kRed));
    EXPECT_EQ(propertiesOf(smallCircle).color, kRed);
    EXPECT_EQ(propertiesOf(largeCircle).color, kRed);
    EXPECT_EQ(propertiesOf(line).color, core::Color::byLayer());
    EXPECT_EQ(types->currentIndex(), 2);

    // A single line: its geometry.
    ASSERT_TRUE(panel().setTypeFilter("LINE"));
    EXPECT_EQ(panel().targets(), std::vector<EntityId>{line});
    EXPECT_EQ(panel().geometry().size(), 5);
    EXPECT_EQ(panel().shown().color, core::Color::byLayer());

    // No arc is selected; All again.
    EXPECT_FALSE(panel().setTypeFilter("ARC"));
    activate(*types, 0);
    EXPECT_TRUE(panel().typeFilter().empty());
    EXPECT_FALSE(panel().shown().color.has_value());

    // A new selection without that type forgets the filter.
    ASSERT_TRUE(panel().setTypeFilter("LINE"));
    select({smallCircle});
    EXPECT_TRUE(panel().typeFilter().empty());
}

TEST_F(PropertiesPanelTest, EntitiesOfLockedLayersAreNeverShownNorChanged) {
    const core::LayerId locked = addLayer("Locked", true);
    const EntityId unlocked = addLine(0.0);
    const EntityId held = addLine(20.0);
    core::EntityProperties onLocked = propertiesOf(held);
    onLocked.layer = locked;
    setProperties(held, onLocked);

    select({unlocked, held});
    EXPECT_EQ(panel().selection(), std::vector<EntityId>{unlocked});
    ASSERT_TRUE(panel().setColor(kRed));
    EXPECT_EQ(propertiesOf(unlocked).color, kRed);
    EXPECT_EQ(propertiesOf(held).color, core::Color::byLayer());
}

TEST_F(PropertiesPanelTest, NothingChangesWhileEditingIsDisabled) {
    const EntityId line = addLine(0.0);
    select({line});
    panel().setEditingEnabled(false);
    EXPECT_FALSE(child<QGroupBox>("propertiesGeneral")->isEnabled());
    EXPECT_FALSE(panel().setColor(kRed));
    EXPECT_EQ(propertiesOf(line).color, core::Color::byLayer());
    panel().setEditingEnabled(true);
    EXPECT_TRUE(panel().setColor(kRed));
}

TEST_F(PropertiesPanelTest, ThePropertyBarChangesTheSelectionOrTheCurrentValues) {
    PropertyControls controls(panel());
    auto* color = controls.findChild<ColorCombo*>(QStringLiteral("propertyColorCombo"));
    auto* linetype = controls.findChild<LinetypeCombo*>(QStringLiteral("propertyLinetypeCombo"));
    auto* lineweight =
        controls.findChild<LineweightCombo*>(QStringLiteral("propertyLineweightCombo"));
    ASSERT_NE(color, nullptr);
    ASSERT_NE(linetype, nullptr);
    ASSERT_NE(lineweight, nullptr);

    // Without a selection: the current values.
    choose(*color, kRed);
    EXPECT_EQ(document().current().color, kRed);
    choose(*linetype, document().continuousLinetype());
    EXPECT_EQ(document().current().linetype, document().continuousLinetype());
    choose(*lineweight, 30);
    EXPECT_EQ(document().current().lineweight, 30);
    undoProperties();
    undoProperties();
    undoProperties();
    EXPECT_EQ(document().current().color, core::Color::byLayer());

    // With a selection: the entities, and the current values stay.
    const EntityId line = addLine(0.0);
    const EntityId circle = addCircle(20.0);
    core::EntityProperties blue = propertiesOf(circle);
    blue.color = kBlue;
    setProperties(circle, blue);
    select({line, circle});
    EXPECT_FALSE(color->currentData().isValid()) << "a mixed colour shows Varies";
    choose(*color, kRed);
    EXPECT_EQ(propertiesOf(line).color, kRed);
    EXPECT_EQ(propertiesOf(circle).color, kRed);
    EXPECT_EQ(document().current().color, core::Color::byLayer());
    choose(*lineweight, 50);
    EXPECT_EQ(propertiesOf(circle).lineweight, 50);
    undoProperties();
    undoProperties();
    EXPECT_EQ(propertiesOf(circle).color, kBlue);

    panel().setEditingEnabled(false);
    EXPECT_FALSE(controls.isEnabled());
}

TEST_F(PropertiesPanelTest, QuickSelectReplacesTheSelectionAndSaysHowMany) {
    const core::LayerId walls = addLayer("Walls");
    const EntityId line = addLine(0.0);
    const EntityId circle = addCircle(20.0);
    core::EntityProperties onWalls = propertiesOf(circle);
    onWalls.layer = walls;
    setProperties(circle, onWalls);
    setProperties(line, onWalls);
    addCircle(40.0);

    const QSignalSpy messages(&panel(), &PropertiesPanel::messageShown);
    cmd::QuickSelectCriteria criteria;
    criteria.property = cmd::QuickSelectCriteria::Property::Layer;
    criteria.value = "walls";
    ASSERT_TRUE(panel().applyQuickSelect(criteria, {}));
    EXPECT_EQ(panel().selection(), (std::vector<EntityId>{line, circle}));
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages.at(0).at(0).toString(), QStringLiteral("2 item(s) selected."));
    EXPECT_EQ(panel().shown().layer, walls);

    // Within the selection.
    criteria.type = "CIRCLE";
    ASSERT_TRUE(panel().applyQuickSelect(criteria, panel().selection()));
    EXPECT_EQ(panel().selection(), std::vector<EntityId>{circle});
    EXPECT_EQ(panel().lastMessage(), QStringLiteral("1 item(s) selected."));
}

/// The panel in the window: the commands, the actions and the controls of
/// the interface.
class PropertiesWindowTest : public testing::Test {
protected:
    PropertiesWindowTest() : m_window(MainWindow::InitialDrawing::Empty) {
        m_window.setScriptMode();
        m_panel = m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"));
    }

    [[nodiscard]] PropertiesPanel* panel() const { return m_panel; }

    [[nodiscard]] MainWindow& window() { return m_window; }

    [[nodiscard]] QLineEdit* input() const {
        return m_window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }

    void enter(const std::string& text) {
        events::type(input(), text);
        events::press(input(), Qt::Key_Return);
    }

    [[nodiscard]] QString history() const {
        return m_window.findChild<QPlainTextEdit*>(QStringLiteral("history"))->toPlainText();
    }

    [[nodiscard]] QuickSelectDialog* dialog() const {
        return m_window.findChild<QuickSelectDialog*>(QStringLiteral("quickSelectDialog"));
    }

    /// Closes the open window of QSELECT with OK or Cancel and disposes of it.
    void closeDialog(bool accept) {
        QuickSelectDialog* open = dialog();
        ASSERT_NE(open, nullptr);
        if (accept) {
            open->accept();
        } else {
            open->reject();
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EXPECT_EQ(dialog(), nullptr);
    }

private:
    MainWindow m_window;
    PropertiesPanel* m_panel{nullptr};
};

// A window costs much more than the panel alone, so one test checks several
// things in a row.
TEST_F(PropertiesWindowTest, TheCommandsAndActionsOpenWhatTheyMust) {
    ASSERT_NE(panel(), nullptr);
    for (const char* id : {"properties.panel", "edit.quick-select"}) {
        EXPECT_NE(window().actionRegistry().action(QString::fromLatin1(id)), nullptr) << id;
    }
    EXPECT_NE(window().findChild<PropertyControls*>(QStringLiteral("propertyControls")), nullptr);

    // PROPERTIES and its aliases show the panel.
    for (const char* name : {"PROPERTIES", "PR", "CH", "MO", "PROPS"}) {
        SCOPED_TRACE(name);
        panel()->hide();
        enter(name);
        EXPECT_FALSE(panel()->isHidden());
        EXPECT_FALSE(window().runner().isActive());
    }
    panel()->hide();
    window().actionRegistry().action(QStringLiteral("properties.panel"))->trigger();
    EXPECT_FALSE(panel()->isHidden());

    // Two lines and a circle.
    enter("LINE");
    enter("0,0");
    enter("10,0");
    enter("");
    enter("LINE");
    enter("0,10");
    enter("10,10");
    enter("");
    enter("CIRCLE");
    enter("50,0");
    enter("5");
    ASSERT_EQ(window().document().entityCount(), 3U);

    // QSELECT opens its window; OK selects the lines.
    enter("QSELECT");
    ASSERT_NE(dialog(), nullptr);
    EXPECT_FALSE(window().runner().isActive());
    auto* type = dialog()->findChild<QComboBox*>(QStringLiteral("quickSelectType"));
    auto* property = dialog()->findChild<QComboBox*>(QStringLiteral("quickSelectProperty"));
    activate(*type, type->findData(QStringLiteral("LINE")));
    activate(*property,
             property->findData(static_cast<int>(cmd::QuickSelectCriteria::Property::None)));
    closeDialog(true);
    EXPECT_EQ(window().selection().size(), 2U);
    EXPECT_TRUE(history().contains(QStringLiteral("2 item(s) selected.")));
    EXPECT_EQ(panel()->typeLabels(), QStringList{QStringLiteral("Line (2)")});

    // The panel changes them; U undoes it.
    ASSERT_TRUE(panel()->setColor(kRed));
    for (const core::EntityRecord& record : window().document().entities()) {
        const bool isLine = std::holds_alternative<core::Line>(record.entity);
        EXPECT_EQ(record.properties.color == kRed, isLine);
    }
    EXPECT_EQ(window().editor().undoName(), "Properties");
    enter("U");
    for (const core::EntityRecord& record : window().document().entities()) {
        EXPECT_EQ(record.properties.color, core::Color::byLayer());
    }

    // The action opens the window on the selection; Cancel keeps it.
    enter("QSELECT");
    closeDialog(true);
    ASSERT_EQ(window().selection().size(), 3U);
    window().actionRegistry().action(QStringLiteral("edit.quick-select"))->trigger();
    ASSERT_NE(dialog(), nullptr);
    EXPECT_TRUE(dialog()->searchesSelection());
    closeDialog(false);
    EXPECT_EQ(window().selection().size(), 3U);

    // A typed QSELECT clears the selection when it starts (ADR 0008): the
    // window still searches the one before, and Cancel gives it back.
    enter("QSELECT");
    ASSERT_NE(dialog(), nullptr);
    EXPECT_TRUE(dialog()->searchesSelection());
    EXPECT_EQ(dialog()->within().size(), 3U);
    closeDialog(false);
    EXPECT_EQ(window().selection().size(), 3U);

    // The bar of the properties: with a selection the entities, without it
    // the current values.
    auto* controls = window().findChild<PropertyControls*>(QStringLiteral("propertyControls"));
    auto* color = controls->findChild<ColorCombo*>(QStringLiteral("propertyColorCombo"));
    choose(*color, kBlue);
    for (const core::EntityRecord& record : window().document().entities()) {
        EXPECT_EQ(record.properties.color, kBlue);
    }
    EXPECT_EQ(window().document().current().color, core::Color::byLayer());
    events::press(input(), Qt::Key_Escape);
    EXPECT_TRUE(window().selection().isEmpty());
    EXPECT_EQ(panel()->typeLabels(), QStringList{QStringLiteral("No selection")});
    choose(*color, kRed);
    EXPECT_EQ(window().document().current().color, kRed);

    // While a command runs the panel and the bar wait.
    enter("LINE");
    EXPECT_FALSE(panel()->editingEnabled());
    EXPECT_FALSE(controls->isEnabled());
    EXPECT_FALSE(
        window().actionRegistry().action(QStringLiteral("edit.quick-select"))->isEnabled());
    events::press(input(), Qt::Key_Escape);
    EXPECT_TRUE(panel()->editingEnabled());
    EXPECT_TRUE(controls->isEnabled());
}

}  // namespace
}  // namespace tg::app
