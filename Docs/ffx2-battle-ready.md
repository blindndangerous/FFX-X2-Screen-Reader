# FFX-2 main-story battle ready cue

The chosen cue is the displayed character name alone: for example, `Yuna`.
There is no sound, "ready" suffix, gauge percentage or command name.

`src/native/ffx2_battle_ready.*` implements the preparation-worker state machine.
It consumes an owned snapshot containing the main-story/battle flags, battle
generation, and at most three members. Each member has a stable opaque identity,
native command readiness, and an owned displayed name. No fixed English name
table substitutes for the actual displayed name.

The first ready observation emits once. Held readiness is silent. A not-ready
observation rearms that identity. Party slot changes do not retrigger a cue;
different ready members emit in their supplied native order. An unknown identity
or empty name is silent. A name arriving while the member remains ready may emit
once. An identity that leaves the roster and returns starts a fresh observation.

Leaving battle, leaving main-story mode or changing generation clears readiness.
Duplicate nonzero identities and counts above three reject a snapshot. A rejected
snapshot preserves same-generation member state; a changed generation retires the
old state first. The future receiver must reject retired generations before
calling this worker helper, as the existing speech-domain receiver already does.

No battle hook or game address is installed. The following are still
**UNVERIFIED / unresolved**, and no RVA or offset is invented for them:

- The owner callback where a main-story character can accept a command.
- The ready transition, stable actor identity and complete roster lifetime.
- The native displayed-name source and reveal/availability gates.
- Which native event rearms readiness after an action, including interruption,
  incapacitation, swapping, menu suspension and rapid action changes.

The future native hook may copy bounded POD and encoded name bytes only, using
the same ownership and generation contract as the menu capture. It must not
construct this helper's strings or vectors on the game thread. Preparation owns
decoding, identity tracking and output. Queue pressure must drop safely; it must
never cause synchronous native speech or a wait. Readiness must come from native
command state, not an inferred ATB threshold or a selected command cursor.

The synthetic fixture covers first/held/new edges, slot order, simultaneous
members, late names, malformed snapshots and context resets. It does not prove
any battle address, actual speech, audio absence in game, or gameplay timing.
Future nonvisual checks are in [the shared checklist](ffx2-ingame-checklist.md).
