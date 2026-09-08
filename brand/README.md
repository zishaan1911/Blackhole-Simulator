<picture>
  <source media="(prefers-color-scheme: dark)" srcset="logo-on-dark.svg">
  <img alt="Kerrscope" src="logo-on-light.svg" width="372">
</picture>

# Brand

Everything here is derived from the renderer rather than invented next to it.
That is the only rule worth stating: if a brand decision cannot be traced back
to something the program computes, it does not belong.

## Name

**Kerrscope.** An instrument for looking at Kerr spacetime. The Kerr metric is
the thing being solved; `-scope` is the honest description of what the program
is — you point it at a black hole and look, and the picture is a measurement
rather than an illustration.

Written **Kerrscope**: one word, capital K, no camel case, no hyphen.
`KERRSCOPE` in the wordmark only.

The executable is and stays **`kerr`**. It is shorter to type, it is what every
existing doc, script and `.gitignore` line already says, and renaming a binary
is a breaking change bought for nothing. Kerrscope is the project; `kerr` is the
command.

## Tagline

> one geodesic per pixel

Twenty-two characters that say the entire differentiator. Lensing here is
integrated, not composited: for each pixel the shader integrates Hamilton's
equations through curved spacetime. No other line needs to be written.

Longer form, where a sentence fits: *Real-time Kerr black hole ray tracer.*

## The mark

A shadow and its photon ring. Two details are physics, not styling, and both
should survive any redraw:

1. **The ring is brighter on the right.** The disk orbits prograde, so the
   approaching side is beamed and blue-shifted. Specific intensity scales as
   `g³`, which is why one side of every render is visibly hotter. The mark's
   ring gradient runs dim ember on the left to near-white on the right.
2. **The shadow sits left of the ring's centre.** Frame dragging enters the
   photon paths through the `g^{tφ}` terms, and at high spin the shadow is
   measurably off-centre. The offset in the mark is 0.8 units in a 64-unit
   viewBox — enough to feel, not enough to look like a mistake.

Do not centre the shadow. Do not make the ring evenly bright. Those are the two
things that make it this project's mark rather than a generic eclipse.

## Colour

The palette is not chosen. It is `blackbody(T)` evaluated at the temperatures
the renderer works in, taken from the normalised Planckian locus in the main
README:

| Token | Hex | Where it comes from |
|---|---|---|
| Void | `#05060A` | The shadow. Rays that reach the capture surface contribute nothing. |
| Receding | `#8C4A08` | Ember at reduced `g` — the far side of the disk, dimmed by `g³`. |
| Ember | `#FF8A0D` | `blackbody(2000 K)` — the default peak disk temperature. The primary accent. |
| Amber | `#FF9E45` | `blackbody(2500 K)` |
| Tan | `#FFBD87` | `blackbody(3400 K)` |
| Photon | `#FFFFFA` | `blackbody(6500 K)` — the approaching inner edge, and body text on dark. |

Two supporting neutrals, which are interface rather than physics:

| Token | Hex | Use |
|---|---|---|
| Ink | `#14161A` | Text on light backgrounds. |
| Muted | `#8E9298` | Secondary text, captions, table rules. |

There is no blue, no purple and no second accent. The renderer produces a
one-dimensional colour ramp because a blackbody has one parameter, and the brand
inherits that constraint. If something needs to be distinguished, use value or
weight.

## Type

**Poppins Light**, uppercase, tracked wide, for the wordmark. Its circular
geometry belongs next to a circular mark, and Light keeps a large wordmark from
competing with the render behind it.

**A monospace face** — whatever the platform's `ui-monospace` resolves to — for
the tagline, HUD values, and anything numeric. Numbers in this project are
measurements; they should look like it.

Body text is the platform's default sans. Nothing is served from a font CDN.

## Assets

| File | Use |
|---|---|
| `mark.svg` | Icon, favicon, avatar. Legible to 16 px; verified at 32. |
| `logo-on-dark.svg` | Horizontal lockup for dark backgrounds. |
| `logo-on-light.svg` | Horizontal lockup for light backgrounds. |
| `social-card.png` | GitHub social preview, 1280×640. Upload under *Settings → Social preview*. |
| `make_social_card.py` | Regenerates the card. Requires Pillow. |

Pair the two lockups with `<picture>` and `prefers-color-scheme` rather than
picking one and hoping.

### Clear space and minimum size

Clear space on all sides is the radius of the mark's shadow — in the 372-unit
lockup, 18 units. The lockup should not be set below 220 px wide; below that use
`mark.svg` alone.

### What not to do

- Do not recolour the mark. The ramp is a physical curve, not a palette swatch.
- Do not put the lockup on a mid-tone background. It is built for `#05060A` or
  for white, and the two files exist so you never have to compromise.
- Do not add a glow, bevel or drop shadow. The halo in the mark is already the
  bloom, and it is doing the job.
- Do not stretch. The circles are circles.

## Licence

The mark, wordmark and card are part of this repository and carry the same MIT
licence as the code. Use them to refer to Kerrscope. Do not use them to imply
that a fork is this project.
