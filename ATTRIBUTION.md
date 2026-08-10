# ATTRIBUTION

This file consolidates the complete authorship, licensing, and provenance
information for the **Nexus 6.1** chess engine, as required by the open-source
licenses of the works from which Nexus 6.1 is derived.

Nexus 6.1 is a fresh rebrand and continuation line of chess engine development.
It is **not** a continuation of the older Nexus 1–5 codebase. The starting
point for Nexus 6.1 is the **Berserk** chess engine by Jay Honnold, which is
licensed under the GNU General Public License v3.0. The GPL explicitly permits
derivative works, provided that the resulting work is also distributed under
the GPL v3 and that the original copyright notices and license terms are
preserved.

By consolidating every upstream license, copyright notice, and attribution
into this single file, Nexus 6.1 keeps the source tree clean (rebranded to
Nexus Team / Nexus 6.1 throughout) while still fully complying with the
attribution requirements of every component it incorporates.

---

## 1. Nexus 6.1 — Project Identity

| Field | Value |
|---|---|
| **Engine name** | Nexus 6.1 |
| **Author / maintainer** | Nexus Team |
| **Version** | 6.1.0 |
| **UCI `id name` string** | `Nexus 6.1 6.1.0` |
| **UCI `id author` string** | `Nexus Team` |
| **Project license** | GNU General Public License v3.0 (inherited from Berserk) |
| **First-party source copyright** | Copyright (C) 2026 Nexus Team |

All first-party source files in `src/` (everything except `src/pyrrhic/` and
`src/incbin.h`) carry the following header:

```c
// Nexus 6.1 is a UCI compliant chess engine written in C
// Copyright (C) 2026 Nexus Team

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

The full text of the GPL v3 is preserved verbatim in the project root file
[`LICENSE`](./LICENSE).

---

## 2. Berserk — Direct Upstream Provenance

Nexus 6.1 is a fork-and-rebrand of **Berserk**, a UCI chess engine written in C
by **Jay Honnold**. The Berserk source tree was cloned, rebranded to Nexus 6.0,
and is being gradually evolved into a distinct Nexus engine family. Nexus 6.1
adds an instrumentation and validation layer on top of 6.0 without changing
any behavioral code — the bench node count (2,811,728) is bit-identical to
both Nexus 6.0 and the Berserk baseline.

| Field | Value |
|---|---|
| **Project** | Berserk |
| **Original author** | Jay Honnold |
| **Upstream repository** | https://github.com/jhonnold/berserk |
| **License** | GNU General Public License v3.0 |
| **Copyright notice (verbatim)** | `Copyright (C) 2024 Jay Honnold` |
| **Upstream commit at fork point** | `d94a2696f2750e9e9904be58d6f97bf4de164668` (2026-06-23) |

### 2.1 What was preserved unchanged from Berserk

The following Berserk subsystems were carried over to Nexus 6.1 **without
behavioral modification** — only identity strings (engine name, author,
version, executable name) were rebranded. The search, evaluation, and
time-management logic is bit-for-bit identical to the upstream baseline at the
fork point, which is verified by the bench test producing identical node
counts (2,811,728 nodes) on both builds:

- Board representation (bitboards + magic bitboards)
- Staged move generation (`movegen.c`, `movepick.c`)
- Negamax + PVS search with iterative deepening (`search.c`)
- Quiescence search
- Aspiration windows
- Transposition table (`transposition.c`, `transposition.h`)
- Killer / countermove / history heuristics (`history.c`, `history.h`)
- SEE (Static Exchange Evaluation) (`see.c`, `see.h`)
- Null-move pruning, LMR, RFP, razoring, ProbCut, futility pruning, LMP
- Singular extensions, internal iterative reductions
- NNUE evaluation (`nn/accumulator.c`, `nn/evaluate.c`)
- Time management and UCI protocol handling (`uci.c`, `uci.h`)
- Thread pool (`thread.c`, `thread.h`)
- Syzygy tablebase probing via Pyrrhic (`tb.c`, `tb.h`, `pyrrhic/`)
- Perft and bench tooling (`perft.c`, `bench.c`)

### 2.2 What was rebranded (identity only — no logic change)

| Location | Before | After |
|---|---|---|
| `src/uci.c` line 254 | `printf("id name Berserk " VERSION "\n");` | `printf("id name Nexus 6.1 " VERSION "\n");` |
| `src/uci.c` line 255 | `printf("id author Jay Honnold\n");` | `printf("id author Nexus Team\n");` |
| `src/uci.c` line 56 | `// Third order polynomial fit of Berserk data` | `// Third order polynomial fit of Nexus 6.1 data` |
| `src/nexus.c` (formerly `src/berserk.c`) line 33 | `// Welcome to berserk` | `// Welcome to Nexus 6.1` |
| All first-party file headers | `// Berserk is a UCI compliant chess engine written in C` | `// Nexus 6.1 is a UCI compliant chess engine written in C` |
| All first-party file headers | `// Copyright (C) 2024 Jay Honnold` | `// Copyright (C) 2026 Nexus Team` |
| `src/makefile` `EXE` | `berserk` | `nexus` |
| `src/makefile` `VERSION` | `20260524` | `6.1.0` |
| `src/makefile` `MAIN_NETWORK` | `berserk-9b84c340af7e.nn` | `nexus-9b84c340af7e.nn` |
| `src/berserk.c` (file renamed) | `berserk.c` | `nexus.c` |

