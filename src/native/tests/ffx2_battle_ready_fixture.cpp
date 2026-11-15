#include "../ffx2_battle_ready.hpp"

#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <vector>

namespace {
std::size_t checks = 0;
std::size_t failures = 0;

void require(bool ok, const char* why) {
    ++checks;
    if (!ok) {
        ++failures;
        std::fprintf(stderr, "FAIL: checks=%zu failures=%zu: %s\n", checks, failures, why);
        std::exit(1);
    }
}

ffx::Ffx2BattleReadyMember member(std::uint64_t identity, bool ready, const wchar_t* name) {
    return {identity, ready, name};
}

ffx::Ffx2BattleReadySnapshot snapshot(std::uint64_t epoch,
                                      std::initializer_list<ffx::Ffx2BattleReadyMember> members) {
    ffx::Ffx2BattleReadySnapshot result{};
    result.mainStory = true;
    result.battleActive = true;
    result.epoch = epoch;
    result.partyCount = members.size();
    std::size_t index = 0;
    for (const auto& item : members) result.party[index++] = item;
    return result;
}

void expectNames(const std::vector<std::wstring>& actual,
                 std::initializer_list<const wchar_t*> expected,
                 const char* why) {
    require(actual.size() == expected.size(), why);
    std::size_t index = 0;
    for (const auto* name : expected) require(actual[index++] == name, why);
}

void firstReadyAndStableIdentity() {
    ffx::Ffx2BattleReadyCue cue;
    auto state = snapshot(1, {member(10, true, L"Yuna"), member(20, false, L"Rikku")});
    expectNames(cue.update(state), {L"Yuna"}, "first ready observation announces displayed name");
    expectNames(cue.update(state), {}, "held ready state does not repeat");
    state = snapshot(1, {member(20, false, L"Rikku"), member(10, true, L"Yuna")});
    expectNames(cue.update(state), {}, "party reordering does not retrigger");
    state.party[1].ready = false;
    expectNames(cue.update(state), {}, "not-ready observation only rearms");
    state.party[1].ready = true;
    expectNames(cue.update(state), {L"Yuna"}, "new ready edge announces again");
}

void orderedNamesAndEpochReset() {
    ffx::Ffx2BattleReadyCue cue;
    expectNames(cue.update(snapshot(7, {member(1, true, L"Yuna")})), {L"Yuna"}, "initial epoch cue");
    expectNames(cue.update(snapshot(8, {member(2, true, L"Paine"), member(1, true, L"Yuna")})),
                {L"Paine", L"Yuna"}, "epoch change resets and preserves native party order");
}

void unknownIdentityAndLateName() {
    ffx::Ffx2BattleReadyCue cue;
    expectNames(cue.update(snapshot(1, {member(0, true, L"Unknown")})), {}, "unknown identity is suppressed");
    expectNames(cue.update(snapshot(1, {member(3, true, L"")})), {}, "empty displayed name is suppressed");
    expectNames(cue.update(snapshot(1, {member(3, true, L"Paine")})), {L"Paine"},
                "name arriving while ready can announce once");
    expectNames(cue.update(snapshot(1, {member(3, true, L"Paine")})), {}, "late-name cue stays one-shot");
}

void invalidSnapshotsDoNotChangeState() {
    ffx::Ffx2BattleReadyCue cue;
    expectNames(cue.update(snapshot(1, {member(4, true, L"Yuna")})), {L"Yuna"}, "seed announced state");
    auto duplicate = snapshot(1, {member(4, false, L"Yuna"), member(4, true, L"Yuna")});
    expectNames(cue.update(duplicate), {}, "duplicate identity snapshot rejected");
    expectNames(cue.update(snapshot(1, {member(4, true, L"Yuna")})), {},
                "duplicate rejection preserves prior readiness state");

    auto oversized = snapshot(1, {member(4, false, L"Yuna")});
    oversized.partyCount = 4;
    expectNames(cue.update(oversized), {}, "oversized snapshot rejected");
    expectNames(cue.update(snapshot(1, {member(4, true, L"Yuna")})), {},
                "oversized rejection preserves prior readiness state");
}

void contextExitResetsState() {
    ffx::Ffx2BattleReadyCue cue;
    expectNames(cue.update(snapshot(1, {member(5, true, L"Rikku")})), {L"Rikku"}, "battle cue");
    auto outsideBattle = snapshot(1, {member(5, true, L"Rikku")});
    outsideBattle.battleActive = false;
    expectNames(cue.update(outsideBattle), {}, "leaving battle clears without cue");
    expectNames(cue.update(snapshot(1, {member(5, true, L"Rikku")})), {L"Rikku"},
                "battle reentry announces ready member");

    auto nonMainStory = snapshot(1, {member(5, true, L"Rikku")});
    nonMainStory.mainStory = false;
    expectNames(cue.update(nonMainStory), {}, "non-main story clears without cue");
    expectNames(cue.update(snapshot(1, {member(5, true, L"Rikku")})), {L"Rikku"},
                "main-story reentry announces ready member");
}

void absentMemberReentryStartsNewEdge() {
    ffx::Ffx2BattleReadyCue cue;
    expectNames(cue.update(snapshot(1, {member(6, true, L"Yuna")})), {L"Yuna"}, "initial member cue");
    expectNames(cue.update(snapshot(1, {})), {}, "empty party snapshot clears roster");
    expectNames(cue.update(snapshot(1, {member(6, true, L"Yuna")})), {L"Yuna"},
                "member reentry starts a fresh ready edge");
}
}

int main() {
    firstReadyAndStableIdentity();
    orderedNamesAndEpochReset();
    unknownIdentityAndLateName();
    invalidSnapshotsDoNotChangeState();
    contextExitResetsState();
    absentMemberReentryStartsNewEdge();
    std::printf("PASS: checks=%zu failures=%zu; worker-side FFX-2 battle ready edges, stable identities, owned names, ordering and resets.\n",
                checks, failures);
}
