# Procedural object regression tests

From the repository root, using a host C++ compiler:

```sh
c++ -std=c++17 -Wall -Wextra -pedantic -fsanitize=address,undefined \
    tools/tests/ProceduralObjects.cpp -o build/procedural-tests
./build/procedural-tests
```

These exercise capacity exhaustion, list reuse across games/reloads, occupied-slot counting at 65535,
and word-sized comparison patches.
The normal 32-bit Windows ASI build checks game structure sizes and offsets.

In-game verification requires SA 1.0 US:

- Use the updated SALodLights build for procedural allocation-failure recovery;
  OLA only adjusts capacities and reports usage. Also test OLA on its own.
- Verify the five OLA limits and usage on the F5 display, including individual
  overrides and the all-vanilla/omitted configuration.
- ProcObjectEntries counts occupied tracking slots and should not exceed its
  configured total. ProcTempObjectThreshold reports the shared temporary-object
  counter; with 'unlimited' it has no threshold.
- Visit grass-heavy terrain and slopes; exercise both scattered and grid-based
  `procobj.dat` rules. Check creation and removal while moving between areas.
- Start/load another game in the same process and check capacity reuse.
- Verify initialization/reload hooks remain installed after calling the original
  routines, including a failed plant-config reload followed by a successful one.
- Check that larger `Objects`, `Buildings`, and `MatrixList` settings coexist
  with the procedural budgets.

Host tests do not validate executable patch locations or in-game rendering.
