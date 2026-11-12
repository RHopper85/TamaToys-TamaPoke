# TamaToys TamaPoke roadmap

## Phase 1: Wi-Fi settings

- [x] Settings hub that keeps the existing time, sound and language controls.
- [x] On-device Wi-Fi On/Off, auto-connect, scan, saved networks and forget confirmation.
- [x] Case-sensitive touchscreen password keyboard; no passwords in source or serial logs.
- [x] Separate home Wi-Fi manager from the existing ESP-NOW LAN radio.
- [x] Emulator sample networks and a headless UI regression test.
- [ ] Verify an actual WPA connection, reconnect and saved credentials on Waveshare hardware.
- [ ] Test LAN battles before and after an internet Wi-Fi connection on two boards.
- [ ] Localize the new Wi-Fi UI into the six supported languages.

## Phase 2: Player profiles and friends

- [x] Prompt for trainer name after first starter choice or Factory Reset; New Game preserves the previous name.
- [x] Implement automatic registration via HTTPS on first Wi-Fi connection (hardware test pending).
- [x] Assign a server-unique friend code and a separate secret device token; store token only on device and hashed on server.
- [x] Friends menu interface on Player screen: pending code, code-entry keyboard, lists, request and removal-confirmation screens; emulator-only demo interactions.
- [ ] Wire those interfaces to the real authenticated API (requests, codes, last-seen and challenges).
- [ ] Friends-only last-seen information; allow players to opt out.
- [x] New Game clears pet progress but preserves name, online token/code/friendships and saved Wi-Fi.
- [x] Factory Reset confirms Yes/No, deletes the server trainer first (Wi-Fi required for an existing online token), then clears game, device identity and Wi-Fi.
- [ ] Verify automatic registration and Factory Reset against a real ESP32 over Wi-Fi with valid TLS certificate and system time.

## Phase 3: Asynchronous internet battles

- [ ] Server snapshots both teams when a battle begins.
- [ ] Each player submits hidden moves; resolve a turn only after both arrive.
- [ ] One new challenge per day, seven-day inactivity expiration.
- [ ] Server-authoritative move resolution with parity tests against the existing battle engine.
- [ ] Preserve current LAN and offline modes.

## Phase 4: Custom Pokémon habitats

**Goal:** Allow players to customize their Pokémon's existing living space with furniture, decorations, toys and rare collectibles without replacing the current TamaPoke home-screen layout.

### Habitat system

- [ ] Extend the existing home environment with a lightweight decoration-placement system.
- [ ] Retain the existing Pokémon movement, animations, backgrounds and care interactions.
- [ ] Allow players to enter a dedicated Decorate/Edit mode from the home interface.
- [ ] Allow purchased or discovered decorations to be placed, moved, removed and rearranged.
- [ ] Use predefined placement positions or zones appropriate for the 466×466 round AMOLED display.
- [ ] Keep decorations within visible screen boundaries without obstructing Pokémon animations or essential UI elements.
- [ ] Add object placement, layering and collision rules where necessary.
- [ ] Store each player's active decorations and positions in persistent save data.
- [ ] Ensure decorations survive device restarts and normal save/load operations.
- [ ] Define appropriate decoration behavior for New Game and Factory Reset.
- [ ] Support future expansion without requiring a complete redesign of the habitat system.

### Decoration categories and placement limits

Start with these limits to prevent clutter:

| Category | Initial limit |
|---|---:|
| Bed / Rest | 1 |
| Food / Care | 1 |
| Toys | 2 |
| Nature | 2 |
| Furniture | 2 |
| Wall / Background Accents | 2 |
| Special / Rare | 1 |

- [ ] Enforce category-specific placement limits.
- [ ] Support independent ownership and placement limits.
- [ ] Allow owned decorations to be swapped without repurchasing them.
- [ ] Prevent invalid placement and overlapping decorations.
- [ ] Keep room composition visually balanced and the Pokémon clearly visible.
- [ ] Permit future placement-capacity upgrades only if display space allows.

### Starting decorations

Provide these items to new players at no cost:

- Simple Bed
- Food Bowl
- Small Cushion
- House Plant
- Wooden Crate
- Basic Toy Ball
- Small Wall Poster

