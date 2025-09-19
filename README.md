# HomeBrew Sorter

![HBSorter](resources/hbsorter.png)

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

## Maybe-goals

- [ ] Automatically detect paths in `/ISO` and `/PSP/GAME`
- [ ] Auto scrolling when the file name is too long
- [ ] Battery percentage indicator
- [ ] New icons/images

## Dependencies

- [pspsdk](https://pspdev.github.io/)
- [oslib](https://github.com/dogo/oslib)

## Building

- `make`
- Grab `EBOOT.PBP` and place it on `??0:/PSP/GAME/HBSORTER/`
	- `??0:` is `ms0:` or `ef0:`, depending on your device and setup

## Tested on:

- Adrenaline eCFW
- ARK-4 (e)CFW

## Credits

- HomeBrew Sorter by Sakya
- GUI & compatibility with game categories by Valantin and suloku
- oslib ([GitHub](https://github.com/dogo/oslib))