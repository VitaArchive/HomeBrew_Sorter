# XMB Title Sorter (a.k.a. HomeBrew Sorter Plus)

![TitleSorter](resources/titlesorter.png)

This is a fork of the HomeBrew Sorter GUI.

A homebrew application that allows to manually sort the order that the titles will be shown in the PSP XMB (VSH).

## File extensions supported

- ISO
- CSO
- ZSO
- JSO
- DAX
- PBP (EBOOT)

## Current detected paths

- `ms0:/PSP/GAME`
- `ms0:/PSP/GAME/PSX`
- `ms0:/PSP/GAME/Utility`
- `ms0:/PSP/GAME150`
- `ms0:/ISO`
- `ms0:/ISO/PSP`
- `ms0:/ISO/VIDEO`

## Game Categories plugin support

This homebrew has support for the [Game Categories plugin](https://github.com/ticky/game-categories-lite) to sort the titles in the categories folders.

The categories with the same name are now merged, just like plugin does.

### Browser mode support

The Browser mode sorts titles from oldest to newest and uses EBOOT.PBP's date and time. The support for this mode consists on changing that dates so the top list title will have the oldest date and the bottom list title the newest. This is an option which is disabled by default, as it would modify the EBOOT.PBP's properties (the other two modes game categories plugin offers sort from newest to oldest by the folders date properties).

While enabled, the sorting will be made for both folders and EBOOT's properties, so you will have the same sort order in any mode. Keep in mind that Browser mode sorts categories alphabetically, so even if you sort the categories from Title Sorter that won't change.

Also, if you already sorted your homebrew and found it didn't work for Browser mode, just enter Title Sorter, enable Browser mode and enter each category and save the list.

---

## Maybe-goals

- [ ] Auto sorting alphabetically by title name
- [ ] Automatically detect paths in `/ISO` and `/PSP/GAME`
- [ ] Auto scrolling when the file name is too long
- [ ] Battery percentage indicator
- [ ] New icons/images

## Build Dependencies

- [pspsdk](https://pspdev.github.io/)
- [oslib](https://github.com/dogo/oslib)

## Runtime Dependencies

- A (e)CFW that its SystemControl module has support for the following functions:
  - `sctrlDeflateDecompress`
  - `lzo1x_decompress`
  - `LZ4_decompress_fast`

> (e)CFW known to have those are: PRO, ARK and Adrenaline (isage's fork).

## Building

- `make`
- Grab `EBOOT.PBP` and place it on `??0:/PSP/GAME/TITLESORTER/`
	- `??0:` is `ms0:` or `ef0:`, depending on your device and setup

## Tested on

- [Adrenaline eCFW](https://github.com/isage/Adrenaline) (Nightly)
- [ARK-4 (e)CFW](https://github.com/PSP-Archive/ARK-4)
- [ARK-5 (e)CFW](https://github.com/PSP-Arkfive/ARK-5)

## Licensing

This software is licensed under the [GNU General Public License, v. 2.0](./LICENSE)
(GPLv2). If a copy of the GPLv2 was not distributed with this file, you can obtain
one at [this link](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html.en).

## Credits

- HomeBrew Sorter by Sakya
- GUI & compatibility with game categories by Valantin and suloku
- oslib ([GitHub](https://github.com/dogo/oslib))