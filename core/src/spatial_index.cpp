// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/spatial_index.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <tuple>

namespace tg::core {

namespace {

bool overlaps(const BBox& a, const BBox& b) {
    return a.min().x <= b.max().x && a.max().x >= b.min().x && a.min().y <= b.max().y &&
           a.max().y >= b.min().y;
}

bool encloses(const BBox& outer, const BBox& inner) {
    return outer.min().x <= inner.min().x && outer.min().y <= inner.min().y &&
           inner.max().x <= outer.max().x && inner.max().y <= outer.max().y;
}

BBox unite(BBox a, const BBox& b) {
    a.expand(b);
    return a;
}

bool sameBox(const BBox& a, const BBox& b) {
    // Exact comparison is intended: a parent box is computed with min/max
    // from its children's, which is exact, so the two must be identical.
    return a.min() == b.min() && a.max() == b.max();
}

/// How "big" a box is, for the split and placement heuristics. Area comes
/// first, as in Guttman's paper; the margin (half perimeter) decides among
/// boxes of equal area, which matters for degenerate boxes: a set of points
/// on a line has no area at all.
struct Size {
    double area{0.0};
    double margin{0.0};
};

Size sizeOf(const BBox& box) {
    return {box.width() * box.height(), box.width() + box.height()};
}

/// Growth of `box` when `added` is added to it.
Size enlargement(const BBox& box, const BBox& added) {
    const Size before = sizeOf(box);
    const Size after = sizeOf(unite(box, added));
    return {after.area - before.area, after.margin - before.margin};
}

// Lexicographic order on (area, margin). These are heuristic choices, not
// geometric facts: ties only decide which of two valid trees is built.
bool operator<(const Size& a, const Size& b) {
    return std::tie(a.area, a.margin) < std::tie(b.area, b.margin);
}

// Groups of a split.
constexpr std::uint8_t kUnassigned = 0;
constexpr std::uint8_t kGroupA = 1;
constexpr std::uint8_t kGroupB = 2;

/// A stack with a fixed capacity, kept on the call stack: the tree is walked
/// without recursion and without allocating.
template <typename T, std::size_t Capacity>
class FixedStack {
public:
    void push(const T& value) {
        assert(m_size < Capacity && "spatial index deeper than kMaxHeight");
        slots()[m_size++] = value;
    }

    T pop() {
        assert(m_size > 0);
        return slots()[--m_size];
    }

    [[nodiscard]] T& top() {
        assert(m_size > 0);
        return slots()[m_size - 1];
    }

    [[nodiscard]] bool isEmpty() const { return m_size == 0; }

    [[nodiscard]] std::span<const T> items() const {
        return std::span<const T>(m_items).first(m_size);
    }

private:
    [[nodiscard]] std::span<T> slots() { return m_items; }

