// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document_query.hpp"
#include "tg/edit/editor.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using tg::core::BBox;
using tg::core::Document;
using tg::core::EntityId;
using tg::core::Polyline;
using tg::core::RectangleMode;
using tg::core::Vec2;
using Clock = std::chrono::steady_clock;

// Workload and acceptance threshold specified by stage 0.3, package P7.
constexpr std::size_t kEntityCount = 100'000;
constexpr std::size_t kRepetitions = 10;
constexpr std::size_t kQueryCount = 1'000;
constexpr std::size_t kEraseCount = 10'000;
constexpr double kWindowLimitMs = 100.0;

// 400 by 250 cells, each 20 drawing units wide/high; shapes remain disjoint.
constexpr std::size_t kColumns = 400;
constexpr double kCellSize = 20.0;
constexpr double kMinSide = 8.0;
constexpr double kSideVariation = 8.0;
constexpr double kCornerRadius = 2.0;
// Equivalent to 3 px picking and 10 px snapping at 10 px per drawing unit.
constexpr double kPickTolerance = 0.3;
constexpr double kSnapAperture = 1.0;
constexpr std::uint32_t kSeed = 0xC0FFEEU;

using Samples = std::array<double, kRepetitions>;

struct Measurements {
    Samples construction{};
    std::array<Samples, 3> window{};
    std::array<Samples, 3> crossing{};
    Samples pick{};
    Samples snap{};
    Samples erase{};
    Samples undo{};
};

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

// The engine's integer sequence is portable; avoid implementation-dependent
// real distributions so every platform measures the same drawing and queries.
double unitRandom(std::mt19937& generator) {
    return static_cast<double>(generator()) / static_cast<double>(std::mt19937::max());
}

std::vector<Polyline> makeGeometry(std::mt19937& generator) {
    std::vector<Polyline> geometry;
    geometry.reserve(kEntityCount);
    // A quarter circle has bulge tan(pi/8) = sqrt(2) - 1.
    constexpr double kQuarterBulge = std::numbers::sqrt2 - 1.0;
    for (std::size_t i = 0; i < kEntityCount; ++i) {
        const double x = static_cast<double>(i % kColumns) * kCellSize;
        const std::size_t row = i / kColumns;
        const double y = static_cast<double>(row) * kCellSize;
        const double width = kMinSide + kSideVariation * unitRandom(generator);
        const double height = kMinSide + kSideVariation * unitRandom(generator);
        Polyline shape;
        // Counter-clockwise rectangle with rounded bottom-right and top-left corners.
        shape.addVertex({x, y});
        shape.addVertex({x + width - kCornerRadius, y}, kQuarterBulge);
        shape.addVertex({x + width, y + kCornerRadius});
        shape.addVertex({x + width, y + height});
        shape.addVertex({x + kCornerRadius, y + height}, kQuarterBulge);
        shape.addVertex({x, y + height - kCornerRadius});
        shape.setClosed(true);
        geometry.push_back(std::move(shape));
    }
    return geometry;
}

Document makeDocument(const std::vector<Polyline>& geometry) {
    Document document;
    // Fixture construction measures Document::add and its spatial-index updates;
    // editing and undo are measured separately through Editor below.
    for (const Polyline& shape : geometry) {
        document.add(shape);
    }
    return document;
}

std::vector<Vec2> makeQueries(const BBox& bounds, std::mt19937& generator) {
    std::vector<Vec2> points;
    points.reserve(kQueryCount);
    for (std::size_t i = 0; i < kQueryCount; ++i) {
        const double x = bounds.min().x + bounds.width() * unitRandom(generator);
        const double y = bounds.min().y + bounds.height() * unitRandom(generator);
        points.emplace_back(x, y);
    }
    return points;
}

std::array<BBox, 3> selectionAreas(const BBox& bounds) {
    const Vec2 center = (bounds.min() + bounds.max()) * 0.5;
    // Linear fractions 0.1 and 0.5 give 1% and 25% of the bounding-box area.
    const Vec2 extent{bounds.width(), bounds.height()};
    return {BBox{center - extent * 0.05, center + extent * 0.05},
            BBox{center - extent * 0.25, center + extent * 0.25}, bounds};
}

