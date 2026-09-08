#!/usr/bin/env python3
"""Regenerate brand/social-card.png.

GitHub's social preview wants a raster image, so this is the one brand asset
that cannot be an SVG. It is generated rather than hand-composed so the card
can be rebuilt when the render behind it is replaced.

    pip install pillow
    python3 brand/make_social_card.py

The backdrop is a real frame out of the renderer, not an illustration of one.
That is the point of the card.
"""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs" / "orbiting_bodies_lensed.png"
OUT = ROOT / "brand" / "social-card.png"

W, H = 1280, 640

VOID = (5, 6, 10)
PHOTON = (242, 240, 236)
EMBER = (255, 138, 13)
MUTED = (150, 150, 156)

# Poppins is the wordmark face; the fallbacks keep the script runnable on a
# machine without it, at the cost of the tracking looking slightly different.
FACES = {
    "wordmark": [
        "/usr/share/fonts/truetype/google-fonts/Poppins-Light.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ],
    "mono": [
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    ],
}


def load(kind: str, size: int) -> ImageFont.FreeTypeFont:
    for path in FACES[kind]:
        if Path(path).exists():
            return ImageFont.truetype(path, size)
    raise SystemExit(f"no font found for {kind}; install fonts-poppins or dejavu")


def tracked(draw: ImageDraw.ImageDraw, xy, text: str, font, fill, tracking: float) -> None:
    """Draw text with extra letter spacing, which PIL does not do natively."""
    x, y = xy
    for ch in text:
        draw.text((x, y), ch, font=font, fill=fill)
        x += draw.textlength(ch, font=font) + tracking


def backdrop() -> Image.Image:
    """Scale the render so the shadow lands right of centre, leaving the left
    third for type."""
    src = Image.open(SOURCE).convert("RGB")
    scaled = src.resize((1600, int(src.height * 1600 / src.width)), Image.LANCZOS)
    top = (scaled.height - H) // 2
    return scaled.crop((0, top, W, top + H))


def scrim(image: Image.Image) -> Image.Image:
    """Fade the left side to the void colour so the wordmark has contrast
    without a box drawn around it."""
    mask = Image.new("L", (W, 1))
    for x in range(W):
        t = min(max((x - 120) / 620.0, 0.0), 1.0)
        # smoothstep, so the falloff has no visible edge
        mask.putpixel((x, 0), int(255 * (1 - t * t * (3 - 2 * t)) * 0.96))
    veil = Image.new("RGB", (W, H), VOID)
    return Image.composite(veil, image, mask.resize((W, H)))


def main() -> None:
    card = scrim(backdrop())
    draw = ImageDraw.Draw(card)

    tracked(draw, (84, 236), "KERRSCOPE", load("wordmark", 82), PHOTON, 11.0)
    tracked(draw, (90, 352), "one geodesic per pixel", load("mono", 27), EMBER, 1.2)

    small = load("mono", 19)
    for i, line in enumerate(
        ["Real-time Kerr black hole ray tracer",
         "OpenGL 3.3  ·  no compute shaders  ·  MIT"]):
        draw.text((90, 432 + i * 30), line, font=small, fill=MUTED)

    draw.line([(84, 238), (84, 306)], fill=EMBER, width=3)

    card.save(OUT, optimize=True)
    print(f"wrote {OUT.relative_to(ROOT)} ({W}x{H})")


if __name__ == "__main__":
    main()
