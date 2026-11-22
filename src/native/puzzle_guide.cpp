// Guide commands and completion refreshes submit owned navigation queries.
// Bevelle timing is evaluated from the copied scene on the preparation worker.
// See ../../Docs/features/puzzles/index.md#cloisters.
#include "puzzle_guide.hpp"
#include "puzzle_guide_owned.hpp"
#include "puzzle_guide_targets.hpp"
#include "navigation_observations.hpp"
#include "navigation_intents.hpp"
#include "utilities.hpp"
#include "nav_audio.hpp"
#include "nav_format.hpp"
#include "settings.hpp"
#include "pedestal.hpp"
#include "auto_walk.hpp"
#include <cmath>
namespace ffx {
void resetPuzzleGuide() {resetOwnedPuzzleGuide();}
std::wstring puzzleGuide(std::wstring_view action) {
    nav::Query query=nav::Query::GuideStep;
    if(action==L"next") query=nav::Query::GuideNext;else if(action==L"previous") query=nav::Query::GuidePrevious;
    else if(action==L"expected") query=nav::Query::GuideExpected;else if(action==L"path") {query=nav::Query::GuidePath;nav::invalidateNavigationIntents();cancelPedestal();clearNavRoute();stopAutoWalk();}
    nav::submitNavigationQuery(query,false,ownedPuzzleGuideRevision());return {};
}
void puzzleGuideCompleted() {if(ownedPuzzleGuideActive() && claimOwnedGuideRefresh()) if(!nav::submitNavigationQuery(nav::Query::GuideCompleted,false,ownedPuzzleGuideRevision())) releaseOwnedGuideRefresh();}
// Bevelle rail auto-turn refresh: runs before the guide is first asked.
void puzzleGuideRailRefresh() {if(claimOwnedGuideRefresh()) if(!nav::submitNavigationQuery(nav::Query::GuideCompleted,false,ownedPuzzleGuideRevision())) releaseOwnedGuideRefresh();}
void installPuzzleGuide() {} // Completion refresh already observes input windows.
}
