# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Site Is

A Jekyll-based GitHub Pages portfolio documenting the **Gametoy project** — a DIY woodbox with LEDs, buttons, buzzer, and display running Zephyr RTOS on an STM32. Deployed at https://adri1mart1.github.io/

## Development Commands

```bash
bundle install              # Install Ruby gem dependencies
bundle exec jekyll serve    # Local dev server (http://localhost:4000)
bundle exec jekyll build    # Build static site to _site/
```

## Architecture

**Single-page layout** — `index.html` uses the `front` layout (`_layouts/front.html`), which assembles the page from numbered includes in `_includes/`:

```
_includes/
  head.html         # metadata, CSS
  nav.html          # fixed navigation
  header.html       # hero section
  01-introduction.html
  02-object-design.html
  03-design-and-fabrication.html
  04-electronic.html
  05-software.html
  06-final-result.html
  contact.html      # footer
  scripts.html      # JS libraries
```

To add or edit content, modify the relevant numbered include file. Navigation links in `nav.html` correspond to section anchors in those files.

## Styling

- `css/main.scss` → imports from `_sass/_base.scss` and `_sass/_mixins.scss`
- `css/override.css` — project-specific overrides (use this for custom styles rather than editing `_base.scss`)
- Primary color: `#F05F40`; dark color: `#222`; fonts: Open Sans / Merriweather

## Remote Theme

`_config.yml` uses `remote_theme: volny/creative-theme-jekyll`. The base layout, Bootstrap 3, Font Awesome, and animation libraries come from this theme.

## Images

85+ project photos in `img/`. Filenames follow the pattern of process stage + subject (e.g. `soldering_01.jpg`, `3d_print_top.png`).
