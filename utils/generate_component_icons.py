"""
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
"""

import argparse
import math
from pathlib import Path

from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont


FIRST_CODEPOINT = 0xA1
COMPONENTS = (
    "hierarchy", "guid", "transform", "name", "world_script", "canvas",
    "mesh_renderer", "sprite_renderer", "particle_emitter", "skinned_mesh_renderer",
    "view_model", "ragdoll", "animation_player", "animation_graph", "animation_library",
    "audio_source", "audio_listener", "camera", "light", "post_process", "environment",
    "fog", "reflection_probe", "prefab_reference", "entity_tags", "physical",
    "compound_shape", "fixed_constraint", "distance_constraint", "point_constraint",
    "hinge_constraint", "cone_constraint", "slider_constraint", "swing_twist_constraint",
    "six_dof_constraint", "pulley_constraint", "vehicle_constraint", "character_mover",
    "debug_widgets", "alive", "disabled", "destroyer", "no_serialize", "generic",
)


class Icon:
    def __init__(self):
        self.pen = TTGlyphPen(None)

    def polygon(self, points, reverse=False):
        area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(points, points[1:] + points[:1]))
        if (area < 0) != reverse:
            points = list(reversed(points))
        scaled = [(round(x * 1000 / 24), round((24 - y) * 1000 / 24)) for x, y in points]
        self.pen.moveTo(scaled[0])
        for point in scaled[1:]:
            self.pen.lineTo(point)
        self.pen.closePath()
        return self

    def disc(self, x, y, radius):
        return self.polygon([(x + radius * math.cos(i * math.tau / 24), y + radius * math.sin(i * math.tau / 24)) for i in range(24)])

    def line(self, *points, width=1.6, closed=False):
        pairs = list(zip(points, points[1:] + (points[:1] if closed else ())))
        for (x0, y0), (x1, y1) in pairs:
            length = math.hypot(x1 - x0, y1 - y0)
            nx, ny = -(y1 - y0) / length * width / 2, (x1 - x0) / length * width / 2
            self.polygon([(x0 + nx, y0 + ny), (x1 + nx, y1 + ny), (x1 - nx, y1 - ny), (x0 - nx, y0 - ny)])
        for x, y in points:
            self.disc(x, y, width / 2)
        return self

    def box(self, x, y, w, h):
        return self.line((x, y), (x + w, y), (x + w, y + h), (x, y + h), closed=True)

    def ring(self, x, y, radius, width=1.6):
        for r, reverse in ((radius + width / 2, False), (radius - width / 2, True)):
            self.polygon([(x + r * math.cos(i * math.tau / 48), y + r * math.sin(i * math.tau / 48)) for i in range(48)], reverse)
        return self

    def arc(self, x, y, rx, ry, start, end, width=1.6):
        steps = max(4, round(abs(end - start) / 12))
        points = [(x + rx * math.cos(math.radians(start + (end - start) * i / steps)), y + ry * math.sin(math.radians(start + (end - start) * i / steps))) for i in range(steps + 1)]
        return self.line(*points, width=width)

    def arrow(self, x0, y0, x1, y1, size=2.5):
        angle = math.atan2(y1 - y0, x1 - x0)
        self.line((x0, y0), (x1, y1))
        return self.line((x1 - size * math.cos(angle - 0.6), y1 - size * math.sin(angle - 0.6)), (x1, y1), (x1 - size * math.cos(angle + 0.6), y1 - size * math.sin(angle + 0.6)))

    def cube(self, x=12, y=12, size=8):
        self.line((x, y - size), (x + size, y - size / 2), (x + size, y + size / 2), (x, y + size), (x - size, y + size / 2), (x - size, y - size / 2), closed=True)
        return self.line((x - size, y - size / 2), (x, y), (x + size, y - size / 2)).line((x, y), (x, y + size))

    def play(self, x=10, y=8, size=8):
        return self.polygon([(x, y), (x + size * 0.8, y + size / 2), (x, y + size)])


