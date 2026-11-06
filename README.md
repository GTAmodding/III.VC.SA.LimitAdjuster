Open Limit Adjuster
==============================

An open source limit adjuster for Grand Theft Auto III, Vice City and San Andreas.

Every limit is unlimited by default: it grows when the game needs more, and costs nothing until then. Pools are the
exception: `unlimited` allocates their ceiling when they are created. Nothing has to be configured.
`III.VC.SA.LimitAdjuster.ini` only matters to cap a limit; see the comments in it for what each limit is.

`OutsideWorldWaterBlocks = unlimited` follows the game's current view distance, including changes made by
Project2DFX. The sea reaches the visible horizon; its storage grows when needed. A number keeps a fixed count
(III/VC: lines of 256-unit blocks; SA: blocks), and `0` keeps the game's own limit.

`MAINsegmentSize` and `MissionSize` (San Andreas) default to `unlimited`: the selected SCM header and `script.img` determine
storage before the script loads, with a minimum of 1 MiB MAIN and 256 KiB mission space. Numbers fix the sizes in
bytes (rounded to four); `0` retains the game's 200000/69000. In III and Vice City, CLEO already enlarges script space,
so the adjuster leaves it alone there.
Saves from vanilla-size scripts keep the vanilla
layout; saves using larger scripts need the same scripts to reload. Files that exceed a fixed size are rejected before copying.
Custom maps: `ImgArchives` (room for 128 IMG archives; left to Mod Loader when it is installed), `TxdStore` (III up to 4096, VC up to 20000), `ObjectData`
(object.dat physics records) and, in San Andreas, `WaterQuads`/`WaterTris`/`WaterVertices` (water.dat planes) grow as
needed. San Andreas `Peds` and `PedIntelligence` share one count. Replays are turned off while the ped or vehicle pool
is larger than the game's own.
`ScriptSearchLights = unlimited` grows the opcode 06B1 pool when full, within the game's 65535-slot handle range.
A number fixes its size, and `0` keeps the native eight slots. All three names are available through `OpenLA_GetLimit`.
`Explosions = unlimited` grows San Andreas' 16 explosion slots when full, up to 255; a number fixes the count. Mods
that read the explosion array directly find it at the operand of `add esi, aExplosions` (0x736AD1).
`FxMemory = unlimited` gives San Andreas 16 MB for effect definitions instead of 1 MB and keeps the room free
after loading, so effects added or reloaded later (EffectLoader, live editors) fit; a number sets the megabytes.

### Supported games

* GTA III 1.0
* GTA Vice City 1.0
* GTA San Andreas 1.0 US (the HOODLUM and the compact exe)

Other versions are left alone; the log says so.

### Installing

Put `III.VC.SA.LimitAdjuster.asi` (and the .ini if you want to change something) in the game folder or in `scripts`,
with an ASI loader. Not inside the `modloader` folder: it has to start before Mod Loader patches the game.

Compatibility: with Mod Loader, IMG archives are left to Mod Loader. With an older skygfx (one that reads a few game
arrays at their fixed addresses), the III/VC visible entity list and the SA point lights keep the game's size; current
skygfx builds work with the moved arrays. With Proper Shaders the SA point lights keep the game's size as well.

With `Log = 1` in the ini's `[OPTIONS]`, `III.VC.SA.LimitAdjuster.log` next to the plugin lists what was changed. A limit that another mod already changed is
left to that mod.

### For other plugins

The plugin exports `OpenLA_GetVersion`, `OpenLA_GetLimit(name, &used, &capacity, &maximum)` and
`OpenLA_GetStreamingBudget()`.

### Compiling

Needs Visual Studio 2026. Clone with submodules (`git clone --recursive`), run `premake5.bat` and open
`build/OpenLA.slnx`.
