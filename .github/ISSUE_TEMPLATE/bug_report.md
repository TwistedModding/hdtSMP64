---
name: Bug report
about: Create a report to help us improve
title: "[BUG] ... happens when ... "
labels: ""
assignees: ""
---

**Describe the bug**
A clear and concise description of what the bug is.

**To Reproduce**
Steps to reproduce the behavior:

1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

**Expected behavior**
A clear and concise description of what you expected to happen.

**Screenshots and videos**
If applicable, add screenshots or videos to help explain your problem.

**Desktop (please complete the following information):**

- OS: [e.g. Windows 10, Wine]

** FSMP**

- Installed version
- AVX: No_AVX, AVX, AVX2, AVX512?

**SKSE**

- installed version
  (You can check it in the bottom left part of the screen, when looking at your MCMs.)

**Skyrim**

- Installed version number
  (You can check it in the bottom left part of the screen, when looking at your MCMs.)

**Required files**

- **Attach the bug-report zip built by the in-game config menu.** In game, open the FSMP config menu (via the SKSE Menu Framework) → **Home** → **Bundle a bug report** → **Build report zip**. It automatically packs your `hdtSMP64.log` and your configuration (`configs.json` + `userConfigs.json`). Also tick **Last validation report** and/or **Last crash log** if your problem involves broken physics assets or a crash. The zip is saved in your SKSE logs folder (the folder button next to the build button opens it).
- If you can't build the zip (e.g. the SKSE Menu Framework isn't installed), attach your `hdtSMP64.log` and `configs.json` / `userConfigs.json` manually instead.

**Additional context**
Add any other context about the problem here.