def build_icons():
    icons = {name: Icon() for name in COMPONENTS}
    icons["hierarchy"].box(9, 3, 6, 5).line((12, 8), (12, 12)).line((5, 15), (5, 12), (19, 12), (19, 15)).box(2.5, 15, 5, 5).box(16.5, 15, 5, 5)
    icons["guid"].line((9, 3), (6, 21)).line((18, 3), (15, 21)).line((4, 8), (21, 8)).line((3, 16), (20, 16))
    icons["transform"].arrow(7, 17, 7, 3).arrow(7, 17, 21, 17).arrow(7, 17, 16, 8).disc(7, 17, 2)
    icons["name"].line((3, 6), (3, 4), (15, 4), (15, 6)).line((9, 4), (9, 20)).line((6, 20), (12, 20)).line((18, 8), (21, 8)).line((19.5, 8), (19.5, 20)).line((18, 20), (21, 20))
    icons["world_script"].line((7, 6), (2, 12), (7, 18)).line((17, 6), (22, 12), (17, 18)).line((14, 3), (10, 21))
    icons["canvas"].box(3, 4, 18, 16).line((3, 8), (21, 8)).line((9, 8), (9, 20)).line((12, 12), (18, 12)).line((12, 16), (16, 16))
    icons["mesh_renderer"].cube()
    icons["sprite_renderer"].box(3, 3, 18, 18).disc(8, 8, 1.8).line((3, 18), (10, 11), (14, 15), (17, 12), (21, 16))
    icons["particle_emitter"].polygon([(12, 5), (14, 10), (19, 12), (14, 14), (12, 19), (10, 14), (5, 12), (10, 10)]).disc(4, 4, 1.7).disc(20, 4, 1.2).disc(20, 20, 1.8).disc(4, 20, 1.2)
    icons["skinned_mesh_renderer"].line((7, 4), (17, 4), (21, 12), (17, 20), (7, 20), (3, 12), closed=True).line((7, 4), (12, 10), (17, 4)).line((7, 20), (12, 14), (17, 20)).line((3, 12), (10, 12)).line((14, 12), (21, 12)).ring(12, 12, 2)
    icons["view_model"].line((2, 12), (7, 7), (17, 7), (22, 12), (17, 17), (7, 17), closed=True).ring(12, 12, 3).line((12, 2), (12, 5)).line((12, 19), (12, 22))
    icons["ragdoll"].ring(13, 4, 2).line((12, 8), (10, 14), (6, 19), (3, 19)).line((10, 14), (16, 17), (18, 21)).line((12, 9), (7, 10), (4, 6)).line((12, 9), (17, 12), (21, 9))
    icons["animation_player"].box(3, 4, 18, 16).play().line((6, 4), (6, 20)).line((18, 4), (18, 20))
    icons["animation_graph"].box(2, 8, 6, 8).play(3.5, 10, 4).line((8, 12), (13, 12), (13, 5), (16, 5)).line((13, 12), (13, 19), (16, 19)).box(16, 2, 6, 6).box(16, 16, 6, 6)
    icons["animation_library"].box(3, 4, 4, 17).box(10, 4, 11, 17).line((3, 8), (7, 8)).line((10, 4), (10, 2), (19, 2)).play(13, 9, 6)
    icons["audio_source"].line((3, 9), (7, 9), (12, 4), (12, 20), (7, 15), (3, 15), closed=True).arc(12, 12, 5, 5, -55, 55).arc(12, 12, 9, 9, -55, 55)
    icons["audio_listener"].arc(12, 12, 9, 9, 180, 360).box(3, 11, 4, 8).box(17, 11, 4, 8).line((17, 19), (15, 21), (12, 21))
    icons["camera"].line((3, 7), (7, 7), (9, 4), (15, 4), (17, 7), (21, 7), (21, 20), (3, 20), closed=True).ring(12, 13, 4).disc(18, 10, 0.9)
    icons["light"].arc(12, 10, 5, 5, 140, 400).line((8.2, 13.2), (9, 17), (15, 17), (15.8, 13.2)).line((10, 21), (14, 21)).line((12, 1.5), (12, 2.5)).line((3, 10), (1.5, 10)).line((21, 10), (22.5, 10)).line((5, 3), (4, 2)).line((19, 3), (20, 2))
    icons["post_process"].line((3, 6), (7, 6)).line((11, 6), (21, 6)).ring(9, 6, 2).line((3, 12), (13, 12)).line((17, 12), (21, 12)).ring(15, 12, 2).line((3, 18), (5, 18)).line((9, 18), (21, 18)).ring(7, 18, 2)
    icons["environment"].line((2, 20), (9, 8), (14, 15), (18, 10), (22, 20), closed=True).line((6, 13), (9, 15), (12, 13)).ring(17, 5, 2.5)
    icons["fog"].line((3, 5), (14, 5)).line((18, 5), (21, 5)).line((6, 10), (21, 10)).line((3, 15), (17, 15)).line((5, 20), (10, 20)).line((14, 20), (21, 20))
    icons["reflection_probe"].ring(12, 12, 7).arc(12, 12, 3, 7, 0, 360, 1.3).line((5, 12), (19, 12)).line((12, 1), (12, 3)).line((12, 21), (12, 23)).line((1, 12), (3, 12)).line((21, 12), (23, 12))
    icons["prefab_reference"].cube(9, 10, 6).line((16, 14), (21, 14), (21, 21), (14, 21), (14, 18)).arrow(13, 16, 18, 16, 2)
    icons["entity_tags"].line((3, 3), (12, 3), (22, 13), (13, 22), (3, 12), closed=True).ring(8, 8, 1.5)
    icons["physical"].cube(9, 9, 5.5).arrow(19, 4, 19, 16).line((3, 21), (21, 21)).line((6, 18), (12, 18))
    icons["compound_shape"].box(3, 3, 8, 8).ring(17, 8, 4).line((4, 21), (9, 13), (14, 21), closed=True).box(16, 15, 5, 6)
    icons["fixed_constraint"].box(2, 8, 6, 8).box(16, 8, 6, 8).line((8, 12), (16, 12)).box(9, 4, 6, 5).arc(12, 4, 2, 2, 180, 360)
    icons["distance_constraint"].ring(4, 12, 2).ring(20, 12, 2).arrow(12, 12, 8, 12, 2).arrow(12, 12, 16, 12, 2).line((4, 5), (4, 7)).line((20, 5), (20, 7)).line((4, 5), (20, 5))
    icons["point_constraint"].ring(12, 12, 3).line((3, 3), (9, 9)).line((15, 9), (21, 3)).line((3, 21), (9, 15)).line((15, 15), (21, 21)).disc(12, 12, 1)
    icons["hinge_constraint"].box(3, 5, 6, 14).line((12, 3), (12, 21)).line((15, 7), (21, 4), (21, 18), (15, 21), closed=True).line((9, 8), (12, 8)).line((9, 16), (12, 16))
    icons["cone_constraint"].line((12, 3), (3, 19)).line((12, 3), (21, 19)).arc(12, 19, 9, 2, 0, 360).line((12, 8), (12, 14)).disc(12, 3, 1.5)
    icons["slider_constraint"].line((3, 8), (21, 8)).line((3, 16), (21, 16)).box(8, 6, 8, 12).arrow(9, 3, 3, 3, 2).arrow(15, 21, 21, 21, 2)
    icons["swing_twist_constraint"].line((12, 3), (12, 21)).ring(12, 12, 2).arc(12, 12, 9, 4, 20, 340).line((20, 9), (20.5, 11), (18, 10.5)).arc(12, 12, 5, 9, 210, 330)
    icons["six_dof_constraint"].arrow(12, 12, 12, 2).arrow(12, 12, 12, 22).arrow(12, 12, 2, 8).arrow(12, 12, 22, 16).arrow(12, 12, 2, 18).arrow(12, 12, 22, 6)
    icons["pulley_constraint"].ring(12, 7, 4).disc(12, 7, 1).line((8, 7), (8, 17)).line((16, 7), (16, 14)).box(5, 17, 6, 5).box(13, 14, 6, 5).line((12, 3), (12, 1))
    icons["vehicle_constraint"].line((3, 16), (3, 10), (6, 10), (8, 5), (17, 5), (20, 10), (21, 10), (21, 16), closed=True).line((6, 10), (20, 10)).ring(7, 17, 2.5).ring(18, 17, 2.5)
    icons["character_mover"].arc(9, 7, 4, 4, 180, 360).line((5, 7), (5, 16)).arc(9, 16, 4, 4, 0, 180).line((13, 16), (13, 7)).arrow(16, 12, 22, 12).line((3, 23), (15, 23))
    icons["debug_widgets"].box(7, 7, 10, 13).arc(12, 7, 3, 3, 180, 360).line((12, 8), (12, 20)).line((3, 8), (7, 11)).line((3, 14), (7, 14)).line((3, 20), (7, 17)).line((17, 11), (21, 8)).line((17, 14), (21, 14)).line((17, 17), (21, 20))
    icons["alive"].line((2, 12), (6, 12), (9, 4), (14, 20), (17, 12), (22, 12))
    icons["disabled"].ring(12, 12, 9).line((6, 18), (18, 6))
    icons["destroyer"].line((4, 6), (20, 6)).line((9, 3), (15, 3)).line((6, 6), (7, 21), (17, 21), (18, 6)).line((10, 10), (10, 17)).line((14, 10), (14, 17))
    icons["no_serialize"].line((3, 3), (17, 3), (21, 7), (21, 21), (3, 21), closed=True).box(7, 3, 8, 6).box(7, 14, 10, 7).line((2, 22), (22, 2), width=2)
    icons["generic"].box(6, 6, 12, 12).line((9, 12), (15, 12)).line((12, 9), (12, 15))
    for position in (8, 16):
        icons["generic"].line((position, 2), (position, 6)).line((position, 18), (position, 22)).line((2, position), (6, position)).line((18, position), (22, position))
    return icons