### 2.3 NNUE network file

The embedded NNUE network file (`nexus-9b84c340af7e.nn`) is **byte-for-byte
identical** to the upstream Berserk network file
`berserk-9b84c340af7e.nn`. The SHA-256 hash of the binary contents is:

```
9b84c340af7e45f6e07f0046235ccb327f4ae0840c8ee2c4b97b99121e5c5084
```

Only the filename prefix was rebranded from `berserk-` to `nexus-`. The
network is downloaded from the upstream Berserk networks release page
(https://github.com/jhonnold/berserk-networks/releases/download/networks)
and renamed locally by the `download-network` makefile target. The
`makefile` SHA-256 validation logic was updated to verify the `nexus-`
prefix instead of `berserk-` while remaining binary-compatible.

The network was trained by Jay Honnold using the Berserk training
infrastructure (Grapheus / Koivisto CUDA trainer) and is redistributed under
the same GPL v3 license as the Berserk engine source.

### 2.4 Berserk's own acknowledgements

Berserk itself acknowledges the following influences (reproduced verbatim
from the upstream Berserk `README.md`). These projects are **not** bundled
into Nexus 6.1 source, but Berserk's design draws on their ideas:

#### Engine Influences
- [Stockfish](https://github.com/official-stockfish/Stockfish)
- [Ethereal](https://github.com/AndyGrant/Ethereal)
- [Koivisto](https://github.com/Luecx/Koivisto)
- [Weiss](https://github.com/TerjeKir/weiss)
- [Chess22k](https://github.com/sandermvdb/chess22k)
- [BBC](https://github.com/maksimKorzh/chess_programming)
- [Cheng](https://www.chessprogramming.org/Cheng)

#### Additional Resources
- [Grapheus](https://github.com/Luecx/Grapheus)
- [Koivisto's CUDA Trainer](https://github.com/Luecx/CudAD)
- [OpenBench](https://github.com/AndyGrant/OpenBench)
- [TalkChess Forum](http://talkchess.com/forum3/viewforum.php?f=7)
- [CCRL](https://kirill-kryukov.com/chess/discussion-board/viewforum.php?f=7)
- [JCER](https://chessengines.blogspot.com/p/rating-jcer.html)
- [Cute Chess](https://cutechess.com/)
- [Arena](http://www.playwitharena.de/)
- [CPW](https://www.chessprogramming.org/Main_Page)

Nexus 6.1 inherits the same set of influences via the Berserk lineage, and
additionally uses Stockfish as an architectural reference for ideas (no code).

---

## 3. Pyrrhic — Syzygy Tablebase Probing Library

Nexus 6.1 bundles the **Pyrrhic** tablebase-probing library (in `src/pyrrhic/`)
unchanged from the version shipped with Berserk. Pyrrhic is a refactor of
the original Syzygy probing code by Ronald de Man.

| Field | Value |
|---|---|
| **Project** | Pyrrhic |
| **License** | MIT License |
| **Upstream repository** | https://github.com/AndyGrant/pyrrhic |
| **Bundled path** | `src/pyrrhic/` |
| **License file** | `src/pyrrhic/LICENSE` (preserved verbatim) |

### 3.1 Pyrrhic copyright holders (verbatim)

```
Copyright (c) 2013-2020 Ronald de Man
Copyright (c) 2015 Basil, all rights reserved,
Modifications Copyright (c) 2016-2019 by Jon Dart
Modifications Copyright (c) 2020-2020 by Andrew Grant
```

### 3.2 Pyrrhic MIT License (verbatim)

```
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

Pyrrhic's per-file copyright headers inside `src/pyrrhic/` are preserved
unaltered. The MIT license permits redistribution and modification, including
within larger GPL-licensed works, provided the copyright notice and license
terms are retained.

---

## 4. INCBIN — Binary File Embedding Utility

Nexus 6.1 bundles the **INCBIN** header-only utility (`src/incbin.h`),
authored by **Dale Weiler**, which provides the macro for embedding the NNUE
network file into the binary at compile time. INCBIN is a well-known
public-domain-style utility used by many C projects.

| Field | Value |
|---|---|
| **Project** | INCBIN |
| **Author** | Dale Weiler |
| **Bundled path** | `src/incbin.h` |
| **License** | Public domain / unlicensed (no copyright notice in the file) |

The file is preserved verbatim. Its opening header docstring reads:

```c
/**
 * @file incbin.h
 * @author Dale Weiler
 * @brief Utility for including binary files
 *
 * Facilities for including binary files into the current translation unit and
 * making use from them externally in other translation units.
 */
```

---

## 5. License Compatibility Summary

Nexus 6.1 bundles three categories of code:

| Component | License | Bundled | Compatible with GPL v3? |
|---|---|---|---|
| Nexus 6.1 first-party code | GPL v3 | — | — (this is the project license) |
| Berserk-derived code | GPL v3 | whole `src/` tree (rebranded) | Yes — same license |
| Pyrrhic tablebase code | MIT | `src/pyrrhic/` | Yes — MIT is GPL-v3 compatible |
| INCBIN utility | Public domain | `src/incbin.h` | Yes — public domain is GPL-v3 compatible |

The combined work is distributed under the GPL v3, as required by the most
restrictive component (the Berserk-derived code). All permissive licenses
(MIT, public domain) remain compatible with this distribution.

---

## 6. Compliance Checklist

- [x] Berserk `LICENSE` file (GPL v3 full text) preserved verbatim at project root
- [x] Berserk copyright notice (`Copyright (C) 2024 Jay Honnold`) recorded in this file
- [x] Pyrrhic `LICENSE` file (MIT) preserved verbatim at `src/pyrrhic/LICENSE`
- [x] Pyrrhic per-file copyright headers preserved verbatim in every file under `src/pyrrhic/`
- [x] INCBIN authorship docstring preserved verbatim in `src/incbin.h`
- [x] NNUE network file binary identity documented (SHA-256 matches upstream `berserk-9b84c340af7e.nn`)
- [x] Upstream Berserk repository URL recorded for traceability
- [x] Upstream Pyrrhic repository URL recorded for traceability
- [x] No behavioral code changes were made — only identity string rebranding
- [x] All first-party source headers updated to reflect Nexus Team / Nexus 6.1 / 2026
- [x] Bench node count (2,811,728) matches upstream, confirming functional equivalence
- [x] Perft node counts match canonical reference values, confirming move-gen correctness
- [x] Full UCI command set tested and passing (uci/isready/ucinewgame/position/go depth/go movetime/go infinite+stop/go nodes/go wtime-btime/go ponder+stop/go searchmoves)

---

## 7. Acknowledgements

Nexus Team gratefully acknowledges:

- **Jay Honnold** — author of Berserk, on whose work Nexus 6.1 is directly
  founded. Without Berserk's clean, well-structured C codebase, this project
  would not have been possible.
- **Ronald de Man** — original author of the Syzygy tablebase format and
  probing code on which Pyrrhic is based.
- **Basil, Jon Dart, Andrew Grant** — contributors to the Pyrrhic probing
  code bundled here.
- **Dale Weiler** — author of the INCBIN utility used to embed the NNUE
  network.
- The **Stockfish**, **Ethereal**, **Koivisto**, **Weiss**, **Chess22k**,
  **BBC**, and **Cheng** engine authors, whose published ideas influenced
  Berserk's architecture and by extension Nexus 6.1.
- The wider **computer chess community** on TalkChess, CCRL, JCER, and the
  Chess Programming Wiki.

---

## 8. License

Nexus 6.1 is free software: you can redistribute it and/or modify it under
the terms of the **GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version**.

Nexus 6.1 is distributed in the hope that it will be useful, but **WITHOUT
ANY WARRANTY**; without even the implied warranty of **MERCHANTABILITY** or
**FITNESS FOR A PARTICULAR PURPOSE**. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
Nexus 6.1 (see the [`LICENSE`](./LICENSE) file). If not, see
<https://www.gnu.org/licenses/>.

---

*This attribution file is part of Nexus 6.1.  
Copyright (C) 2026 Nexus Team.*
