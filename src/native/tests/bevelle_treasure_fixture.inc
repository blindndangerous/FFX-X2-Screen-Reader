namespace {
void bevelleDirectCollectionFixture(const TrialUtility& t) {
    resetPuzzleSolver();load(t,t.collect);
    setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);
    check(trialCommand(L"collect").find(L"opening")!=std::wstring::npos && requests==1 &&
        initPc==t.collect.entryPc,"direct Bevelle utility starts its original reward");
    setValue(0x01000a70,1,0x20|t.mask); // Modeled native W16 reward completion.
    complete(1);
    // Retained W54:6 reaches the sole REQEW W16:8 after native rail setup.
    // Global slot 2 identifies W54 regardless of its local worker number.
    put(controller,0,uint8_t{0});put(controller,0xc,uint16_t{3});
    put(helper,4,ptr(chunk));put(helper,0x2e,uint16_t{54});
    put(helper,0x90,ptr(helperSignal));put(helperSignal,8,uint16_t{6});
    put(target,0x80,ptr(signal));put(target,0x90,ptr(signal));
    put(signal,8,uint16_t{8});put(signal,10,uint16_t{2});put(signal,14,uint8_t{3});
    const auto threadPc=ptr(target)+0x12c+3*0x4c+0x18;
    const auto code=ptr(chunk)+0x1000;
    put(helperSignal,14,uint8_t{2});
    put(helper,0x32,uint8_t{2});
    write<uintptr_t>(ptr(helper)+0x12c+2*0x4c+0x18,code+0x1cde2);
    write<uintptr_t>(threadPc,code+t.collect.entryPc);
    Event e;e.args[0]=ptr(target);e.args[1]=ptr(signal);fire(0x469040,e);
    check(read<uintptr_t>(threadPc)==code+t.collect.returnPc,
        "after direct collection the physical Bevelle crossing goes straight to the reward child RET");
    const auto before=writes;complete(1);
    check(read<uintptr_t>(ptr(helper)+0x90)==ptr(helperSignal) &&
        read<uint8_t>(ptr(controller))==0 && writes==before && requests==1,
        "the child return leaves native W54 progression and control ownership intact");
    puts("PASS: direct Bevelle collection followed by W54 crossing skips the grant body using the original script identity and retains the native caller.");
}
void finishTrialBonus(const TrialUtility& t) {
    if(std::string(t.script)!="bvyt1100") return;
    check(utilityBusy() && initPc==79849 && !utilityValue(utilityScript(),0x0c00002a,1),
        "Bevelle runs the original branch chest after destruction treasure, before marking it or leaving");
    const auto before=requests;complete(5);
    check(utilityValue(utilityScript(),0x0c00002a,1)==1 && requests==before,
        "Bevelle records the original caller marker only after the owned chest RET");
    pumpPuzzleSolver();
}
void bevelleTreasureFixtures() {
    const auto& t=trialUtilities[4];const auto& c=puzzleContracts[4];const auto& i=c.interactions.front();
    bevelleDirectCollectionFixture(t);
    const auto& end=puzzleCompletions[4];
    const auto begin=[&] {
        loadSolver(t,c,i);setValue(0x01000a70,1,t.mask);puzzleCommand(L"on");
        natural(i);complete(read<uint16_t>(ptr(target)+0x2e)==i.worker?1:3);
        pumpPuzzleSolver();pumpPuzzleSolver();
        check(initPc==end.bonus.pc && requests==1 && utilityBusy() && puzzleArmed(),
            "already collected destruction treasure still leads to the missing branch chest");
    };
    begin();const auto pendingWrites=writes;pumpPuzzleSolver();pumpPuzzleSolver();
    check(requests==1 && writes==pendingWrites && !warps && !utilityValue(utilityScript(),0x0c00002a,1),
        "pending branch chest dialogue cannot duplicate requests, mark collection early or advance the story");
    puzzleCommand(L"off");complete(5);pumpPuzzleSolver();
    check(utilityValue(utilityScript(),0x0c00002a,1)==1 && requests==1 && !warps && !puzzleArmed(),
        "Off lets the current chest finish and retain its collection marker without continuing to the exit");
    puzzleCommand(L"on");natural(i);complete(3);pumpPuzzleSolver();pumpPuzzleSolver();
    check(requests==2 && initPc==end.later.pc && !puzzleArmed(),
        "re-enabling after Off skips the collected chest and starts only the native exit");complete(4);

    begin();puzzleCompleted(ptr(bonusWorkerBytes),ptr(bonusSignalBytes),false,true);pumpPuzzleSolver();
    check(!puzzleArmed() && !utilityValue(utilityScript(),0x0c00002a,1) && requests==1 && !warps,
        "interrupted branch chest never records collection or proceeds to story completion");
    loadSolver(t,c,i);setValue(0x01000a70,1,t.mask);setValue(0x0c00002a,1,1);
    puzzleCommand(L"on");natural(i);complete(3);pumpPuzzleSolver();pumpPuzzleSolver();
    check(requests==1 && initPc==end.later.pc,"both previously opened chests are skipped before the native exit");complete(4);

    begin();const auto beforeEpoch=writes;++epoch;updateUtilityContext();
    puzzleCompleted(ptr(bonusWorkerBytes),ptr(bonusSignalBytes),true,true);pumpPuzzleSolver();
    check(writes==beforeEpoch && !utilityValue(utilityScript(),0x0c00002a,1) && requests==1 && !warps,
        "generation change retires the pending chest without a stale marker or exit");resetPuzzleSolver();
    begin();const auto beforeAllocation=writes;utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
    complete(5);pumpPuzzleSolver();
    check(writes==beforeAllocation && !utilityValue(utilityScript(),0x0c00002a,1) && requests==1 && !warps && !puzzleArmed(),
        "same-address replacement allocation cannot inherit an earlier chest completion marker");resetPuzzleSolver();
    begin();puzzleCompleted(ptr(target),ptr(signal),true,true);puzzleCompleted(ptr(bonusWorkerBytes),ptr(physicalSignal),true,true);
    check(!utilityValue(utilityScript(),0x0c00002a,1) && requests==1 && utilityBusy(),
        "another worker or signal cannot acknowledge the pending branch chest");finishTrialBonus(t);complete(4);
    puts("PASS: Bevelle destruction treasure then original optional chest then exit; collected-state skips, owned RET marker, queued dialogue, Off/re-enable, interruption, generation/allocation and worker/signal guards; native item grant/UI effects remain simulated.");
}
}
