// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/attribute_definition.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/color.hpp"
#include "tg/core/document.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/math.hpp"
#include "tg/core/raw_group.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/text.hpp"
#include "tg/core/text_affine.hpp"
#include "tg/core/transform2d.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kEps = 1e-9;

void expectPoint(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kEps);
    EXPECT_NEAR(actual.y, expected.y, kEps);
}

/// The difference of two angles, in (-pi, pi].
double angleBetween(double a, double b) {
    double difference = normalizeAngle(a - b);
    if (difference > kPi) {
        difference -= kTwoPi;
    }
    return difference;
}

void expectSameText(const Text& actual, const Text& expected) {
    expectPoint(actual.position, expected.position);
    expectPoint(actual.alignmentPoint, expected.alignmentPoint);
    EXPECT_NEAR(actual.height, expected.height, kEps);
    EXPECT_NEAR(actual.widthFactor, expected.widthFactor, kEps);
    EXPECT_NEAR(angleBetween(actual.rotation, expected.rotation), 0.0, kEps);
    EXPECT_NEAR(actual.obliqueAngle, expected.obliqueAngle, kEps);
    EXPECT_EQ(actual.mirroredX, expected.mirroredX);
    EXPECT_EQ(actual.mirroredY, expected.mirroredY);
    EXPECT_EQ(actual.horizontal, expected.horizontal);
    EXPECT_EQ(actual.vertical, expected.vertical);
    EXPECT_EQ(actual.value, expected.value);
}

Text textAt(const Vec2& position, std::string value, double height = 1.0) {
    Text text;
    text.position = position;
    text.height = height;
    text.value = std::move(value);
    return text;
}

AttributeDefinition definitionOf(const Vec2& position, std::string tag, std::string value,
                                 AttributeFlags flags = {}) {
    return {textAt(position, std::move(value)), std::move(tag), "", flags};
}

/// An entity of a block, with the default properties of a new drawing.
EntityRecord member(const Document& document, Entity entity) {
    EntityRecord record{EntityId{}, std::move(entity)};
    record.properties.layer = document.layerZero();
    record.properties.linetype = document.byLayerLinetype();
    return record;
}

Insert insertOf(std::string name, const Vec2& insertion, double scaleX = 1.0, double scaleY = 1.0,
                double degrees = 0.0) {
    Insert insert;
    insert.block = std::move(name);
    insert.insertion = insertion;
    insert.scaleX = scaleX;
    insert.scaleY = scaleY;
    insert.rotation = degreesToRadians(degrees);
    return insert;
}

/// The block "SYM", base (0, 0): a variable ATTDEF REF at (1, 0), an
/// invisible NOTE at (2, 0), a constant PIN at (3, 0) and a line.
Document drawingWithSymbol() {
    Document document = Document::newDrawing();
    BlockDefinition block{"SYM"};
    block.entities.push_back(member(document, definitionOf({1, 0}, "REF", "-K1")));
    block.entities.push_back(
        member(document, definitionOf({2, 0}, "NOTE", "n", {.invisible = true})));
    block.entities.push_back(
        member(document, definitionOf({3, 0}, "PIN", "1", {.invisible = true, .constant = true})));
    block.entities.push_back(member(document, Line{{0, 0}, {4, 0}}));
    document.replaceBlock("SYM", std::move(block));
    return document;
}

/// The corners of textBox() mapped by `transform`, in their order.
std::array<Vec2, 4> mappedBox(const Text& text, const Transform2D& transform) {
    std::array<Vec2, 4> corners = textBox(text);
    for (Vec2& corner : corners) {
        corner = transform.apply(corner);
    }
    return corners;
}

void expectExactImage(const Text& text, const Transform2D& transform) {
    const Text result = transformedText(text, transform);
    const std::array<Vec2, 4> expected = mappedBox(text, transform);
    const std::array<Vec2, 4> actual = textBox(result);
    for (std::size_t i = 0; const Vec2& corner : actual) {
        expectPoint(corner, expected.at(i++));
    }
}

