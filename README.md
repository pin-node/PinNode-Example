# pin-node-example

A working starting point for a RetroPinUpgrade (RPU) game that reports to
**PinNode**. It is Dick Hamill's
[ExampleMachine](https://github.com/RetroPinUpgrade/ExampleMachine) (upstream
`main`, 16 Sep 2026) with the PinNode link added and nothing taken away: attract
mode, a game that starts, scores, serves the next ball and ends, the operator
menus, and every step of it reported to the node.

It is a "nothing game" on purpose. Copy it, change the machine definitions, and
build your rules into the places marked `CUSTOMIZATION TODO`.

Hardware: RPU Rev 4 (or Rev 102) with a Mega 2560 Pro, and an ESP32 running the
PinNode node firmware in the board's WiFi socket. Sound is a WAV Trigger.

## What PinNode gets

| Event | Where in `pin-node-example.ino` |
|---|---|
| HELLO (version, ruleset, manifest, build) | `setup()`, and again whenever the link returns |
| Mode (attract, game, service) | `loop()` |
| Game start, player added | `InitGamePlay()`, `AddPlayer()` |
| Ball start, ball end | `InitNewBall()`, `MACHINE_STATE_BALL_OVER` |
| Game end (normal, or abandoned for a new game) | `RunGamePlayMode()` |
| Tilt warnings and tilts | `HandleSystemSwitches()` |
| Coins | the two coin switch handlers |
| Start from the app | `RunAttractMode()` and `RunGamePlayMode()` |
| Claim codes on the score displays | `RunAttractMode()` |
| 23 settings, changeable from the app | `PinNodeSettingsTable` |
| 6 audits | `PinNodeAuditsTable` |
| An achievement and two stats, as examples | `ACH_FIRST_GAME`, `STAT_POP_BUMPER_HITS`, `STAT_BEST_BALL` |
| Operator tests and one-shots from the phone | `PinNodeActionsTable`, `HandleOperatorAction()`, `RunDiagnosticsMode()` |
| The tournament settings lock, at the coin door too | `GuardAdjustmentIfLocked()` |

## Operator actions from the phone

The app can run what the coin-door menu runs:

- **Tests** (lamp, display, solenoid, switch, sound) run only in
  `MACHINE_STATE_DIAGNOSTICS`. The phone puts the machine there, from attract
  only, with `enter-diagnostics`, and takes it out with `exit-diagnostics`. The
  self-test or credit button at the machine leaves it too. After five minutes
  with nothing running, the PinNode library asks for the exit itself (when the
  phone was what entered diagnostics). A test stops by itself 60 seconds after the
  last keepalive.
- **One-shots** (eject balls, clear audits, factory defaults, the rules presets)
  run in attract or diagnostics.
- Nothing runs during a game or while the coin-door menu is open: the request
  is answered "not now".

Each is a plain function (`StartLampTest()`, `StartSolenoidTest()`,
`ClearAudits()`, `RestoreDefaults()`, `InstallPreset()`, `StopTest()` …) in the
sketch; `OperatorMenus.cpp` is untouched. The action numbers are the `ACTION_`
defines, which must match `actions[]` in the manifest.

All of it is inside `#ifdef RPU_OS_USE_PINNODE_LINK` (set in `RPU_Config.h`), so
`git grep -n RPU_OS_USE_PINNODE_LINK` lists every hook.

## Rules for game code

- **Never use `Serial2`.** RPULink drives USART2 itself.
- **Keep every pass of `loop()` well under 100 ms**, and don't call `delay()`
  after `setup()`. The node counts the link as down after 100 ms.
- A saved ball is not the end of a ball: `PinNodeCtrl_BallEnd` belongs only in
  `MACHINE_STATE_BALL_OVER`.
- The link and the `PinNodeCtrl_` API come from the installed **PinNode**
  library (`#include <PinNode.h>`). There are no copies of it in this repo.

## Building it for a real machine

Out of the box the sketch drives ExampleMachine's made-up playfield
(`ExampleMachineMEGA.h`), whose coil and switch numbers match no real game. To
build it for a machine, turn on that machine's define in `RPU_Config.h`:

