# Moonlight TV

Moonlight TV is a GUI front end for [Moonlight GameStream Client](https://moonlight-stream.org/). With some components from [moonlight-embedded](https://github.com/irtimmer/moonlight-embedded).
It was originally designed for LG webOS TVs, but may support running on more devices in the future.

## Features

* Supports up to 4 controllers
* Utilizes system hardware decoder to get best performance (webOS 2/3/4/5)
* Easy to port to other OSes (Now runs on macOS, Arch, Debian, Raspbian and Windows)

## Screenshots

![Launcher](https://user-images.githubusercontent.com/830358/135492540-5dae06fc-9653-4ff5-a6f3-c714ba0b58ac.png)

![Settings](https://user-images.githubusercontent.com/830358/135492547-6f9789a0-ae8a-42c9-ac11-7ecf921c83a0.png)

![In-game Overlay](https://user-images.githubusercontent.com/830358/135492550-fb3a3e4f-4835-4fde-93d7-c2761e85a712.png)

## Download

### For webOS

[Easy installation with dev-manager-desktop](https://github.com/webosbrew/dev-manager-desktop) (recommended)

Or download IPK from [Latest release](https://github.com/mariotaku/moonlight-tv/releases/latest)

## [Installation Guide](https://github.com/mariotaku/moonlight-tv/wiki/Installation-Guide)

## [Compatibility Status](https://github.com/mariotaku/moonlight-tv/wiki/Compatibility-Status)

## [Gamepad Setup](https://github.com/mariotaku/moonlight-tv/wiki/Gamepad-Setup)

## (For developers) Building for webOS

 - Follow instructions [here](https://github.com/webosbrew/meta-lg-webos-ndk) to setup NDK
 - Create a directory e.g. `build` in project root directory, and `cd` into it.
 - run `cmake .. -DTARGET_WEBOS=ON`
 - run `make webos-package-moonlight` to get IPK package in `build` directory