- [ ] Add starter decorations to the initial player inventory.
- [ ] Create a simple default habitat layout using a subset of these items.
- [ ] Allow players to remove or rearrange all starting decorations.

### Poké Mart — Decor category

- [ ] Add a dedicated Decor category to the existing Poké Mart.
- [ ] Integrate decoration purchases with the existing Pokédollar economy.
- [ ] Organize decorations by category and availability.
- [ ] Display item previews, descriptions and purchase prices.
- [ ] Indicate whether an item is already owned.
- [ ] Prevent unnecessary duplicate purchases for unique decorations.
- [ ] Store purchased decorations permanently in the player's decoration inventory.
- [ ] Preserve compatibility with existing shop items and consumables.

### Decoration catalog

Target an initial catalog of approximately 60 items, expandable in future updates.

**Furniture**
- Small Bed
- Deluxe Bed
- Beanbag Chair
- Wooden Table
- Couch
- Bookshelf
- Toy Chest
- Floor Lamp
- Rocking Chair
- Food Bowl

**Decorations**
- Potted Plant
- Flower Pot
- Wall Clock
- Picture Frame
- Round Rug
- Star Rug
- Poké Ball Cushion
- Curtains
- Aquarium
- Mini Fountain

**Nature**
- Oak Tree
- Pine Tree
- Berry Bush
- Flower Patch
- Mushroom Cluster
- Large Rock
- Tree Stump
- Log Bench
- Small Pond
- Wooden Fence

**Outdoor**
- Camping Tent
- Campfire
- Picnic Blanket
- Beach Umbrella
- Beach Chair
- Sandcastle
- Hammock
- Lantern
- Park Bench
- Birdbath

**Toys**
- Poké Ball Toy
- Rubber Ball
- Teddy Bear
- Building Blocks
- Jump Rope
- Toy Train
- Mini Slide
- Swing
- Scratch Post
- Bubble Machine

**Rare / Special**
- Crystal Lamp
- Golden Statue
- Meteorite
- Ancient Relic
- Glowing Mushroom
- Mini Waterfall
- Volcano Model
- Snow Globe
- Treasure Chest
- Mystery Egg Statue

Additional expedition-exclusive decorations may be introduced separately.

### Interactive decorations

- [ ] Support simple Pokémon interactions with selected decorations.
- [ ] Beds: Pokémon may sleep or rest.
- [ ] Toys: Pokémon may approach and play.
- [ ] Food bowls: Pokémon may approach during feeding animations.
- [ ] Cushions and chairs: Pokémon may sit or rest.
- [ ] Selected special decorations: optional visual effects or simple animations.
- [ ] Avoid excessive background processing or memory consumption.

## Phase 5: Pokémon expeditions

**Goal:** Introduce a passive adventure system that allows Pokémon to leave the player's habitat, explore destinations, and return with rewards, including rare decorations.

Expeditions must operate independently of internet connectivity.

### Core expedition system

- [ ] Add an Expeditions menu to the existing game interface.
- [ ] Allow players to select an eligible Pokémon from their party or available Pokémon storage.
- [ ] Allow players to select from unlocked expedition destinations.
- [ ] Show expedition duration, difficulty, recommended Pokémon types and possible general reward categories.
- [ ] Require confirmation before sending a Pokémon on an expedition.
- [ ] Mark the selected Pokémon as away and unavailable until the expedition resolves.
- [ ] Prevent an expedition Pokémon from being used in battles, trades, care activities or other incompatible actions.
- [ ] Do not allow expeditions to interrupt or replace the existing active Explore mode.
- [ ] Preserve the player's ability to care for another available Pokémon while an expedition is underway.

### No-recall expedition rules

**Design decision: Once dispatched, Pokémon cannot be recalled.**

- [ ] Do not provide a Recall or Cancel Expedition button after departure.
- [ ] Pokémon remain unavailable until their expedition ends.
- [ ] Expeditions resolve automatically into success, partial success or failure.
- [ ] All outcomes result in the Pokémon returning safely.
- [ ] Failure must not permanently delete or kill a Pokémon.
- [ ] Prevent Pokémon from becoming permanently stranded because of interrupted saves, firmware updates or power loss.
- [ ] Preserve expedition state across device restarts and power cycles.
- [ ] Include a recovery mechanism for corrupted or expired expedition state without introducing a player-accessible recall option.