template <typename Operation>
double milliseconds(Operation&& operation) {
    const auto start = Clock::now();
    std::forward<Operation>(operation)();
    const auto end = Clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double median(Samples values) {
    std::ranges::sort(values);
    // Ten samples: the median is the mean of the two central values.
    return (values.at(kRepetitions / 2 - 1) + values.at(kRepetitions / 2)) * 0.5;
}

void measureSelections(const Document& document, const std::array<BBox, 3>& areas,
                       std::size_t repetition, Measurements& measurements,
                       std::uint64_t& checksum) {
    for (std::size_t area = 0; area < areas.size(); ++area) {
        std::vector<EntityId> window;
        std::vector<EntityId> crossing;
        measurements.window.at(area).at(repetition) = milliseconds([&] {
            window = tg::core::selectInRectangle(document, areas.at(area), RectangleMode::Window);
        });
        measurements.crossing.at(area).at(repetition) = milliseconds([&] {
            crossing =
                tg::core::selectInRectangle(document, areas.at(area), RectangleMode::Crossing);
        });
        require(!window.empty() && crossing.size() >= window.size(), "Invalid rectangle results");
        if (area == areas.size() - 1) {
            require(window.size() == kEntityCount && crossing.size() == kEntityCount,
                    "Full rectangle did not select every entity");
        }
        // Consume results outside the timer, including ids rather than only sizes.
        for (const EntityId id : window) {
            checksum += id.value;
        }
        for (const EntityId id : crossing) {
            checksum += id.value;
        }
    }
}

void measureQueries(const Document& document, const std::vector<Vec2>& points,
                    std::size_t repetition, Measurements& measurements, std::uint64_t& checksum) {
    std::array<std::optional<EntityId>, kQueryCount> picks;
    std::array<std::optional<tg::core::SnapPoint>, kQueryCount> snaps;
    measurements.pick.at(repetition) = milliseconds([&] {
        for (std::size_t i = 0; i < points.size(); ++i) {
            picks.at(i) = tg::core::pickEntity(document, points[i], kPickTolerance);
        }
    });
    measurements.snap.at(repetition) = milliseconds([&] {
        for (std::size_t i = 0; i < points.size(); ++i) {
            snaps.at(i) = tg::core::findSnap(document, points[i], kSnapAperture, {});
        }
    });
    std::size_t pickHits = 0;
    std::size_t snapHits = 0;
    for (const auto& pick : picks) {
        if (pick) {
            ++pickHits;
            checksum += pick->value;
        }
    }
    for (const auto& snap : snaps) {
        if (snap) {
            ++snapHits;
            checksum += static_cast<std::uint64_t>(snap->kind) + 1;
        }
    }
    require(pickHits > 0 && pickHits < points.size(), "Picking needs both hits and misses");
    require(snapHits > 0 && snapHits < points.size(), "Snapping needs both hits and misses");
}

void measureEditing(const Document& baseline, std::size_t repetition, Measurements& measurements) {
    // Copy outside the timer: undo appends restored entities, so reusing the
    // document would silently change removal cost on subsequent repetitions.
    Document document = baseline;
    tg::edit::Editor editor(document);
    std::vector<EntityId> ids;
    ids.reserve(kEraseCount);
    // Remove evenly spaced entities in drawing order, not only cheap tail entries.
    for (std::size_t i = 0; i < kEraseCount; ++i) {
        ids.push_back(baseline.entities()[i * (kEntityCount / kEraseCount)].id);
    }
    std::size_t removed = 0;
    measurements.erase.at(repetition) = milliseconds([&] {
        editor.beginTransaction("Benchmark erase");
        for (const EntityId id : ids) {
            if (editor.remove(id)) {
                ++removed;
            }
        }
        editor.commit();
    });
    require(removed == kEraseCount && document.entityCount() == kEntityCount - kEraseCount,
            "Deletion did not remove the expected entities");
    for (const EntityId id : ids) {
        require(document.find(id) == nullptr, "Deleted entity still exists");
    }
    bool undone = false;
    measurements.undo.at(repetition) = milliseconds([&] { undone = editor.undo(); });
    require(undone && document.entityCount() == kEntityCount && !editor.canUndo(),
            "Deletion was not restored by a single undo");
    for (const EntityId id : ids) {
        const auto* restored = document.find(id);
        require(restored != nullptr && *restored == *baseline.find(id),
                "Undo did not restore the original id and geometry");
    }
}

std::string_view systemName() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "Other";
#endif
}

