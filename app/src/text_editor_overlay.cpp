// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "text_editor_overlay.hpp"

#include "qt_painter.hpp"
#include "text_format_bar.hpp"
#include "tg/core/document.hpp"
#include "tg/core/entity_record.hpp"
#include "tg/core/math.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/core/text_style_table.hpp"

#include <QChar>
#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <QWheelEvent>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <span>
#include <utility>
#include <variant>

namespace tg::app {
namespace {

// The space between the text and the border, in pixels at any zoom.
constexpr double kBorderMarginPx = 4.0;
// The selection is the colour of the highlight, see-through over the text.
constexpr std::uint8_t kSelectionAlpha = 90;
// The bytes after "%%" that make one character of a TEXT (core::displayText()).
constexpr std::string_view kPercentCodes = "cdpCDP%";
// The number of columns of the attachments of an MTEXT: left, centre, right.
constexpr int kAttachmentColumns = 3;

/// The bytes of the unit of a TEXT value that starts at `offset`: one of
/// %%c, %%d, %%p and %%%, read from the left as displayText() reads them, or
/// a UTF-8 character; a byte that starts no well-formed character alone.
std::size_t unitLength(std::string_view value, std::size_t offset) {
    constexpr std::size_t kPercentCode = 3;
    if (offset + kPercentCode <= value.size() && value.substr(offset, 2) == "%%" &&
        kPercentCodes.find(value[offset + 2]) != std::string_view::npos) {
        return kPercentCode;
    }
    const auto lead = static_cast<unsigned char>(value[offset]);
    std::size_t length = 1;
    if ((lead & 0xE0U) == 0xC0U) {
        length = 2;
    } else if ((lead & 0xF0U) == 0xE0U) {
        length = 3;
    } else if ((lead & 0xF8U) == 0xF0U) {
        length = 4;
    }
    if (offset + length > value.size()) {
        return 1;
    }
    for (std::size_t index = 1; index < length; ++index) {
        if ((static_cast<unsigned char>(value[offset + index]) & 0xC0U) != 0x80U) {
            return 1;
        }
    }
    return length;
}

/// The byte offset of every position of a TEXT value, from 0 to its size.
std::vector<std::size_t> positionOffsets(std::string_view value) {
    std::vector<std::size_t> offsets{0};
    for (std::size_t offset = 0; offset < value.size();) {
        offset += unitLength(value, offset);
        offsets.push_back(offset);
    }
    return offsets;
}

/// The distance of `point` from the segment from `from` to `to`.
double distanceToSegment(const core::Vec2& point, const core::Vec2& from, const core::Vec2& to) {
    const core::Vec2 along = to - from;
    const double lengthSquared = along.lengthSquared();
    if (lengthSquared <= 0.0) {
        return core::distance(point, from);
    }
    const double t = std::clamp(core::dot(point - from, along) / lengthSquared, 0.0, 1.0);
    return core::distance(point, from + (along * t));
}

core::Vec2 toVec2(const QPointF& point) {
    return {point.x(), point.y()};
}

/// True for the keys the editor takes for itself, before the shortcuts of
/// the window: `Canc` must not erase the selection of the drawing, a
/// letter must not reach the command line.
bool claimsKey(const QKeyEvent& event) {
    switch (event.key()) {
        case Qt::Key_Escape:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Home:
        case Qt::Key_End:
            return true;
        case Qt::Key_A:
            if (event.modifiers().testFlag(Qt::ControlModifier)) {
                return true;
            }
            break;
        case Qt::Key_B:
        case Qt::Key_I:
        case Qt::Key_U:
            if (event.modifiers().testFlag(Qt::ControlModifier)) {
                return true;
            }
            break;
        default:
            break;
    }
    // With Ctrl (but AltGr, which is Ctrl+Alt on Windows) or Meta the key
    // is a shortcut, whatever text it carries: Ctrl+B is the bold of PT13.
    const Qt::KeyboardModifiers modifiers = event.modifiers();
    if ((modifiers.testFlag(Qt::ControlModifier) && !modifiers.testFlag(Qt::AltModifier)) ||
        modifiers.testFlag(Qt::MetaModifier)) {
        return false;
    }
    const QString text = event.text();
    return !text.isEmpty() &&
           std::ranges::all_of(text, [](QChar character) { return character.isPrint(); });
}

}  // namespace

// --- TextLineEditModel --------------------------------------------------

TextLineEditModel::TextLineEditModel(core::Text text)
    : m_text(std::move(text)),
      m_offsets(positionOffsets(m_text.value)),
      m_caret(m_offsets.size() - 1),
      m_anchor(m_caret) {}

core::MTextCaret TextLineEditModel::caretAt(std::size_t position) const {
    assert(position < m_offsets.size() && "a position of the line");
    const core::TextFrame frame = core::textFrame(m_text);
    const std::string_view before = std::string_view(m_text.value).substr(0, m_offsets[position]);
    const double advance =
        core::textWidth(core::displayText(before)) * frame.height * frame.widthFactor;
    return {frame.origin + (frame.xAxis * advance), frame.height, 0};
}

core::Vec2 TextLineEditModel::up() const {
    return core::textFrame(m_text).yAxis;
}

std::size_t TextLineEditModel::closestPosition(const core::Vec2& point) const {
    const core::Vec2 rise = up();
    std::size_t closest = 0;
    double best = std::numeric_limits<double>::infinity();
    for (std::size_t position = 0; position < m_offsets.size(); ++position) {
        const core::MTextCaret caret = caretAt(position);
        const double gap =
            distanceToSegment(point, caret.point, caret.point + (rise * caret.height));
        if (gap < best) {
            best = gap;
            closest = position;
        }
    }
    return closest;
}

void TextLineEditModel::setCaret(std::size_t position) {
    assert(position < m_offsets.size() && "a position of the line");
    m_caret = position;
    m_anchor = position;
}

void TextLineEditModel::select(std::size_t anchor, std::size_t caret) {
    assert(anchor < m_offsets.size() && caret < m_offsets.size() && "positions of the line");
    m_anchor = anchor;
    m_caret = caret;
}

void TextLineEditModel::insertText(std::string_view text) {
    std::string line;
    std::ranges::copy_if(text, std::back_inserter(line),
                         [](char byte) { return byte != '\n' && byte != '\r'; });
    replace(std::min(m_caret, m_anchor), std::max(m_caret, m_anchor), line);
}

void TextLineEditModel::deleteBackward() {
    if (hasSelection()) {
        replace(std::min(m_caret, m_anchor), std::max(m_caret, m_anchor), {});
    } else if (m_caret > 0) {
        replace(m_caret - 1, m_caret, {});
    }
}

void TextLineEditModel::deleteForward() {
    if (hasSelection()) {
        replace(std::min(m_caret, m_anchor), std::max(m_caret, m_anchor), {});
    } else if (m_caret + 1 < m_offsets.size()) {
        replace(m_caret, m_caret + 1, {});
    }
}

void TextLineEditModel::replace(std::size_t from, std::size_t to, std::string_view text) {
    const std::size_t begin = m_offsets[from];
    const std::size_t end = m_offsets[to];
    m_text.value.replace(begin, end - begin, text);
    m_offsets = positionOffsets(m_text.value);
    // Written next to a "%%" or a "%", the text may end inside a unit that
    // it completes: the cursor goes after that unit.
    const auto found = std::ranges::lower_bound(m_offsets, begin + text.size());
    m_caret = std::min(static_cast<std::size_t>(std::distance(m_offsets.begin(), found)),
                       m_offsets.size() - 1);
    m_anchor = m_caret;
}

// --- TextEditorOverlay --------------------------------------------------

TextEditorOverlay::TextEditorOverlay(QWidget* parent, const render::Viewport& viewport)
    : QWidget(parent), m_viewport(viewport), m_formatBar(new TextFormatBar(this)) {
    assert(parent != nullptr && "the text editor needs a parent drawing view");
    setObjectName(QStringLiteral("textEditor"));
    setFocusPolicy(Qt::StrongFocus);
    hide();
    m_formatBar->hide();
}

void TextEditorOverlay::open(const cmd::TextEditRequest& request, const core::Document& document) {
    close();
    m_document = &document;
    if (const auto* text = std::get_if<core::Text>(&request.shown)) {
        core::Text edited = *text;
        edited.value = request.value;
        m_line.emplace(std::move(edited));
    } else {
        core::MText edited = std::get<core::MText>(request.shown);
        edited.value = request.value;
        core::MTextEditOptions options;
        // As the drawing lays it out (core::generateOf()): the width factor
        // and the oblique angle of its style, 1 and 0 without it.
        if (const core::TextStyle* style = document.textStyles().find(edited.style)) {
            options.widthFactor = style->widthFactor;
            options.oblique = style->oblique;
        }
        options.measurement = request.measurement;
        core::MTextEditModel& model = m_mtext.emplace(std::move(edited), std::move(options));
        model.setCaret(model.positionCount() - 1);
    }
    m_request = request;
    m_openedBox = currentBox();
    setGeometry(parentWidget()->rect());
    show();
    raise();
    m_formatBar->adjustSize();
    const auto editorBorder = border();
    double left = std::numeric_limits<double>::infinity();
    double top = std::numeric_limits<double>::infinity();
    for (const core::Vec2& corner : editorBorder) {
        const core::Vec2 screen = m_viewport.worldToScreen(corner);
        left = std::min(left, screen.x);
        top = std::min(top, screen.y);
    }
    const int barX = std::clamp(static_cast<int>(std::lround(left)), 0,
                                std::max(0, width() - m_formatBar->width()));
    const int barY = std::clamp(static_cast<int>(std::lround(top)) - m_formatBar->height(), 0,
                                std::max(0, height() - m_formatBar->height()));
    m_formatBar->move(barX, barY);
    m_formatBar->show();
    setFocus(Qt::OtherFocusReason);
    changed();
}

void TextEditorOverlay::close() {
    m_request.reset();
    m_mtext.reset();
    m_line.reset();
    m_document = nullptr;
    m_selecting = false;
    m_formatBar->hide();
    hide();
}

std::string TextEditorOverlay::value() const {
    if (m_mtext) {
        return m_mtext->value();
    }
    if (m_line) {
        return m_line->value();
    }
    return {};
}

const core::MTextEditModel* TextEditorOverlay::model() const {
    return m_mtext ? &*m_mtext : nullptr;
}

const TextLineEditModel* TextEditorOverlay::lineModel() const {
    return m_line ? &*m_line : nullptr;
}

std::vector<cmd::Input> TextEditorOverlay::confirmation() const {
    if (!m_request) {
        return {};
    }
    const std::string current = value();
    // Unchanged, or changed back: Enter, which keeps the value (verbale
    // della rilettura della #993, [bloccante] 1).
    if (current == m_request->value) {
        return {cmd::ConfirmInput{}};
    }
    // The text of a dimension: the measurement of the first <> stands for it.
    const bool dimension = m_request->measurement.has_value();
    const bool empty = m_mtext ? m_mtext->isEmpty() : (!m_line || m_line->isEmpty());
    if (empty) {
        // As AutoCAD: a TEXT or an MTEXT emptied stays as it was, a
        // dimension goes back to its measurement, and the hidden codes left
        // have nothing to act on (docs/adr/0054, point 5).
        if (dimension) {
            return {cmd::TextInput{""}};
        }
        return {cmd::ConfirmInput{}};
    }
    const bool newMText = !m_request->target && !dimension && m_mtext.has_value();
    if (newMText) {
        // MTEXT takes lines until an empty one.
        return {cmd::TextInput{current}, cmd::ConfirmInput{}};
    }
    return {cmd::TextInput{current}};
}

void TextEditorOverlay::applyFormat(const core::MTextFormatChange& change) {
    if (!m_mtext) {
        return;
    }
    m_mtext->applyFormat(change);
    changed();
}

void TextEditorOverlay::setStyle(const render::SceneStyle& style) {
    m_style = style;
    update();
}

std::array<core::Vec2, 4> TextEditorOverlay::border() const {
    if (!isOpen()) {
        return {};
    }
    return corners(currentBox(), margin());
}

TextEditorOverlay::Box TextEditorOverlay::currentBox() const {
    const core::FontMetrics& font = core::fontMetrics();
    Box box;
    if (m_line) {
        const core::TextFrame frame = core::textFrame(m_line->shown());
        box.origin = frame.origin;
        box.xAxis = frame.xAxis;
        box.yAxis = frame.yAxis;
        box.right =
            core::textWidth(core::displayText(m_line->value())) * frame.height * frame.widthFactor;
        box.bottom = font.descent * frame.height;
        box.top = font.ascent * frame.height;
        return box;
    }
    if (!m_mtext) {
        return box;
    }
    const core::MText& shown = m_mtext->shown();
    const auto direction = core::sinCos(shown.rotation);
    box.origin = shown.insertion;
    box.xAxis = {direction.cosine, direction.sine};
    box.yAxis = box.xAxis.perpendicular();
    box.left = std::numeric_limits<double>::infinity();
    box.right = -std::numeric_limits<double>::infinity();
    box.bottom = std::numeric_limits<double>::infinity();
    box.top = -std::numeric_limits<double>::infinity();
    if (shown.width > 0.0) {
        // As wide as the column, which the attachment puts to the right of
        // the insertion, around it or to its left.
        const int column = (static_cast<int>(shown.attachment) - 1) % kAttachmentColumns;
        box.left = -shown.width * static_cast<double>(column) / 2.0;
        box.right = box.left + shown.width;
    }
    // Every cursor position, from the descent to the ascent of its row: the
    // text without a column, and what goes past it (a long word).
    for (std::size_t position = 0; position < m_mtext->positionCount(); ++position) {
        const core::MTextCaret caret = m_mtext->caretAt(position);
        const core::Vec2 relative = caret.point - box.origin;
        const double x = core::dot(relative, box.xAxis);
        const double y = core::dot(relative, box.yAxis);
        box.left = std::min(box.left, x);
        box.right = std::max(box.right, x);
        box.bottom = std::min(box.bottom, y + (font.descent * caret.height));
        box.top = std::max(box.top, y + (font.ascent * caret.height));
    }
    return box;
}

std::array<core::Vec2, 4> TextEditorOverlay::corners(const Box& box, double margin) {
    const double left = box.left - margin;
    const double right = box.right + margin;
    const double bottom = box.bottom - margin;
    const double top = box.top + margin;
    const auto at = [&box](double x, double y) {
        return box.origin + (box.xAxis * x) + (box.yAxis * y);
    };
    return {at(left, bottom), at(right, bottom), at(right, top), at(left, top)};
}

bool TextEditorOverlay::insideBorder(const core::Vec2& point) const {
    const Box box = currentBox();
    const double determinant = core::cross(box.xAxis, box.yAxis);
    if (std::abs(determinant) <= 0.0) {
        return false;
    }
    // The coordinates of the point in the system of the text, whose axes
    // are not perpendicular with an oblique angle.
    const core::Vec2 relative = point - box.origin;
    const double x = core::cross(relative, box.yAxis) / determinant;
    const double y = core::cross(box.xAxis, relative) / determinant;
    const double grow = margin();
    return x >= box.left - grow && x <= box.right + grow && y >= box.bottom - grow &&
           y <= box.top + grow;
}

double TextEditorOverlay::margin() const {
    return kBorderMarginPx / m_viewport.scale();
}

std::size_t TextEditorOverlay::positionCount() const {
    if (m_mtext) {
        return m_mtext->positionCount();
    }
    return m_line ? m_line->positionCount() : 1;
}

std::size_t TextEditorOverlay::caret() const {
    if (m_mtext) {
        return m_mtext->caret();
    }
    return m_line ? m_line->caret() : 0;
}

std::size_t TextEditorOverlay::anchor() const {
    if (m_mtext) {
        return m_mtext->anchor();
    }
    return m_line ? m_line->anchor() : 0;
}

core::MTextCaret TextEditorOverlay::caretAt(std::size_t position) const {
    if (m_mtext) {
        return m_mtext->caretAt(position);
    }
    return m_line ? m_line->caretAt(position) : core::MTextCaret{};
}

core::Vec2 TextEditorOverlay::up() const {
    if (m_line) {
        return m_line->up();
    }
    const double rotation = m_mtext ? m_mtext->shown().rotation : 0.0;
    const auto direction = core::sinCos(rotation);
    return core::Vec2{direction.cosine, direction.sine}.perpendicular();
}

std::size_t TextEditorOverlay::closestPosition(const core::Vec2& point) const {
    if (m_mtext) {
        return m_mtext->closestPosition(point);
    }
    return m_line ? m_line->closestPosition(point) : 0;
}

void TextEditorOverlay::moveCaret(std::size_t position, bool extend) {
    if (m_mtext) {
        core::MTextEditModel& model = *m_mtext;
        if (extend) {
            model.select(model.anchor(), position);
        } else {
            model.setCaret(position);
        }
    } else if (m_line) {
        TextLineEditModel& model = *m_line;
        if (extend) {
            model.select(model.anchor(), position);
        } else {
            model.setCaret(position);
        }
    }
    changed();
}

std::size_t TextEditorOverlay::verticalPosition(int direction) const {
    if (!m_mtext) {
        return caret();
    }
    const core::MTextEditModel& model = *m_mtext;
    const core::MTextCaret current = model.caretAt(model.caret());
    const bool first = current.row == 0;
    const bool last = current.row + 1 >= model.rowCount();
    if ((direction < 0 && first) || (direction > 0 && last)) {
        return model.caret();
    }
    const std::size_t row = direction < 0 ? current.row - 1 : current.row + 1;
    const core::MTextCaret target = model.caretAt(model.rowStart(row));
    // The cursor carried onto the other row, halfway up it: the nearest
    // position is there, below or above the cursor.
    const core::Vec2 rise = up();
    const double shift = core::dot(target.point - current.point, rise);
    return model.closestPosition(current.point + (rise * (shift + (target.height / 2.0))));
}

void TextEditorOverlay::insertText(std::string_view text) {
    if (m_mtext) {
        m_mtext->insertText(text);
    } else if (m_line) {
        m_line->insertText(text);
    }
    changed();
}

void TextEditorOverlay::newParagraph() {
    if (m_mtext) {
        m_mtext->newParagraph();
        changed();
    }
}

void TextEditorOverlay::deleteBackward() {
    if (m_mtext) {
        m_mtext->deleteBackward();
    } else if (m_line) {
        m_line->deleteBackward();
    }
    changed();
}

void TextEditorOverlay::deleteForward() {
    if (m_mtext) {
        m_mtext->deleteForward();
    } else if (m_line) {
        m_line->deleteForward();
    }
    changed();
}

void TextEditorOverlay::changed() {
    m_formatBar->refresh();
    update();
    emit selectionChanged();
}

bool TextEditorOverlay::event(QEvent* event) {
    if (event->type() == QEvent::ShortcutOverride && isOpen()) {
        const auto* key = dynamic_cast<const QKeyEvent*>(event);
        if (key != nullptr && claimsKey(*key)) {
            // The key comes as a key press, and no shortcut of the window
            // fires.
            event->accept();
            return true;
        }
    }
    return QWidget::event(event);
}

void TextEditorOverlay::paintEvent(QPaintEvent* /*event*/) {
    if (!isOpen() || m_document == nullptr) {
        return;
    }
    QPainter painter(this);
    QtPainter backend(painter);
    const auto toScreen = [this](const std::array<core::Vec2, 4>& points) {
        std::array<core::Vec2, 4> screen{};
        std::ranges::transform(points, screen.begin(), [this](const core::Vec2& point) {
            return m_viewport.worldToScreen(point);
        });
        return screen;
    };
    const double grow = margin();
    // The view still draws the text being edited: the background hides it,
    // where it was when the editor opened and where it is now.
    backend.fillPolygon(toScreen(corners(m_openedBox, grow)), m_style.background);
    const std::array<core::Vec2, 4> frame = toScreen(corners(currentBox(), grow));
    backend.fillPolygon(frame, m_style.background);

    const core::Vec2 rise = up();
    render::Color selection = m_style.highlight;
    selection.a = kSelectionAlpha;
    const std::size_t last = std::max(caret(), anchor());
    for (std::size_t position = std::min(caret(), anchor()); position < last; ++position) {
        const core::MTextCaret from = caretAt(position);
        const core::MTextCaret to = caretAt(position + 1);
        // A space where a row breaks is not drawn: nothing to show.
        if (from.row != to.row) {
            continue;
        }
        backend.fillPolygon(toScreen({from.point, to.point, to.point + (rise * to.height),
                                      from.point + (rise * from.height)}),
                            selection);
    }

    // The text in the main colour of the theme, as the drawing lays it out.
    render::SceneStyle textStyle = m_style;
    textStyle.preview = m_style.entity;
    textStyle.devicePixelRatio = devicePixelRatioF();
    core::Entity entity;
    if (m_mtext) {
        entity = m_mtext->shown();
    } else if (m_line) {
        entity = m_line->shown();
    }
    render::renderPreview(*m_document, std::span<const core::Entity>(&entity, 1), m_viewport,
                          textStyle, backend);

    const core::MTextCaret cursor = caretAt(caret());
    backend.setPen(m_style.entity, 0.0, render::LineStyle::Solid);
    backend.drawLine(m_viewport.worldToScreen(cursor.point),
                     m_viewport.worldToScreen(cursor.point + (rise * cursor.height)));
    backend.setPen(m_style.highlight, 0.0, render::LineStyle::Solid);
    backend.drawPolyline(frame, true);
}

void TextEditorOverlay::keyPressEvent(QKeyEvent* event) {
    if (!isOpen()) {
        QWidget::keyPressEvent(event);
        return;
    }
    const bool shift = event->modifiers().testFlag(Qt::ShiftModifier);
    const bool control = event->modifiers().testFlag(Qt::ControlModifier);
    switch (event->key()) {
        case Qt::Key_Escape:
            emit cancelled();
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            // A TEXT is one line: Enter confirms it.
            if (control || m_line) {
                emit confirmed();
            } else {
                newParagraph();
            }
            break;
        case Qt::Key_Backspace:
            deleteBackward();
            break;
        case Qt::Key_Delete:
            deleteForward();
            break;
        case Qt::Key_Left:
            if (caret() != anchor() && !shift) {
                moveCaret(std::min(caret(), anchor()), false);
            } else {
                moveCaret(caret() > 0 ? caret() - 1 : 0, shift);
            }
            break;
        case Qt::Key_Right:
            if (caret() != anchor() && !shift) {
                moveCaret(std::max(caret(), anchor()), false);
            } else {
                moveCaret(std::min(caret() + 1, positionCount() - 1), shift);
            }
            break;
        case Qt::Key_Up:
            moveCaret(verticalPosition(-1), shift);
            break;
        case Qt::Key_Down:
            moveCaret(verticalPosition(1), shift);
            break;
        case Qt::Key_Home:
            if (m_mtext && !control) {
                const core::MTextEditModel& model = *m_mtext;
                moveCaret(model.rowStart(model.caretAt(model.caret()).row), shift);
            } else {
                moveCaret(0, shift);
            }
            break;
        case Qt::Key_End:
            if (m_mtext && !control) {
                const core::MTextEditModel& model = *m_mtext;
                moveCaret(model.rowEnd(model.caretAt(model.caret()).row), shift);
            } else {
                moveCaret(positionCount() - 1, shift);
            }
            break;
        default:
            if (event->key() == Qt::Key_A && control) {
                moveCaret(0, false);
                moveCaret(positionCount() - 1, true);
                break;
            }
            if (control && m_mtext &&
                (event->key() == Qt::Key_B || event->key() == Qt::Key_I ||
                 event->key() == Qt::Key_U)) {
                const core::MTextFormat format = m_mtext->formatAtCaret();
                core::MTextFormatChange change;
                if (event->key() == Qt::Key_B) {
                    change.kind = core::MTextFormatChange::Kind::Bold;
                    change.on = !format.bold;
                } else if (event->key() == Qt::Key_I) {
                    change.kind = core::MTextFormatChange::Kind::Italic;
                    change.on = !format.italic;
                } else {
                    change.kind = core::MTextFormatChange::Kind::Underline;
                    change.on = !format.underline;
                }
                applyFormat(change);
                break;
            }
            if (!claimsKey(*event)) {
                // Not the editor's: the function keys of the window.
                event->ignore();
                return;
            }
            insertText(event->text().toStdString());
            break;
    }
    event->accept();
}

void TextEditorOverlay::mousePressEvent(QMouseEvent* event) {
    if (!isOpen() || event->button() == Qt::MiddleButton) {
        // Pan, in the view below.
        event->ignore();
        return;
    }
    event->accept();
    if (event->button() != Qt::LeftButton) {
        return;  // the right button does nothing while writing
    }
    const core::Vec2 point = m_viewport.screenToWorld(toVec2(event->position()));
    if (!insideBorder(point)) {
        emit confirmed();
        return;
    }
    m_selecting = true;
    moveCaret(closestPosition(point), event->modifiers().testFlag(Qt::ShiftModifier));
}

void TextEditorOverlay::mouseMoveEvent(QMouseEvent* event) {
    if (!m_selecting || !isOpen() || !event->buttons().testFlag(Qt::LeftButton)) {
        // The pointer and a pan are the view's.
        event->ignore();
        return;
    }
    moveCaret(closestPosition(m_viewport.screenToWorld(toVec2(event->position()))), true);
    event->accept();
}

void TextEditorOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        event->accept();
        return;
    }
    event->ignore();
}

void TextEditorOverlay::mouseDoubleClickEvent(QMouseEvent* event) {
    if (isOpen() && event->button() != Qt::MiddleButton) {
        mousePressEvent(event);
        return;
    }
    // The middle button zooms to the extents, in the view below.
    event->ignore();
}

void TextEditorOverlay::wheelEvent(QWheelEvent* event) {
    // Zoom, in the view below; the editor follows its viewport. Handed over
    // here: Qt passes a wheel event it did not get from the window system
    // (a test, a script) to nobody but its receiver.
    QCoreApplication::sendEvent(parentWidget(), event);
}

}  // namespace tg::app