```cpp
#define MACHINE_BALLY_STAR_TREK
```

Its lamps, switches and solenoids then come from the **RPUMachines** library
([github.com/pin-node/RPUMachines](https://github.com/pin-node/RPUMachines)), and the nothing game serves from the outhole
kicker, fires each bumper and slingshot from its own switch, resets the one bank
of four drop targets, and numbers the solenoid test by coil number. Don't run
the unselected build on a real machine: its "serve ball" is another machine's
coil.

To add a machine: add its header to RPUMachines (by pull request), and a block like the
Star Trek one at the top of `ExampleMachineMEGA.h` that maps the template's
names onto it.

## The manifest

`pinnode.game.json` declares the ruleset: its id and version, achievements,
stats, settings and audits. `PinNodeGame.h` is generated from it and gives the
sketch the `ACH_`, `STAT_`, `SET_` and `AUD_` constants and the manifest hash.

Edit the manifest and make a new `PinNodeGame.h` in the Achievements builder at
[pin-node.com/builder](https://pin-node.com/builder); commit both files.

When you copy this template for your own game, give the ruleset its own `id`,
`title`, `name` and `repo` (the GitHub repo your game lives in), start its
`version` at your first release, and make a new `PinNodeGame.h`.

From the command line (or in CI), `pinnode-gen` from the PinNode library
([github.com/pin-node/PinNode](https://github.com/pin-node/PinNode)) does the same:

```bash
pinnode-gen check pinnode.game.json       # validate
pinnode-gen generate pinnode.game.json    # rewrite PinNodeGame.h
```

The position of a row in
`PinNodeSettingsTable` and `PinNodeAuditsTable` is its `index` in the manifest.
Once a version has been released, an index never changes meaning.

To add an achievement: add it to `achievements` in the manifest with the next
free `index`, make a new `PinNodeGame.h`, and call `PinNodeCtrl_Achievement(slot, ACH_YOURS)`
where it's earned (slot is the player number, 1-4). Award it once per player per
game. A stat is the same with `PinNodeCtrl_Stat(slot, STAT_YOURS, PINNODE_STAT_ADD, 1)`
or `PINNODE_STAT_MAX`.

## Building

Install PinNode: Tools → Manage Libraries…, search PinNode, Install. For the
Bally Star Trek option (`MACHINE_BALLY_STAR_TREK` in `RPU_Config.h`), also install
RPUMachines the same way.

From the command line:

```bash
arduino-cli lib install PinNode            # and RPUMachines for the Star Trek option
arduino-cli compile -b arduino:avr:mega:cpu=atmega2560 .
```

The sketch folder must be called `pin-node-example` (it has to match the `.ino`).

RPU board schematics: see [RetroPinUpgrade](https://github.com/RetroPinUpgrade) on GitHub.

## Changes from upstream ExampleMachine

- `RPU_Config.h`: `RPU_OS_HARDWARE_REV 4` and `RPU_OS_USE_PINNODE_LINK`.
- The PinNode hooks listed above.
- `InitNewBall()` turns the solenoid stack and the flippers back on. Upstream
  leaves the stack off after the first bonus countdown and the flippers off
  after a tilt.
- First-boot defaults write the three coin audits as 4-byte values (upstream
  writes one byte, so a new EEPROM reads 4,294,967,040 coins) and store the game
  rules selection.
- With ball save set to 0 there is no ball save. Upstream still saves a ball that
  drains within 3 seconds of its first switch.
- A new high score flashes in the display of the player who set it (upstream
  always uses the last player's display).
- The mode reported to PinNode during the match sequence keeps the last ball's
  number.
- The serve coil is pulsed once per ball (upstream pulses it twice: the "ball
  still in the trough" check fires before the scheduled serve).
- `SwitchToChuteNum()` returns the chute for coin switches 2 and 3 (upstream
  counts every coin as chute 1).

## Licence

GPL-3.0, as RPU and ExampleMachine are. See `LICENSE`.