TEST(Attributes, DefinitionsAreSupportedWithAValidTextAndTag) {
    EXPECT_TRUE(supported(definitionOf({0, 0}, "PIN", "")));
    EXPECT_FALSE(supported(definitionOf({0, 0}, "", "x")));
    EXPECT_FALSE(supported(definitionOf({0, 0}, "TWO WORDS", "x")));
    EXPECT_FALSE(supported(definitionOf({0, 0}, "TAB\tTAB", "x")));
    AttributeDefinition flat = definitionOf({0, 0}, "PIN", "x");
    flat.text.height = 0.0;
    EXPECT_FALSE(supported(flat));
    AttributeDefinition narrow = definitionOf({0, 0}, "PIN", "x");
    narrow.text.widthFactor = -1.0;
    EXPECT_FALSE(supported(narrow));
    AttributeDefinition far = definitionOf({0, 0}, "PIN", "x");
    far.text.position.x = std::nan("");
    EXPECT_FALSE(supported(far));
}

TEST(Attributes, ADefinitionShowsItsTag) {
    const AttributeDefinition definition = definitionOf({10, 20}, "A", "LONG_DEFAULT");
    EXPECT_EQ(shownText(definition).value, "A");
    const BBox box = boundingBox(definition);
    EXPECT_EQ(box.min(), shownText(definition).boundingBox().min());
    EXPECT_EQ(box.max(), shownText(definition).boundingBox().max());
    // Transformed and moved as that text, with its own default value back.
    const TransformOutcome moved = transformedOf(
        Document{}, definition, Similarity::translation({1, 1}), MirrorText::Readable);
    ASSERT_TRUE(moved.entity.has_value());
    const auto result = std::get<AttributeDefinition>(moved.entity.value_or(Entity{}));
    expectPoint(result.text.position, {11, 21});
    EXPECT_EQ(result.text.value, "LONG_DEFAULT");
    EXPECT_EQ(result.tag, "A");
}

TEST(Attributes, DefaultAttributesComeFromTheVariableDefinitions) {
    const Document document = drawingWithSymbol();
    const std::vector<Attribute> attributes =
        defaultAttributes(document, insertOf("SYM", {10, 20}));
    // The constant PIN gives no ATTRIB.
    ASSERT_EQ(attributes.size(), 2U);
    EXPECT_EQ(attributes[0].tag, "REF");
    EXPECT_EQ(attributes[0].text.value, "-K1");
    EXPECT_FALSE(attributes[0].invisible);
    expectPoint(attributes[0].text.position, {11, 20});
    EXPECT_EQ(attributes[0].id, EntityId{});
    EXPECT_EQ(attributes[1].tag, "NOTE");
    EXPECT_EQ(attributes[1].text.value, "n");
    EXPECT_TRUE(attributes[1].invisible);
    expectPoint(attributes[1].text.position, {12, 20});
    EXPECT_EQ(attributes[1].id, EntityId{});
    // Nothing without the definition.
    EXPECT_TRUE(defaultAttributes(document, insertOf("Missing", {0, 0})).empty());
}

TEST(Attributes, ANewAttributeTakesThePropertiesOfItsDefinition) {
    Document document = Document::newDrawing();
    Layer pins;
    pins.name = "PINS";
    pins.linetype = document.continuousLinetype();
    const LayerId layer = document.addLayer(pins);
    EntityRecord record = member(document, definitionOf({0, 0}, "PIN", "1"));
    record.properties.layer = layer;
    record.properties.color = Color::fromIndex(1);
    BlockDefinition block{"P"};
    block.entities.push_back(record);
    document.replaceBlock("P", std::move(block));

    const std::vector<Attribute> attributes = defaultAttributes(document, insertOf("P", {5, 5}));
    ASSERT_EQ(attributes.size(), 1U);
    EXPECT_EQ(attributes[0].properties.layer, layer);
    EXPECT_EQ(attributes[0].properties.color, Color::fromIndex(1));
    EXPECT_EQ(attributes[0].properties, record.properties);
}

