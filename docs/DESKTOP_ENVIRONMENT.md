# VulcanOS Desktop Environment — Design Specification

**Status:** design-only. Nothing in this document is implemented yet.
No compositor, no window manager, no toolkit, no panel exists in the
codebase as of this writing (`kernel/` has boot, interrupts, and
memory management only — see `PROJECT_STATUS.md`). This spec exists
so that when the desktop *is* buildable — once VulcanOS has a
scheduler, a framebuffer/GPU driver, and a libc — the visual identity
and architecture decisions don't have to be re-derived from scratch
or reconstructed from memory.

This is a living reference, not a changelog. Update it in place as
decisions firm up; don't append "v2" sections.

---

## 1. Why this document exists now

The desktop is the last major subsystem on VulcanOS's roadmap (see
`PROJECT_STATUS.md`: kernel → mm → scheduler → fs → libc → userland
→ *then* graphics). By the time the kernel is ready for a
compositor, a lot of real work will have happened in between, and
specific visual decisions — exact colors, spacing, the shape of an
icon tile — are exactly the kind of detail that's easy to lose or
drift on across a long gap, even though the *system-level*
architecture decisions (protocol design, IPC model) tend to survive
fine in written form regardless of gap length. This document pins
the former down precisely and leaves room for the latter to be
revisited when there's a real display driver to design against.

## 2. Grounding reference

The visual direction below is informed by a reference screenshot of
an existing Openbox/LXQt-style Linux desktop (dark plum/sunset
wallpaper, a top-row grid of rounded-square application icons with
centered labels beneath each, floating windows with a colored
titlebar strip and right-aligned window controls, a bottom panel
with a launcher/taskbar region on the left and a system tray + clock
on the right). That reference is **not** software VulcanOS will
ship, run, or depend on in any way — no Openbox code, no LXQt panel,
no borrowed assets. It's cited here only as the layout/composition
inspiration this section formalizes into VulcanOS's own idiom, per
the project's stated goal of learning from existing desktop
environments' engineering approaches without copying their
implementation.

What VulcanOS keeps from that reference: the general *silhouette* —
icon grid up top, floating windows in the middle, a persistent panel
at the bottom edge. What VulcanOS does **not** keep: the specific
icon artwork, the exact panel widget layout, any color choice, any
window-chrome detail, or the underlying toolkit/protocol. Those are
designed fresh below.

**A rendered visual reference exists**: `docs/mockups/
ember-shell-reference.html` (and its rendered
`ember-shell-reference.png`) is a static, self-contained HTML/CSS
mockup applying every color/spacing decision in Sections 4-7 below.
It is not real VulcanOS code and never will run on VulcanOS — it's a
"does this actually look right" check, viewable in any browser, so
the palette and layout described in prose below have a concrete,
checkable artifact rather than existing only as a hex table. Every
color in that file was verified pixel-for-pixel against the table in
Section 4 (`ember-core` renders as exactly `#E8491B`, etc.) at the
time it was built — if either drifts from the other in a future
edit, that's a bug in this document, not an acceptable inconsistency
to leave standing.

## 3. Identity