### Initial expedition destinations

| Destination | Duration | Difficulty | Recommended types |
|---|---|---|---|
| Viridian Forest | 15 minutes | Easy | Grass / Bug |
| Rocky Cave | 30 minutes | Easy | Rock / Ground |
| Sunny Beach | 45 minutes | Medium | Water |
| Haunted Woods | 1 hour | Medium | Ghost / Dark |
| Crystal Cavern | 2 hours | Hard | Rock / Fairy |
| Snowpeak Mountain | 3 hours | Hard | Ice |
| Ancient Ruins | 4 hours | Very Hard | Psychic / Ground |

Durations and difficulty values are initial balancing targets.

- [ ] Implement destination definitions that can be expanded without changing the core expedition engine.
- [ ] Introduce destination unlocking through progression or achievements.
- [ ] Consider Pokémon type, level, friendship or other appropriate factors when calculating outcomes.
- [ ] Provide meaningful risk-versus-reward differences between destinations.
- [ ] Prevent repeated expedition abuse through save manipulation where reasonably practical.

### Expedition outcomes

**Success**
- Pokémon returns with normal or enhanced rewards.
- Chance to discover decorations, including rare items.
- Possible Pokédollar, item and friendship rewards.

**Partial success**
- Pokémon returns with reduced rewards.
- Lower chance of finding rare decorations.
- Possible mild fatigue or mood reduction.

**Failure**
- Pokémon returns safely.
- Little or no reward.
- Possible temporary fatigue, energy or mood penalty.
- No permanent Pokémon loss.

- [ ] Implement configurable success probabilities and reward tables.
- [ ] Randomize expedition results at an appropriate point without allowing easy reroll exploits.
- [ ] Create a return notification and expedition summary.
- [ ] Display discovered items and rewards after expedition completion.
- [ ] Add rewards to the appropriate inventory without duplication.
- [ ] Support offline expedition progression using the existing RTC/time infrastructure, subject to hardware verification.

### Expedition reward categories

- Pokédollars
- Berries
- Consumables
- Battle items
- Friendship or bond bonuses
- Common decorations
- Uncommon decorations
- Rare and exclusive decorations
- Special collectibles

### Expedition-exclusive decorations

Initial candidates:

- Camp Lantern
- Mushroom Stool
- Flower Patch
- Driftwood Bench
- Beach Shell Decor
- Crystal Cluster
- Cave Rock Seat
- Forest Log
- Explorer Pack
- Treasure Chest

Rare discovery candidates:

- Glowing Crystal
- Ancient Statue
- Moon Lamp
- Meteor Fragment
- Shrine Marker
- Golden Toy
- Fossil Display
- Legendary Banner

- [ ] Assign discoverable decorations to suitable destinations.
- [ ] Define rarity tiers and weighted reward probabilities.
- [ ] Prevent unavailable or undiscovered exclusive decorations from appearing in ordinary shop listings.
- [ ] Consider special achievement or seasonal expedition rewards in future updates.
- [ ] Maintain an expedition discovery history or journal.

## Phase 6: Decoration sprites, SD-card assets and discovery protection

**Goal:** Create a complete, consistent pixel-art decoration library and distribute game-ready assets without making undiscovered content obvious when browsing the SD card.

### Sprite design requirements

- [ ] Create original decoration sprites that visually complement the existing Pokémon Mystery Dungeon-style character sprites.
- [ ] Use clean, consistent pixel art rather than mismatched generic tilesets.
- [ ] Match the established visual scale, shading, perspective and color style of TamaPoke.
- [ ] Use nearest-neighbor scaling without smoothing or blurry interpolation.
- [ ] Create transparent-background source sprites.
- [ ] Provide each decoration as an individual image file.
- [ ] Standardize sprite dimensions by object type, with final sizes determined after checking the existing renderer and available habitat space.
- [ ] Keep sprites readable on the 466×466 round display.
- [ ] Ensure furniture, plants, toys and special objects use consistent visual proportions.
- [ ] Create animation frames only for objects that require them.
- [ ] Maintain clear attribution and asset licensing records where applicable.

