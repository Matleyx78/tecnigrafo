// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/entity_id.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace tg::core {

/// Dynamic R-tree of axis-aligned boxes, keyed by entity (docs/adr/0007).
/// Nodes live in a vector and refer to each other by index, so the index
/// is copied and moved with the default semantics.
///
/// Guttman's R-tree with quadratic split: every node but the root holds
/// between kMinEntries and kMaxEntries entries; erasing reinserts the
/// entries of the nodes left with too few.
class SpatialIndex {
public:
    static constexpr std::size_t kMaxEntries = 16;
    /// About 40% of kMaxEntries, the fill Guttman found to work best.
    static constexpr std::size_t kMinEntries = 6;

    /// Precondition: `id` is not in the index and `box` is not empty.
    void insert(EntityId id, const BBox& box);

    /// Removes `id`, inserted with `box`. Returns false, changing nothing,
    /// if it is not in the index.
    bool erase(EntityId id, const BBox& box);

    /// Appends to `out` every id whose box overlaps `area`, borders
    /// included, in no particular order.
    void query(const BBox& area, std::vector<EntityId>& out) const;

    /// Union of all boxes; empty if the index is empty.
    [[nodiscard]] BBox bounds() const;

    [[nodiscard]] std::size_t size() const { return m_size; }

    [[nodiscard]] bool isEmpty() const { return m_size == 0; }

    void clear();

    /// For tests: every node's box is the union of its children, every
    /// node but the root holds between 6 and 16 entries, all leaves are at
    /// the same depth, size() matches the leaves.
    [[nodiscard]] bool checkInvariants() const;

private:
    using NodeIndex = std::uint32_t;

    /// "No node": what addEntry() returns when the node did not split.
    static constexpr NodeIndex kNoNode = std::numeric_limits<NodeIndex>::max();

    /// A box and what it refers to: an entity id in a leaf, the index of a
    /// child node in an inner node.
    struct Entry {
        BBox box;
        std::uint64_t ref{0};
    };

    /// The entries of node n are m_entries[n * kMaxEntries, n * kMaxEntries
    /// + count). Level 0 is a leaf; a node's children are one level lower.
    struct Node {
        std::uint32_t count{0};
        std::uint32_t level{0};
    };

    /// An entry taken out of a node that became too small while erasing,
    /// to be inserted again at the same level.
    struct Orphan {
        Entry entry;
        std::uint32_t level{0};
    };

    /// A node on the way from the root, and a slot among its entries.
    struct PathStep {
        NodeIndex node{0};
        std::size_t slot{0};
    };

    /// Upper bound on the number of levels. Every node but the root has at
    /// least kMinEntries children, so 32 levels would take more than 6^30
    /// entries: far more than memory can hold. The tree is walked with
    /// stacks of this depth instead of recursion.
    static constexpr std::size_t kMaxHeight = 32;

    [[nodiscard]] std::span<Entry> entries(NodeIndex node);
    [[nodiscard]] std::span<const Entry> entries(NodeIndex node) const;
    [[nodiscard]] BBox nodeBox(NodeIndex node) const;

    NodeIndex allocateNode(std::uint32_t level);
    void freeNode(NodeIndex node);

    void insertAtLevel(const Entry& entry, std::uint32_t level);
    [[nodiscard]] std::size_t chooseSubtree(NodeIndex node, const BBox& box) const;
    NodeIndex addEntry(NodeIndex node, const Entry& entry);
    NodeIndex split(NodeIndex node, const Entry& extra);
    void removeEntry(NodeIndex node, std::size_t index);

    std::vector<Node> m_nodes;
    std::vector<Entry> m_entries;   ///< kMaxEntries slots per node
    std::vector<NodeIndex> m_free;  ///< nodes to reuse
    std::optional<NodeIndex> m_root;
    std::size_t m_size{0};
};

}  // namespace tg::core