def update_font(path):
    font = TTFont(path, recalcTimestamp=False)
    order = font.getGlyphOrder()
    icons = build_icons()
    component_glyphs = {f"component_{component}" for component in COMPONENTS}
    for table in font["cmap"].tables:
        for codepoint, name in list(table.cmap.items()):
            if name in component_glyphs:
                del table.cmap[codepoint]

    for index, component in enumerate(COMPONENTS):
        codepoint = FIRST_CODEPOINT + index
        name = f"component_{component}"
        for table in font["cmap"].tables:
            if table.isUnicode():
                encoded = codepoint
            elif table.platformID == 1 and table.platEncID == 0:
                try:
                    encoded = chr(codepoint).encode("mac_roman")[0]
                except UnicodeEncodeError:
                    continue
            else:
                continue

            existing = table.cmap.get(encoded)
            if existing is not None and existing != name:
                raise ValueError(f"U+{codepoint:04X} is already assigned to {existing}")
            table.cmap[encoded] = name
        glyph = icons[component].pen.glyph()
        glyph.recalcBounds(font["glyf"])
        if name not in order:
            order.append(name)
        font["glyf"][name] = glyph
        font["hmtx"][name] = (1000, glyph.xMin)
    font.setGlyphOrder(order)
    font["OS/2"].updateFirstAndLastCharIndex(font)
    font["OS/2"].recalcUnicodeRanges(font)
    font.save(path)


