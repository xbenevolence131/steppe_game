# Settled Faction AI Notes

Settled factions should organize their military as groups rather than operate as loose individual units. Their basic behavior should be to defend infrastructure first, then project force through field armies.

## Core Roles

### Garrison Pool

The AI should maintain a pool of units suitable for fixed defense. Infantry and militia are the only current generated-garrison candidates; cavalry and other mounted units should remain available for mobile field armies, reserves, and raids.

Generated garrisons are currently assigned to:

- owned cities and towns
- Great Wall gates and passes
- supply-source towns when they are represented by owned urban hexes
- later: caravan depots, bridgeheads, and farmland districts

The current implementation builds owned defensive objectives first, then assigns available infantry-class units from the garrison pool. A unit already on its objective receives a `hold_hex` directive; a unit assigned from elsewhere receives a `defend_hex` directive until it reaches the post.

Manual AI groups currently take precedence over generated groups. This means a unit in a manual field-army group will not be peeled off into a generated garrison, even if it is infantry on a defensive objective. That behavior should probably be narrowed so only manual `garrison` groups can lock down defensive infantry.

### Field Armies

Field armies are mobile operational groups. They should be used for:

- offensive excursions into hostile territory
- hunting hostile intrusions inside or near settled territory
- recapturing lost towns
- relieving threatened garrisons
- later: escorting caravans

Field armies should have an anchor, target, and group-level directive. Individual units can still execute tactical moves, but their objective should come from the group.

### Reserve

Reserve is a flexible role for units not currently committed to fixed garrison or field-army tasks. It should fill uncovered defensive requirements, reinforce threatened objectives, and eventually rotate into field armies when enough fixed defense exists.

Reserve may become more of an assignment state than a long-term group type once garrison planning is more complete.

## Assignment Flow

1. Build a list of owned defensive objectives.
2. Score and deduplicate those objectives.
3. Assign infantry-class garrison units from the garrison pool.
4. Group remaining mobile units into field armies.
5. Assign field-army directives from the strategic situation.

## Objective Scoring

Defended objectives should be prioritized by value and threat.

Higher priority:

- city or major town
- supply source
- Great Wall gate/pass on the enclosed settled side of the wall
- currently threatened objective
- objective without a defender

Lower priority:

- minor town
- already covered objective
- objective far from current hostile activity

Possible baseline targets:

- major city: one or two defenders
- wall gate/pass: one defender
- threatened city or pass: add one defender if available

## Disposition Use

The simplified diplomacy score should drive AI hostility.

Suggested interpretation:

- `0-15`: deeply hostile, prefer raiding/horde hunting/offensive action
- `16-30`: hostile, form field armies and capture targets
- `31-35`: unfriendly, stay defensive/reserve-heavy
- `36-64`: neutral, avoid aggression
- `65-100`: friendly, avoid conflict and later allow cooperation

Settled factions should not attack, capture, or raid non-hostile factions except under explicit future rules.

## Implementation Direction

Generated AI groups should continue to be regenerated from current state for now, while preserving manually authored groups.

Near-term changes:

- replace "unit already on city/pass" garrison detection with explicit garrison requirements
- assign suitable units to those requirements
- move assigned garrison units toward their objective
- form field armies from remaining mobile units
- use disposition bands to choose reserve, field army, or raiding-force behavior

Longer-term changes:

- persist field-army identity and target to reduce jitter
- support caravan escort groups
- add farmland/road corridor defense objectives
- add production/recruitment priorities based on garrison shortages