### Asset deliverables

Each asset release should include:

1. Individual source PNG files for every decoration.
2. A catalog/manifest containing identifiers, dimensions, categories, rarity and placement rules.
3. Game-ready compiled sprite data.
4. SD-card asset bundles.
5. Any required conversion or packing scripts.
6. Documentation explaining how assets are updated or expanded.

Proposed development directory:

```text
assets/
  decor/
    source/
      furniture/
      decorations/
      nature/
      outdoor/
      toys/
      rare/
    manifest/
    packed/
```

Final runtime paths and formats must align with the existing TamaPoke SD-card loading system.

### Asset packaging and hidden discoveries

- [ ] Extend or reuse the existing binary sprite-packaging pipeline.
- [ ] Store decoration sprites on the SD card rather than embedding the entire catalog in firmware.
- [ ] Pack release assets into binary bundles instead of exposing individual named PNG files.
- [ ] Use numeric item IDs rather than obvious item names in release bundle indexes.
- [ ] Compress or lightly obfuscate packed data where practical.
- [ ] Avoid placing readable spoiler lists or descriptions of undiscovered rare items in release files.
- [ ] Preserve unpacked individual source files for development and future edits.
- [ ] Keep the runtime asset loader efficient and compatible with ESP32 memory constraints.
- [ ] Document that packaging discourages casual spoilers but cannot guarantee secrecy against reverse engineering.

### In-game discovery system

- [ ] Track whether each decoration has been discovered.
- [ ] Display undiscovered decorations as `???` or silhouettes where appropriate.
- [ ] Hide names and detailed previews for undiscovered rare decorations.
- [ ] Reveal the decoration's name, sprite and description after discovery.
- [ ] Keep purchased, discovered and currently placed states separate.
- [ ] Prevent collecting an item from automatically placing it in the habitat.
- [ ] Ensure unique rewards and duplicate discoveries have defined behavior.
- [ ] Make the discovery state persistent across restarts.

### Testing and validation

- [ ] Test sprite rendering and transparency in the emulator.
- [ ] Verify decoration scaling and positioning on actual Waveshare hardware.
- [ ] Check frame rate, RAM usage and SD-card read performance.
- [ ] Confirm habitat layouts load correctly after restarting.
- [ ] Confirm decorations do not obstruct Pokémon animations or touch controls.
- [ ] Test expedition persistence after power loss and restart.
- [ ] Test all expedition outcomes and reward rarities.
- [ ] Validate the packed asset format against missing and corrupted files.
- [ ] Add headless emulator tests for the Decor shop, habitat editing and expedition flow.
- [ ] Preserve compatibility with existing player saves wherever practical.

## Future ideas / backlog

These are optional improvements rather than committed release requirements.

- [ ] More habitat themes and alternate room backgrounds.
- [ ] Seasonal and holiday decoration collections.
- [ ] Expanded Pokémon personalities and decoration preferences.
- [ ] Rare Pokémon interactions with particular habitat objects.
- [ ] Additional expedition destinations and region-specific adventures.
- [ ] Expedition achievements and collectible badges.
- [ ] Special events that unlock exclusive decorations.
- [ ] Companion web dashboard for viewing collections and expedition history.
- [ ] Additional playable Pokémon generations when compatible animation packs become available.
- [ ] Expand expedition mechanics with optional random events or discoveries.

## Hosting

- [x] Implement a minimal HTTPS PHP API in a `server/` folder in this same repository (registration/profile/deletion).
- [ ] Create database migrations for profiles, devices, friendships, battles and turns.
- [x] Deploy initial registration/profile API to IONOS MariaDB; deploy the deletion update next.
- [ ] Configure secrets only on the host, never in firmware or GitHub.

## GBA interface rollout — Patch 7 (2026-11-12)
- [x] Restyle Friends, PC Box, Pokémon detail sheets, confirmation dialogs, first-run starters, name entry, training hub and training HUD/results with one consistent RGB565 GBA palette.
- [ ] Verify layout and tap behavior on the physical 466x466 ESP32 display (especially localized names and the SD Pokémon thumbnails).
- [ ] Complete the Windows emulator regression suite and review optional fine polish before finalizing the UI overhaul.
