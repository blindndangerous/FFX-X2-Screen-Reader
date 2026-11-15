#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ffx {
constexpr std::size_t ffx2BattleReadyPartyCapacity = 3;

struct Ffx2BattleReadyMember {
    // Stable opaque actor key; zero means that identity is unavailable.
    std::uint64_t identity = 0;
    // Copied native command readiness; never inferred from an ATB threshold.
    bool ready = false;
    std::wstring displayedName;
};

struct Ffx2BattleReadySnapshot {
    bool mainStory = false;
    bool battleActive = false;
    std::uint64_t epoch = 0;
    std::size_t partyCount = 0;
    std::array<Ffx2BattleReadyMember, ffx2BattleReadyPartyCapacity> party{};
};

class Ffx2BattleReadyCue {
public:
    // Worker-only. Invalid snapshots in the same active epoch leave member state unchanged.
    std::vector<std::wstring> update(const Ffx2BattleReadySnapshot& snapshot);

private:
    struct MemberState {
        std::uint64_t identity = 0;
        bool ready = false;
        bool announced = false;
    };

    void reset();

    std::array<MemberState, ffx2BattleReadyPartyCapacity> members_{};
    std::size_t memberCount_ = 0;
    std::uint64_t epoch_ = 0;
    bool inBattle_ = false;
};
}
