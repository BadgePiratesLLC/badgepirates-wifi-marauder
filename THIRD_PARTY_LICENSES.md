# Third-Party Licenses

This firmware is built primarily on [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder)
by justcallmekoko (MIT License — see [LICENSE](LICENSE)), pulled in as the
`esp32marauder-upstream` git submodule.

In addition, this build links the following third-party libraries, which are
licensed under the **GNU Lesser General Public License v3.0 (LGPL-3.0)**
rather than MIT:

| Library | Source | License |
|---|---|---|
| ESPAsyncWebServer | bundled in `esp32marauder-upstream/libraries` (used for the Evil Portal feature) | LGPL-3.0 |
| AsyncTCP | [me-no-dev/AsyncTCP](https://github.com/me-no-dev/AsyncTCP) (`lib_deps` in `platformio.ini`) | LGPL-3.0 |

Both are statically linked into the compiled firmware binary. Under LGPL-3.0
this is permitted as a "Combined Work" provided that:

- The complete corresponding source of the Application (this repository, MIT
  licensed) remains available — it does, as a public repo.
- The complete corresponding source of the Library (ESPAsyncWebServer /
  AsyncTCP, including any modifications) remains available under LGPL-3.0 —
  it does, unmodified, via the upstream submodule and the PlatformIO registry
  URLs referenced in `platformio.ini`.
- Users are given the license text and are permitted to modify and relink
  against a modified version of the Library.

Full LGPL-3.0 text: https://www.gnu.org/licenses/lgpl-3.0.txt