TEST(Attributes, TheFlagsTheModelDoesNotHoldAreRewritten) {
    Document document = Document::newDrawing();
    AttributeDefinition definition =
        definitionOf({0, 0}, "CHECK", "x", {.invisible = true, .verify = true});
    definition.lockPosition = true;
    EntityRecord record = member(document, definition);
    // The groups 280 of the ATTDEF as read: the locked position is copied
    // as a field, not as these groups (docs/tappe/0.8.1/PT15, point 1).
    record.extras.specific = {{100, "AcDbAttributeDefinition"}, {280, "0"}, {280, "1"}};
    BlockDefinition block{"V"};
    block.entities.push_back(record);
    document.replaceBlock("V", std::move(block));

    const std::vector<Attribute> attributes = defaultAttributes(document, insertOf("V", {0, 0}));
    ASSERT_EQ(attributes.size(), 1U);
    const RawGroups expected{{70, "5"}};
    EXPECT_EQ(attributes[0].extras.specific, expected);
    EXPECT_TRUE(attributes[0].lockPosition);
    const AttributeFlags flags = attributeFlags(attributes[0]);
    EXPECT_TRUE(flags.invisible);
    EXPECT_TRUE(flags.verify);
    EXPECT_FALSE(flags.preset);
    EXPECT_FALSE(flags.constant);

    Insert insert = insertOf("V", {0, 0});
    insert.attributes = attributes;
    const std::vector<PlacedAttribute> placed = attributesOf(document, insert);
    ASSERT_EQ(placed.size(), 1U);
    EXPECT_TRUE(placed[0].flags.verify);
    // A group 70 written with blanks, as a DXF file may hold it.
    Attribute preset;
    preset.extras.specific.push_back({70, "     8"});
    EXPECT_TRUE(attributeFlags(preset).preset);
    EXPECT_FALSE(attributeFlags(preset).verify);
}

TEST(Attributes, AttributesOfGivesTheConstantDefinitionsInTheDrawing) {
    const Document document = drawingWithSymbol();
    Insert insert = insertOf("SYM", {10, 20}, 1.0, 1.0, 90.0);
    insert.attributes = defaultAttributes(document, insert);
    const std::vector<PlacedAttribute> placed = attributesOf(document, insert);
    ASSERT_EQ(placed.size(), 3U);
    EXPECT_EQ(placed[0].tag, "REF");
    EXPECT_FALSE(placed[0].fromDefinition);
    EXPECT_EQ(placed[1].tag, "NOTE");
    EXPECT_TRUE(placed[1].flags.invisible);
    // The pin, in the coordinates of the drawing.
    EXPECT_EQ(placed[2].tag, "PIN");
    EXPECT_EQ(placed[2].value, "1");
    EXPECT_EQ(placed[2].text.value, "1");
    EXPECT_TRUE(placed[2].fromDefinition);
    EXPECT_TRUE(placed[2].flags.constant);
    EXPECT_TRUE(placed[2].flags.invisible);
    expectPoint(placed[2].text.position, {10, 23});
    EXPECT_NEAR(angleBetween(placed[2].text.rotation, kPi / 2.0), 0.0, kEps);
}

TEST(Attributes, FindAttributeIgnoresTheCaseAndGivesTheFirst) {
    const Document document = drawingWithSymbol();
    Insert insert = insertOf("SYM", {0, 0});
    Attribute upper;
    upper.text = textAt({0, 0}, "first");
    upper.tag = "PIN";
    Attribute lower = upper;
    lower.text.value = "second";
    lower.tag = "pin";
    insert.attributes = {upper, lower};
    const std::optional<PlacedAttribute> found = findAttribute(document, insert, "Pin");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value_or(PlacedAttribute{}).value, "first");
    // Without ATTRIBs, the constant ATTDEF of the definition.
    insert.attributes.clear();
    const std::optional<PlacedAttribute> pin = findAttribute(document, insert, "pin");
    ASSERT_TRUE(pin.has_value());
    EXPECT_TRUE(pin.value_or(PlacedAttribute{}).fromDefinition);
    EXPECT_FALSE(findAttribute(document, insert, "ABSENT").has_value());
}

