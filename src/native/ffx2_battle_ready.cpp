#include "ffx2_battle_ready.hpp"

namespace ffx {
void Ffx2BattleReadyCue::reset() {
    members_ = {};
    memberCount_ = 0;
    epoch_ = 0;
    inBattle_ = false;
}

std::vector<std::wstring> Ffx2BattleReadyCue::update(const Ffx2BattleReadySnapshot& snapshot) {
    if (!snapshot.mainStory || !snapshot.battleActive) {
        reset();
        return {};
    }
    if (inBattle_ && epoch_ != snapshot.epoch) reset();

    if (snapshot.partyCount > ffx2BattleReadyPartyCapacity) return {};
    for (std::size_t i = 0; i < snapshot.partyCount; ++i) {
        const auto identity = snapshot.party[i].identity;
        if (!identity) continue;
        for (std::size_t j = 0; j < i; ++j)
            if (snapshot.party[j].identity == identity) return {};
    }

    if (!inBattle_) {
        members_ = {};
        memberCount_ = 0;
        epoch_ = snapshot.epoch;
        inBattle_ = true;
    }

    std::array<MemberState, ffx2BattleReadyPartyCapacity> next{};
    std::size_t nextCount = 0;
    std::vector<std::wstring> names;
    for (std::size_t i = 0; i < snapshot.partyCount; ++i) {
        const auto& member = snapshot.party[i];
        if (!member.identity) continue;

        bool wasReady = false;
        bool announced = false;
        for (std::size_t j = 0; j < memberCount_; ++j) {
            if (members_[j].identity == member.identity) {
                wasReady = members_[j].ready;
                announced = members_[j].announced;
                break;
            }
        }

        auto& state = next[nextCount++];
        state.identity = member.identity;
        state.ready = member.ready;
        state.announced = member.ready && wasReady && announced;
        if (member.ready && !state.announced && !member.displayedName.empty()) {
            names.push_back(member.displayedName);
            state.announced = true;
        }
    }
    members_ = next;
    memberCount_ = nextCount;
    return names;
}
}