- **Name:** VulcanOS. Desktop environment name: **Ember Shell** (ties
  to the kernel's existing codename, `VULCAN_CODENAME "Ember"` in
  `kernel/include/config.h` — the desktop is "Ember Shell running on
  VulcanOS N.N," the same way GNOME Shell runs on a distro).
- **Tone:** warm, volcanic, dark-by-default. Not a generic flat
  Material clone, not a skeuomorphic throwback. Molten color accents
  against near-black neutrals; sharp but not brutalist geometry
  (rounded corners, but small radii — 6-8px, not the 20px+ "friendly
  blob" look common in current mainstream OS design).

## 4. Color system

Named tokens, not raw hex scattered through component specs. Every
component below references these names so a future global palette
edit is a one-place change.

| Token | Hex | Usage |
|---|---|---|
| `ember-obsidian` | `#15100F` | Desktop background base, deep window chrome |
| `ember-basalt` | `#221A18` | Panel background, inactive window titlebar |
| `ember-ash` | `#4A3F3C` | Borders, dividers, disabled text |
| `ember-smoke` | `#8A7B76` | Secondary text, inactive icon labels |
| `ember-pumice` | `#D8CFC9` | Primary text on dark surfaces |
| `ember-core` | `#E8491B` | Primary accent — active window titlebar, focus rings, primary buttons |
| `ember-magma` | `#FF7A3D` | Hover states, secondary accent, active taskbar item |
| `ember-flare` | `#FFB454` | High-emphasis highlights, notification badges, warnings |
| `ember-vent` | `#2E8B7A` | Success/confirmation states (deliberately *not* a generic green — a teal that reads as "cooled lava vent," keeping the palette's identity even in status colors) |
| `ember-fault` | `#C0334D` | Error/destructive states |

Rule: **no pure black, no pure white.** `ember-obsidian` and
`ember-pumice` are as close as the palette gets, and both carry a
warm tint (obsidian leans brown-black, not blue-black; pumice leans
warm grey, not paper white). This is what keeps the whole system
feeling like one material instead of a dark-mode reskin of a
light-first design.

Light-mode variant: deferred. Ember Shell ships dark-only for the
first release; a light palette is a follow-up derived from these
same tokens once the dark system is validated, not designed in
parallel now.

## 5. Typography

- **UI font:** a single original geometric-sans typeface,
  provisionally named **Vulcan Sans** (to be designed/commissioned —
  do not substitute a well-known open-source font like Inter, Roboto,
  or Fira Sans as a permanent choice; those are fine as *temporary*
  development placeholders but VulcanOS's own identity work isn't
  done until this is original or properly licensed).
- **Monospace:** likewise, an original or properly-licensed mono
  face for `vulsh` and any terminal emulator — not a placeholder
  ship default.
- **Scale:** 13px base UI text, 11px secondary/caption text, 15px
  window titlebar text, 20px panel clock. Small, information-dense
  defaults in keeping with the "developer-focused, not
  beginner-oversized" goal from the project brief — but every size
  must be user-adjustable in settings; density is a default, not a
  mandate.

## 6. Layout anatomy

### 6.1 Desktop surface
- Full-bleed wallpaper. Default wallpaper is an original VulcanOS
  piece (volcanic gradient in the `ember-*` palette — not the sunset
  photo from the reference), swappable by the user.
- Icon grid anchored top-left by default (reference image showed a
  top row; VulcanOS keeps top-anchored placement but grows downward
  in a column-major grid as more icons are added, rather than
  wrapping into a second horizontal row immediately).
- Icon tile: 64×64px icon glyph, rounded-square **frame** at 8px
  corner radius (not the icon glyph itself — see Iconography below),
  12px label beneath in `ember-pumice`, tile hit-area 88×88px
  including padding. Hover state: `ember-basalt` tile background
  fades in behind the icon at 60% opacity.

### 6.2 Windows
- Titlebar height: 32px. Active window titlebar: `ember-core`
  gradient (subtle, `ember-core` → 8% darker, top to bottom — not
  flat, but not a heavy skeuomorphic bevel either). Inactive
  titlebar: flat `ember-basalt`.
- Window controls: right-aligned (matches the reference), but
  VulcanOS's own glyph set — not a copy of any existing OS's
  close/minimize/maximize icon shapes. See Iconography.
- Corner radius: 8px on the two top corners only when not maximized;
  square when maximized (a maximized window shouldn't visually float
  above the work area it fills).
- Focus ring: none needed separately — the titlebar color change
  *is* the focus indicator, keeping chrome minimal.

### 6.3 Panel
- Fixed to the bottom edge (matches the reference), 40px tall,
  `ember-basalt` background, 1px `ember-ash` top border.
- Left region: launcher button + running-application taskbar
  entries. Taskbar entries show icon + truncated title; active
  window's entry gets an `ember-core` underline (2px), not a full
  background fill — keeps the panel visually quiet when many windows
  are open.
- Right region: system tray (network, volume, battery — each an
  original glyph, see below) + 24-hour clock in `ember-pumice`, 20px.
  No AM/PM ship default; locale-driven 12-hour format is a settings
  toggle, not the default, to match the "developer-focused" tone.
- Center region: reserved, empty by default. Not a workspace
  switcher by default (VulcanOS's workspace model is a separate,
  not-yet-designed decision — don't presume it here).

## 7. Iconography

Governed by the project's iconography design directive
(`/mnt/skills/user/iconography-design-directive/SKILL.md` at build
time, or wherever that skill lands in a future working environment)
— **no emoji, ever**, and every icon follows one consistent
construction logic rather than being drawn ad hoc per-icon. Concrete
rules for Ember Shell specifically:

- **Grid:** all icons built on a 24×24 unit grid with 2px live
  padding, scaled up for the 64px desktop tiles.
- **Stroke:** 2px consistent stroke weight for line icons (window
  controls, tray icons); filled glyphs (app icons) use flat fills
  from the `ember-*` palette, never gradients or drop shadows —
  gradients are reserved for the titlebar and wallpaper, not
  scattered into every glyph.
- **Corner logic:** every icon's rounded corners use the *same*
  radius as the chrome they sit in (8px family for app-tile frames,
  4px for small inline glyphs like tray icons) — no mixed radii
  within one icon set.
- **Window control glyphs:** close = a single diagonal cross built
  from two straight strokes at the palette's stroke weight (not an
  "X" borrowed from any specific existing OS's exact angle/weight);
  minimize = a single horizontal bar; maximize = an outlined square.
  Simple enough to be instantly legible at 12px, original enough to
  not be a traced copy of Windows/macOS/GNOME's exact glyphs.

## 8. Compositor & protocol architecture (design intent, not implemented)

Per the project brief, VulcanOS does not adopt X11 or an unmodified
Wayland as its windowing system verbatim — it designs its own,
learning from both.

- **Display protocol:** a VulcanOS-native protocol, working name
  **VDP (Vulcan Display Protocol)**. Client-compositor IPC over a
  VulcanOS-native socket/message-passing primitive (depends on
  whatever IPC mechanism the kernel's `proc`/scheduler work settles
  on — not designed yet, intentionally, since designing IPC framing
  before the kernel has a process model to run it on would be
  premature).
- **Buffer model:** client-side rendering into shared memory buffers
  handed to the compositor for composition — the same *category* of
  design Wayland uses (client draws, compositor composites, no
  server-side drawing API like X11's) because that model is simply
  correct for a modern compositor, not because VulcanOS is copying
  Wayland's specific wire protocol. VDP's actual message format,
  buffer handoff mechanism, and synchronization primitives are a
  separate design task for when there's a real GPU/framebuffer
  driver to build against.
- **Rendering backend:** software rasterization first (framebuffer
  writes, no GPU driver dependency), matching the "boot cleanly on
  real and virtual hardware without a GPU-specific driver stack"
  goal implicit in the project's BIOS/UEFI-first, driver-model-second
  approach. A GPU-accelerated path is future work once VulcanOS has
  its own driver framework mature enough to support one.

This section is intentionally the least specified part of the
document — protocol and buffer-management design decisions made
today, disconnected from a real kernel process model and a real
framebuffer driver, would very likely need to be redone anyway once
those exist. What's fixed here (client-side rendering, VulcanOS-
native protocol identity, software-first rendering) is fixed because
those are architecture-level commitments worth stating now; the wire
format is not.

## 9. What this document deliberately does not cover

- **Application framework / toolkit API** (how a VulcanOS app
  actually declares a button or a layout) — depends on libc and the
  syscall surface, neither of which exist yet.
- **Workspace/virtual-desktop model** — not decided; don't assume
  one exists when reading Section 6.3's panel spec.
- **Settings app / control panel design** — out of scope until
  there's a toolkit to build it in.
- **Notification system design** — flagged for a future pass;
  `ember-flare` is reserved for it but the interaction model isn't
  designed.
- **Accessibility (screen reader, high-contrast mode, keyboard-only
  navigation)** — this is a real gap in this version of the spec,
  not a deliberate deferral the way the items above are. It needs a
  dedicated pass before implementation begins, not just before
  ship.

## 10. Revisiting this document

When the kernel reaches the point of having a scheduler, a working
libc, and at least a framebuffer driver stub, the right next step is
**not** to start implementing Section 6 directly. Re-read Section 8
first, make the real protocol and buffer-management decisions
against the actual driver/IPC primitives that exist by then, and
only then move into building the visual layer this document
specifies. The visual system (Sections 3-7) should need little to no
revision by that point; the architecture section (8) should be
expected to change completely.