TEST(Attributes, TextUnderANonUniformScaling) {
    const Text text = textAt({1, 1}, "A");
    const Text result = transformedText(text, Transform2D::scaling(2.0, 1.0));
    expectPoint(result.position, {2, 1});
    EXPECT_NEAR(result.height, 1.0, kEps);
    EXPECT_NEAR(result.widthFactor, 2.0, kEps);
    EXPECT_NEAR(result.rotation, 0.0, kEps);
    EXPECT_NEAR(result.obliqueAngle, 0.0, kEps);
    EXPECT_FALSE(result.mirroredX);
    expectExactImage(text, Transform2D::scaling(2.0, 1.0));
}

TEST(Attributes, TextUnderAReflectionThatIsNotASimilarity) {
    const Text text = textAt({1, 1}, "A");
    const Text result = transformedText(text, Transform2D::scaling(-2.0, 1.0));
    expectPoint(result.position, {-2, 1});
    EXPECT_TRUE(result.mirroredX);
    EXPECT_FALSE(result.mirroredY);
    EXPECT_NEAR(result.height, 1.0, kEps);
    EXPECT_NEAR(result.widthFactor, 2.0, kEps);
    EXPECT_NEAR(result.rotation, 0.0, kEps);
    expectExactImage(text, Transform2D::scaling(-2.0, 1.0));
}

TEST(Attributes, TextUnderASimilarityIsTransformed) {
    Text text = textAt({3, 4}, "AB", 2.0);
    text.rotation = degreesToRadians(30.0);
    text.obliqueAngle = degreesToRadians(10.0);
    const Similarity turn = Similarity::rotation(degreesToRadians(40.0), {1, 1}) *
                            Similarity::scaling(2.0, {0, 0}).value_or(Similarity{});
    expectSameText(transformedText(text, turn.matrix()),
                   std::get<Text>(transformed(text, turn, MirrorText::Mirrored)));
    const Similarity mirror = Similarity::reflection({0, 0}, {1, 2}).value_or(Similarity{});
    expectSameText(transformedText(text, mirror.matrix()),
                   std::get<Text>(transformed(text, mirror, MirrorText::Mirrored)));
}

TEST(Attributes, ARotatedTextKeepsItsRotationUnderAScaling) {
    // A text at 90°, height 2, width factor 1, under diag(2, 1).
    Text text = textAt({5, 0}, "A", 2.0);
    text.rotation = kPi / 2.0;
    const Text result = transformedText(text, Transform2D::scaling(2.0, 1.0));
    EXPECT_NEAR(result.height, 4.0, kEps);
    EXPECT_NEAR(result.widthFactor, 0.5, kEps);
    EXPECT_NEAR(result.rotation, kPi / 2.0, kEps);  // not 180°
    expectPoint(result.position, {10, 0});
}

TEST(Attributes, TheObliqueAngleFollowsTheShear) {
    Text text = textAt({10, 0}, "A", 2.0);
    text.obliqueAngle = atan(0.25);
    const Text result = transformedText(text, Transform2D::scaling(2.0, 1.0));
    EXPECT_NEAR(tan(result.obliqueAngle), 0.5, kEps);
    EXPECT_NEAR(result.height, 2.0, kEps);
    EXPECT_NEAR(result.widthFactor, 2.0, kEps);
}

TEST(Attributes, EveryPointOfTheTextMapsExactly) {
    // A shear with a reflection, on texts with flags, oblique angles and
    // alignments: the corners of the box are the images of the corners.
    const Transform2D shear{2.0, 0.5, 0.3, -1.0, 5.0, 7.0};
    const Transform2D slant{1.0, 0.0, 0.7, 1.5, -2.0, 1.0};
    Text text = textAt({3, 4}, "AB", 2.0);
    text.rotation = degreesToRadians(30.0);
    text.obliqueAngle = degreesToRadians(10.0);
    for (const Transform2D& transform : {shear, slant}) {
        expectExactImage(text, transform);
        Text backward = text;
        backward.mirroredX = true;
        expectExactImage(backward, transform);
        Text upsideDown = text;
        upsideDown.mirroredY = true;
        expectExactImage(upsideDown, transform);
        Text centered = text;
        centered.horizontal = TextHorizontal::Center;
        centered.vertical = TextVertical::Middle;
        centered.alignmentPoint = {6, 1};
        expectExactImage(centered, transform);
        Text aligned = text;
        aligned.horizontal = TextHorizontal::Aligned;
        aligned.alignmentPoint = {9, 6};
        expectExactImage(aligned, transform);
        Text fitted = aligned;
        fitted.horizontal = TextHorizontal::Fit;
        fitted.mirroredX = true;
        expectExactImage(fitted, transform);
    }
}