    std::array<T, Capacity> m_items{};
    std::size_t m_size{0};
};

}  // namespace

// ---------------------------------------------------------------------------
// Public interface

void SpatialIndex::insert(EntityId id, const BBox& box) {
    assert(!box.isEmpty() && "a spatial index entry needs a non-empty box");
    insertAtLevel(Entry{box, id.value}, 0);
    ++m_size;
}

bool SpatialIndex::erase(EntityId id, const BBox& box) {
    if (!m_root || box.isEmpty()) {
        return false;
    }
    // Depth-first search of the leaf holding `id`, entering only the
    // children whose box encloses `box`. When the leaf is found, `path`
    // holds the nodes above it, each with the slot after the one followed.
    FixedStack<PathStep, kMaxHeight> path;
    path.push(PathStep{*m_root, 0});
    std::optional<std::size_t> slotInLeaf;
    while (!path.isEmpty()) {
        PathStep& step = path.top();
        const std::span<const Entry> items = entries(step.node);
        if (m_nodes[step.node].level == 0) {
            const auto it = std::ranges::find(items, id.value, &Entry::ref);
            if (it != items.end()) {
                slotInLeaf = static_cast<std::size_t>(it - items.begin());
                break;
            }
            path.pop();
            continue;
        }
        while (step.slot < items.size() && !encloses(items[step.slot].box, box)) {
            ++step.slot;
        }
        if (step.slot == items.size()) {
            path.pop();
            continue;
        }
        const auto child = static_cast<NodeIndex>(items[step.slot].ref);
        ++step.slot;
        path.push(PathStep{child, 0});
    }
    if (!slotInLeaf) {
        return false;
    }

    NodeIndex node = path.pop().node;
    removeEntry(node, *slotInLeaf);
    --m_size;

    // Condense the tree: going up, dissolve the nodes left with too few
    // entries, keeping their entries aside, and recompute the other boxes.
    FixedStack<Orphan, kMaxHeight * kMinEntries> orphans;
    while (!path.isEmpty()) {
        const PathStep parent = path.pop();
        const std::size_t slot = parent.slot - 1;
        if (m_nodes[node].count < kMinEntries) {
            for (const Entry& entry : entries(node)) {
                orphans.push(Orphan{entry, m_nodes[node].level});
            }
            freeNode(node);
            removeEntry(parent.node, slot);
        } else {
            entries(parent.node)[slot].box = nodeBox(node);
        }
        node = parent.node;
    }
    for (const Orphan& orphan : orphans.items()) {
        insertAtLevel(orphan.entry, orphan.level);
    }
    // An inner root with a single child is useless: the child becomes root.
    while (m_nodes[*m_root].level > 0 && m_nodes[*m_root].count == 1) {
        const NodeIndex old = *m_root;
        m_root = static_cast<NodeIndex>(entries(old).front().ref);
        freeNode(old);
    }
    return true;
}

void SpatialIndex::query(const BBox& area, std::vector<EntityId>& out) const {
    if (!m_root || area.isEmpty()) {
        return;
    }
    // Pending nodes: at most kMaxEntries per level of the tree.
    FixedStack<NodeIndex, kMaxHeight * kMaxEntries> pending;
    pending.push(*m_root);
    while (!pending.isEmpty()) {
        const NodeIndex node = pending.pop();
        const bool leaf = m_nodes[node].level == 0;
        for (const Entry& entry : entries(node)) {
            if (!overlaps(entry.box, area)) {
                continue;
            }
            if (leaf) {
                out.push_back(EntityId{entry.ref});
            } else {
                pending.push(static_cast<NodeIndex>(entry.ref));
            }
        }
    }
}

BBox SpatialIndex::bounds() const {
    return m_root ? nodeBox(*m_root) : BBox{};
}

void SpatialIndex::clear() {
    m_nodes.clear();
    m_entries.clear();
    m_free.clear();
    m_root.reset();
    m_size = 0;
}

bool SpatialIndex::checkInvariants() const {
    if (!m_root) {
        return m_size == 0 && m_nodes.empty() && m_entries.empty() && m_free.empty();
    }
    const NodeIndex root = *m_root;
    if (m_nodes[root].level > 0 && m_nodes[root].count < 2) {
        return false;  // erase() collapses inner roots with one child
    }
    std::size_t leafEntries = 0;
    FixedStack<NodeIndex, kMaxHeight * kMaxEntries> pending;
    pending.push(root);
    while (!pending.isEmpty()) {
        const NodeIndex node = pending.pop();
        const Node& info = m_nodes[node];
        if (info.count > kMaxEntries || (node != root && info.count < kMinEntries)) {
            return false;
        }
        if (info.level == 0) {
            leafEntries += info.count;
            if (std::ranges::any_of(entries(node),
                                    [](const Entry& e) { return e.box.isEmpty(); })) {
                return false;
            }
            continue;
        }
        for (const Entry& entry : entries(node)) {
            if (entry.ref >= m_nodes.size()) {
                return false;
            }
            const auto child = static_cast<NodeIndex>(entry.ref);
            // Levels decrease by one at every step, so all leaves (level 0)
            // are at the same depth.
            if (m_nodes[child].level + 1 != info.level || !sameBox(entry.box, nodeBox(child))) {
                return false;
            }
            pending.push(child);
        }
    }
    return leafEntries == m_size;
}

// ---------------------------------------------------------------------------
// Node storage

std::span<SpatialIndex::Entry> SpatialIndex::entries(NodeIndex node) {
    return std::span<Entry>(m_entries).subspan(node * kMaxEntries, m_nodes[node].count);
}

std::span<const SpatialIndex::Entry> SpatialIndex::entries(NodeIndex node) const {
    return std::span<const Entry>(m_entries).subspan(node * kMaxEntries, m_nodes[node].count);
}

BBox SpatialIndex::nodeBox(NodeIndex node) const {
    BBox box;
    for (const Entry& entry : entries(node)) {
        box.expand(entry.box);
    }
    return box;
}

SpatialIndex::NodeIndex SpatialIndex::allocateNode(std::uint32_t level) {
    if (!m_free.empty()) {
        const NodeIndex node = m_free.back();
        m_free.pop_back();
        m_nodes[node] = Node{0, level};
        return node;
    }
    const auto node = static_cast<NodeIndex>(m_nodes.size());
    m_nodes.push_back(Node{0, level});
    m_entries.resize(m_entries.size() + kMaxEntries);
    return node;
}

void SpatialIndex::freeNode(NodeIndex node) {
    m_nodes[node].count = 0;
    m_free.push_back(node);
}

// ---------------------------------------------------------------------------
// Insertion

/// Inserts `entry` in a node of the given level (0 for entity boxes, higher
/// for the subtrees of dissolved nodes being reinserted).
void SpatialIndex::insertAtLevel(const Entry& entry, std::uint32_t level) {
    if (!m_root) {
        m_root = allocateNode(0);
    }
    assert(m_nodes[*m_root].level >= level);
    FixedStack<PathStep, kMaxHeight> path;
    NodeIndex node = *m_root;
    while (m_nodes[node].level > level) {
        const std::size_t slot = chooseSubtree(node, entry.box);
        path.push(PathStep{node, slot});
        node = static_cast<NodeIndex>(entries(node)[slot].ref);
    }
    NodeIndex sibling = addEntry(node, entry);

    // Back up the path: refresh the box of the child just changed (a split
    // may have shrunk it, so it is recomputed, not expanded), then place the
    // sibling created by a split, which may split the parent in turn.
    while (!path.isEmpty()) {
        const PathStep step = path.pop();
        entries(step.node)[step.slot].box = nodeBox(node);
        if (sibling != kNoNode) {
            sibling = addEntry(step.node, Entry{nodeBox(sibling), sibling});
        }
        node = step.node;
    }
    if (sibling == kNoNode) {
        return;
    }
    // The root was split: the tree grows by one level.
    const NodeIndex oldRoot = node;  // the walk up ended on the root
    assert(m_nodes[oldRoot].level + 1 < kMaxHeight);
    const NodeIndex newRoot = allocateNode(m_nodes[oldRoot].level + 1);
    addEntry(newRoot, Entry{nodeBox(oldRoot), oldRoot});
    addEntry(newRoot, Entry{nodeBox(sibling), sibling});
    m_root = newRoot;
}

/// The child of `node` whose box grows least to take `box` (ties: the
/// smaller box).
std::size_t SpatialIndex::chooseSubtree(NodeIndex node, const BBox& box) const {
    const std::span<const Entry> children = entries(node);
    std::size_t best = 0;
    Size bestGrowth{};
    Size bestSize{};
    for (std::size_t i = 0; i < children.size(); ++i) {
        const Size growth = enlargement(children[i].box, box);
        const Size size = sizeOf(children[i].box);
        if (i == 0 || growth < bestGrowth || (!(bestGrowth < growth) && size < bestSize)) {
            best = i;
            bestGrowth = growth;
            bestSize = size;
        }
    }
    return best;
}

/// Adds `entry` to `node`, splitting it if full. Returns the new sibling of
/// `node` after a split, kNoNode otherwise.
SpatialIndex::NodeIndex SpatialIndex::addEntry(NodeIndex node, const Entry& entry) {
    if (m_nodes[node].count == kMaxEntries) {
        return split(node, entry);
    }
    ++m_nodes[node].count;
    entries(node).back() = entry;
    return kNoNode;
}

/// Guttman's quadratic split of the kMaxEntries entries of a full `node`
/// plus `extra`. The two groups go to `node` and to a new sibling at the
/// same level, which is returned.
SpatialIndex::NodeIndex SpatialIndex::split(NodeIndex node, const Entry& extra) {
    constexpr std::size_t kCount = kMaxEntries + 1;
    std::array<Entry, kCount> all{};
    const std::span<Entry> pool(all);
    std::ranges::copy(entries(node), pool.begin());
    pool.back() = extra;

    // Seeds: the pair that would waste the most space in one group.
    std::size_t seedA = 0;
    std::size_t seedB = 1;
    Size worst{-1.0, -1.0};
    for (std::size_t i = 0; i < kCount; ++i) {
        for (std::size_t j = i + 1; j < kCount; ++j) {
            const Size joined = sizeOf(unite(pool[i].box, pool[j].box));
            const Size a = sizeOf(pool[i].box);
            const Size b = sizeOf(pool[j].box);
            const Size waste{joined.area - a.area - b.area, joined.margin - a.margin - b.margin};
            if (worst < waste) {
                worst = waste;
                seedA = i;
                seedB = j;
            }
        }
    }

    std::array<std::uint8_t, kCount> groupStorage{};  // all kUnassigned
    const std::span<std::uint8_t> group(groupStorage);
    group[seedA] = kGroupA;
    group[seedB] = kGroupB;
    BBox boxA = pool[seedA].box;
    BBox boxB = pool[seedB].box;
    std::size_t countA = 1;
    std::size_t countB = 1;
    std::size_t remaining = kCount - 2;

    while (remaining > 0) {
        // A group that needs every remaining entry to reach the minimum
        // takes them all.
        std::uint8_t forced = kUnassigned;
        if (countA + remaining == kMinEntries) {
            forced = kGroupA;
        } else if (countB + remaining == kMinEntries) {
            forced = kGroupB;
        }
        if (forced != kUnassigned) {
            std::ranges::replace(group, kUnassigned, forced);
            break;
        }
        // Next: the entry with the strongest preference for one group.
        std::size_t next = 0;
        Size strongest{-1.0, -1.0};
        for (std::size_t i = 0; i < kCount; ++i) {
            if (group[i] != kUnassigned) {
                continue;
            }
            const Size toA = enlargement(boxA, pool[i].box);
            const Size toB = enlargement(boxB, pool[i].box);
            const Size preference{std::abs(toA.area - toB.area), std::abs(toA.margin - toB.margin)};
            if (strongest < preference) {
                strongest = preference;
                next = i;
            }
        }
        // To the group that grows least; ties: the smaller, then the one
        // with fewer entries.
        const Size toA = enlargement(boxA, pool[next].box);
        const Size toB = enlargement(boxB, pool[next].box);
        const Size sizeA = sizeOf(boxA);
        const Size sizeB = sizeOf(boxB);
        bool chooseA = countA <= countB;
        if (toA < toB || toB < toA) {
            chooseA = toA < toB;
        } else if (sizeA < sizeB || sizeB < sizeA) {
            chooseA = sizeA < sizeB;
        }
        if (chooseA) {
            group[next] = kGroupA;
            boxA.expand(pool[next].box);
            ++countA;
        } else {
            group[next] = kGroupB;
            boxB.expand(pool[next].box);
            ++countB;
        }
        --remaining;
    }

    const NodeIndex sibling = allocateNode(m_nodes[node].level);
    m_nodes[node].count = 0;
    for (std::size_t i = 0; i < kCount; ++i) {
        const NodeIndex target = group[i] == kGroupA ? node : sibling;
        ++m_nodes[target].count;
        entries(target).back() = pool[i];
    }
    return sibling;
}

// ---------------------------------------------------------------------------
// Erasing

void SpatialIndex::removeEntry(NodeIndex node, std::size_t index) {
    const std::span<Entry> items = entries(node);
    items[index] = items.back();  // order inside a node does not matter
    --m_nodes[node].count;
}

}  // namespace tg::core
