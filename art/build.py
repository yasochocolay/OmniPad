#!/usr/bin/env python3
"""Compose OmniPad artwork SVGs from mark.svgpart and render them with rsvg-convert."""
import os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out")  # generated, not committed
RSVG = os.path.expanduser("~/ps4-tools/extra/bin/rsvg-convert")
FONT_DIR = sys.argv[1]  # folder containing the OFL app font (Gontserrat)
os.makedirs(OUT, exist_ok=True)

mark = open(os.path.join(HERE, "mark.svgpart")).read()

def bg_defs(w, h, cx, cy, r):
    return f"""
  <defs>
    <radialGradient id="bg" gradientUnits="userSpaceOnUse" cx="{cx}" cy="{cy}" r="{r}">
      <stop offset="0" stop-color="#3A2A9A"/>
      <stop offset="0.55" stop-color="#1A1550"/>
      <stop offset="1" stop-color="#0A0920"/>
    </radialGradient>
  </defs>
  <rect width="{w}" height="{h}" fill="url(#bg)"/>"""

def write(name, w, h, body):
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">{body}\n</svg>\n'
    p = os.path.join(OUT, name + ".svg")
    open(p, "w").write(svg)
    return p

def render(svg, png, w, h):
    env = dict(os.environ)
    conf = os.path.join(OUT, "fonts.conf")
    open(conf, "w").write(f'<?xml version="1.0"?><!DOCTYPE fontconfig SYSTEM "fonts.dtd"><fontconfig>'
                          f'<dir>{FONT_DIR}</dir><cachedir>{OUT}/fc-cache</cachedir></fontconfig>')
    env["FONTCONFIG_FILE"] = conf
    env["PANGOCAIRO_BACKEND"] = "fc"  # use the bundled OFL font, never a system font
    subprocess.run([RSVG, "-w", str(w), "-h", str(h), "-o", os.path.join(OUT, png), svg], check=True, env=env)
    print("rendered", png)

# 1. PS4 tile icon: 512x512, opaque
p = write("icon0", 512, 512, bg_defs(512, 512, 256, 200, 380) + "\n" + mark)
render(p, "icon0.png", 512, 512)

# 2. In-app header logo: 96x96, transparent (mark only, cropped to the orbit)
p = write("app_logo", 512, 512, f'<g transform="translate(256 256) scale(1.12) translate(-256 -272)">{mark}</g>')
render(p, "app_logo.png", 96, 96)

# 3. PS4 home background (pic0): 1920x1080, mark on the right, wordmark on the left
title = """
  <text x="170" y="560" font-family="Gontserrat" font-size="150" font-weight="bold" fill="#FFFFFF" letter-spacing="2">Omni<tspan fill="#22E1FF">Pad</tspan></text>
  <text x="176" y="640" font-family="Gontserrat" font-size="46" fill="#B9B6E8">Any controller. One console.</text>"""
p = write("pic0", 1920, 1080, bg_defs(1920, 1080, 1350, 470, 1200) +
          f'\n  <g transform="translate(1020 150) scale(1.45)">{mark}</g>' + title)
render(p, "pic0.png", 1920, 1080)

# 4. In-app background (behind UI cards): darker, faint orbit only, no text
faint = """
  <g opacity="0.22" transform="rotate(-16 1300 560)">
    <ellipse cx="1300" cy="560" rx="760" ry="300" fill="none" stroke="#7B6CFF" stroke-width="6"/>
    <ellipse cx="1300" cy="560" rx="560" ry="210" fill="none" stroke="#22E1FF" stroke-width="3"/>
  </g>
  <rect width="1920" height="1080" fill="#05040F" opacity="0.35"/>"""
p = write("bg_native", 1920, 1080, bg_defs(1920, 1080, 1300, 420, 1300) + faint)
render(p, "bg_native.png", 1920, 1080)