TEST(Attributes, TheGeneratedGeometryDrawsTheVisibleAttributes) {
    Document document = drawingWithSymbol();
    Insert insert = insertOf("SYM", {10, 20});
    insert.attributes = defaultAttributes(document, insert);
    const EntityId id = document.add(insert);
    const std::vector<GeneratedItem> items = generate(document, *document.find(id));
    // The line, then REF; neither NOTE (invisible) nor the ATTDEFs.
    ASSERT_EQ(items.size(), 2U);
    ASSERT_TRUE(std::holds_alternative<Line>(items[0].shape));
    ASSERT_TRUE(std::holds_alternative<Text>(items[1].shape));
    EXPECT_EQ(std::get<Text>(items[1].shape).value, "-K1");
    expectPoint(std::get<Text>(items[1].shape).position, {11, 20});
}

TEST(Attributes, AttdispDecidesWhichAttributesOfAnInsertAreDrawn) {
    // docs/adr/0056, point 1: Normal the visible ones, On all, Off none.
    Document document = drawingWithSymbol();
    Insert insert = insertOf("SYM", {10, 20});
    insert.attributes = defaultAttributes(document, insert);
    ASSERT_EQ(insert.attributes.size(), 2U);
    ASSERT_TRUE(insert.attributes[1].invisible);
    const EntityId id = document.add(insert);
    const auto texts = [&] {
        std::vector<std::string> values;
        for (const GeneratedItem& item : generate(document, *document.find(id))) {
            if (const auto* text = std::get_if<Text>(&item.shape)) {
                values.push_back(text->value);
            }
        }
        return values;
    };
    EXPECT_EQ(texts(), (std::vector<std::string>{"-K1"}));
    const BBox normal = extent(document, *document.find(id));
    document.setAttributeDisplay(AttributeDisplay::On);
    EXPECT_EQ(texts(), (std::vector<std::string>{"-K1", "n"}));
    document.setAttributeDisplay(AttributeDisplay::Off);
    EXPECT_TRUE(texts().empty());
    // The extents measure the attributes whether drawn or not.
    EXPECT_EQ(extent(document, *document.find(id)).min(), normal.min());
    EXPECT_EQ(extent(document, *document.find(id)).max(), normal.max());
}

TEST(Attributes, AnInvisibleDefinitionInTheDrawingIsAlwaysDrawnPickedAndSnapped) {
    // An ATTDEF outside the blocks, on a layer that is on: drawn, picked and
    // snapped with ATTDISP Off, Normal and On, as in AutoCAD; an invisible
    // ATTRIB of the same drawing follows ATTDISP (docs/adr/0056, point 1).
    Document document = Document::newDrawing();
    // The block "TAG": a visible REF at (0, 0) and an invisible NOTE far from it.
    BlockDefinition block{"TAG"};
    block.entities.push_back(member(document, definitionOf({0, 0}, "REF", "r")));
    block.entities.push_back(
        member(document, definitionOf({20, 0}, "NOTE", "n", {.invisible = true})));
    document.replaceBlock("TAG", std::move(block));
    const EntityId definition =
        document.add(definitionOf({100, 0}, "HIDDEN", "h", {.invisible = true}));
    Insert insert = insertOf("TAG", {0, 50});
    insert.attributes = defaultAttributes(document, insert);
    document.add(insert);
    const Vec2 note{20, 50};  // the invisible NOTE of the insert
    SnapModes insertion{};
    insertion.endpoint = false;
    insertion.midpoint = false;
    insertion.center = false;
    insertion.intersection = false;
    for (const AttributeDisplay display :
         {AttributeDisplay::Off, AttributeDisplay::Normal, AttributeDisplay::On}) {
        SCOPED_TRACE(static_cast<int>(display));
        document.setAttributeDisplay(display);
        const std::vector<GeneratedItem> items = generate(document, *document.find(definition));
        ASSERT_EQ(items.size(), 1U);
        EXPECT_TRUE(items[0].properties.visible);
        EXPECT_EQ(std::get<Text>(items[0].shape).value, "HIDDEN");
        EXPECT_EQ(pickEntity(document, {100.2, 0.2}, 0.01), definition);
        EXPECT_EQ(selectInRectangle(document, BBox{{99, -1}, {110, 2}}, RectangleMode::Window),
                  (std::vector<EntityId>{definition}));
        const auto snap = findSnap(document, {100.1, 0.1}, 0.5, insertion);
        ASSERT_TRUE(snap.has_value());
        EXPECT_EQ(snap.value_or(SnapPoint{}), (SnapPoint{{100, 0}, SnapKind::Insertion}));
        // The NOTE of the insert only with On.
        const auto attribute = findSnap(document, note + Vec2{0.1, 0.1}, 0.5, insertion);
        if (display == AttributeDisplay::On) {
            EXPECT_EQ(attribute.value_or(SnapPoint{}), (SnapPoint{note, SnapKind::Insertion}));
        } else {
            EXPECT_FALSE(attribute.has_value());
        }
    }
}