void printRow(std::string_view name, const Samples& samples) {
    std::cout << std::left << std::setw(34) << name << std::right << std::setw(14)
              << median(samples) << '\n';
}

int run() {
    std::cout << "tg_bench | System: " << systemName() << " | Build: " << TG_BENCH_BUILD_TYPE
              << "\n100000 closed polylines; 10 repetitions; median milliseconds\n"
              << "Seed: " << kSeed << "; grid: 400 x 250; cell: 20; sides: [8,16]; radius: 2\n"
              << "Areas: centred 1%, 25%, 100% of drawing bounds\n"
              << "Random queries: 1000 per batch; pick tolerance: 0.3; snap aperture: 1\n"
              << "Deletion: 10000 evenly spaced ids in ascending drawing order\n";
#ifndef NDEBUG
    std::cout << "WARNING: Debug timings do not qualify for the 100 ms criterion.\n";
#endif
    // Fixed seed is required for reproducible benchmarks, not used for security.
    // NOLINTNEXTLINE(cert-msc32-c,cert-msc51-cpp)
    std::mt19937 generator{kSeed};
    const auto geometry = makeGeometry(generator);
    const Document baseline = makeDocument(geometry);
    const auto areas = selectionAreas(baseline.extents());
    const auto points = makeQueries(baseline.extents(), generator);
    Measurements measurements;
    std::uint64_t checksum = 0;
    for (std::size_t repetition = 0; repetition < kRepetitions; ++repetition) {
        Document constructed;
        measurements.construction.at(repetition) =
            milliseconds([&] { constructed = makeDocument(geometry); });
        require(constructed.entityCount() == kEntityCount, "Construction count mismatch");
        checksum += constructed.entityCount();
        measureSelections(baseline, areas, repetition, measurements, checksum);
        measureQueries(baseline, points, repetition, measurements, checksum);
        measureEditing(baseline, repetition, measurements);
        std::cerr << "Completed repetition " << repetition + 1 << '/' << kRepetitions << '\n';
    }
    std::cout << '\n' << std::fixed << std::setprecision(3);
    std::cout << std::left << std::setw(34) << "Operation" << std::right << std::setw(14)
              << "Median (ms)" << '\n';
    printRow("Document construction", measurements.construction);
    constexpr std::array kWindowNames{"Window 1%", "Window 25%", "Window 100%"};
    constexpr std::array kCrossingNames{"Crossing 1%", "Crossing 25%", "Crossing 100%"};
    for (std::size_t i = 0; i < kWindowNames.size(); ++i) {
        printRow(kWindowNames.at(i), measurements.window.at(i));
        printRow(kCrossingNames.at(i), measurements.crossing.at(i));
    }
    printRow("pickEntity (1000 calls)", measurements.pick);
    printRow("findSnap (1000 calls)", measurements.snap);
    printRow("Erase 10000 + commit", measurements.erase);
    printRow("Undo erase", measurements.undo);
    std::cout << "Checksum: " << checksum << '\n';
    const bool passed = median(measurements.window.back()) <= kWindowLimitMs;
    std::cout << "Window 100% <= 100 ms: " << (passed ? "PASS" : "FAIL") << '\n';
    return passed ? 0 : 1;
}

}  // namespace

int main() {
    try {
        return run();
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 2;
    }
}