def render_preview(path, output):
    from PIL import Image, ImageDraw, ImageFont

    scale = 2
    cell_w, cell_h = 210 * scale, 106 * scale
    image = Image.new("RGB", (6 * cell_w, math.ceil(len(COMPONENTS) / 6) * cell_h), (22, 24, 29))
    draw = ImageDraw.Draw(image)
    large = ImageFont.truetype(str(path), 40 * scale)
    small = ImageFont.truetype(str(path), 16 * scale)
    label = ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", 11 * scale)
    for index, name in enumerate(COMPONENTS):
        x, y = (index % 6) * cell_w, (index // 6) * cell_h
        draw.rounded_rectangle((x + 8 * scale, y + 6 * scale, x + cell_w - 8 * scale, y + cell_h - 6 * scale), 8 * scale, fill=(32, 35, 41))
        text = chr(FIRST_CODEPOINT + index)
        draw.text((x + cell_w / 2 - 18 * scale, y + 35 * scale), text, font=large, fill=(213, 224, 237), anchor="mm")
        draw.text((x + cell_w / 2 + 30 * scale, y + 35 * scale), text, font=small, fill=(90, 190, 255), anchor="mm")
        draw.text((x + cell_w / 2, y + 78 * scale), name.replace("_", " "), font=label, fill=(174, 184, 198), anchor="mm")
    image.save(output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", type=Path, default=Path(__file__).resolve().parents[1] / "assets/editor/resource_pack/fonts/icons.ttf")
    parser.add_argument("--preview", type=Path)
    args = parser.parse_args()
    update_font(args.font)
    if args.preview:
        render_preview(args.font, args.preview)
    print(f"Updated {len(COMPONENTS)} glyphs: U+{FIRST_CODEPOINT:04X}–U+{FIRST_CODEPOINT + len(COMPONENTS) - 1:04X}")