TEST(Attributes, AnInvisibleDefinitionOnAFrozenLayerIsNotDrawn) {
    Document document = Document::newDrawing();
    Layer frozen;
    frozen.name = "Frozen";
    frozen.linetype = document.continuousLinetype();
    frozen.frozen = true;
    const LayerId layer = document.addLayer(frozen);
    const EntityId id = document.add(definitionOf({0, 0}, "HIDDEN", "h", {.invisible = true}));
    EntityProperties properties = document.find(id)->properties;
    properties.layer = layer;
    document.setProperties(id, properties);
    document.setAttributeDisplay(AttributeDisplay::On);
    EXPECT_TRUE(generate(document, *document.find(id)).empty());
    EXPECT_FALSE(pickEntity(document, {0.2, 0.2}, 0.01).has_value());
}

TEST(Attributes, MirrorTransformsTheAttributesAsTexts) {
    const Document document = drawingWithSymbol();
    Insert insert = insertOf("SYM", {10, 20});
    insert.attributes = defaultAttributes(document, insert);
    const Similarity mirror = Similarity::reflection({0, 0}, {0, 1}).value_or(Similarity{});
    const TransformOutcome readable = transformedOf(document, insert, mirror, MirrorText::Readable);
    ASSERT_TRUE(readable.entity.has_value());
    const auto result = std::get<Insert>(readable.entity.value_or(Entity{}));
    ASSERT_EQ(result.attributes.size(), 2U);
    for (std::size_t i = 0; i < result.attributes.size(); ++i) {
        const Text expected =
            std::get<Text>(transformed(insert.attributes[i].text, mirror, MirrorText::Readable));
        EXPECT_EQ(result.attributes[i].text, expected);
        EXPECT_FALSE(result.attributes[i].text.mirroredX);
        EXPECT_EQ(result.attributes[i].tag, insert.attributes[i].tag);
    }
    expectPoint(result.insertion, {-10, 20});
}

TEST(Attributes, ACopyHasAttributesWithoutHandles) {
    Insert insert = insertOf("SYM", {0, 0});
    Attribute attribute;
    attribute.id = EntityId{0x40};
    attribute.tag = "REF";
    attribute.extras.beforeEntity = {{102, "{ACAD_REACTORS"}, {330, "1F"}, {102, "}"}, {330, "2"}};
    attribute.extras.specific = {{70, "4"}};
    insert.attributes.push_back(attribute);
    const Insert copy = insertForCopy(insert);
    ASSERT_EQ(copy.attributes.size(), 1U);
    EXPECT_EQ(copy.attributes[0].id, EntityId{});
    const RawGroups owner{{330, "2"}};
    EXPECT_EQ(copy.attributes[0].extras.beforeEntity, owner);
    EXPECT_EQ(copy.attributes[0].extras.specific, attribute.extras.specific);
    EXPECT_EQ(copy.attributes[0].tag, "REF");
}

}  // namespace
}  // namespace tg::core
