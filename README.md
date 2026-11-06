# TamaToys TamaPoke — Phase 4 integration, Step 2

**Scope of this patch:** Wire the decoration inventory from Step 1 into persistent NVS and serial backup/export. This does **not** yet implement the Decor shop interface, sprite drawing, or placement UI. It is a safe backend increment.

## Files to copy into repository root
- `decor_catalog.h`, `decor_catalog.cpp` (replaces first starter pack versions; fixes duplicate validation on imported layouts)
- `decor_store.h`, `decor_store.cpp` (new two-slot CRC-backed NVS persistence adapter)
- `TamaPoke.ino` (one include and one `decorBegin()` call)
- `save.cpp` (registers `decA` and `decB` in the existing serial backup format)

`pet.cpp` and `save.h` are present **unchanged for reference** and should not be replaced.

## Why isolated storage?
Existing `petA/petB` and `plyA/plyB` checkpoint formats are left unchanged. The decoration module stores owned/discovered bitsets and 11 placed item IDs in two new keys (`decA`, `decB`), each verified by CRC and generation. A torn write can fall back to the prior valid key. An unreadable save is not automatically replaced with starter items.

## Testing
- Run `bash tests/run.sh` from this extracted folder (pure C++ catalog tests).
- In the full checkout, run `bash tools/emu/tests/run.sh save` and `bash tools/emu/tests/run.sh reset` (depending on the available test targets; inspect the runner first). Then run the full emulator regression suite.
- Serial backup should now include the `decA` and `decB` NVS keys, where present.
- Test upgrade from an old save (starter decorations granted), reload, and simulated bad slot writes.
- Validate save transfer size with an actual populated game; current serial transfer maximum remains 4096 bytes.
- Only then stage/commit in VS Code Source Control.

## Important limitations
- This step does not yet give the user a Decor tab or show objects in the habitat.
- The runtime catalog currently contains rare item names in code. Packed SD sprites later reduce casual browsing of art but cannot fully conceal content in open source.
- There is no full firmware emulator checkout in the uploaded files, so a complete build cannot be verified here.
