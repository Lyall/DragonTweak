## ⚠️ This repository has been migrated
Active development has moved to [Codeberg](https://codeberg.org/Lyall/DragonTweak).  
This repo is archived and will no longer receive updates.

# DragonTweak
[![Patreon-Button](https://github.com/Lyall/DragonTweak/blob/main/.github/Patreon-Button.png?raw=true)](https://www.patreon.com/Wintermance) 
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/DragonTweak/total.svg)](https://github.com/Lyall/DragonTweak/releases)

**DragonTweak** is an ASI plugin for Dragon Engine games that can:
- Disable pillarboxing/letterboxing.
- Adjust shadow quality.
- Adjust level of detail (LOD).
- Skip intro logos.

### Current Features 
| Game                                              | Disable Pillarboxing/Letterboxing | Adjust Shadow Quality    | Adjust LOD | Skip Intro Logos |
|---------------------------------------------------|-----------------------------------|--------------------------|------------|------------------|
| Like a Dragon: Pirate Yakuza in Hawaii            | ✔️                               | ✔️                       | ✔️         | ✔️              |
| Like a Dragon: Infinite Wealth                    | ✔️                               | ✔️                       | ✔️         | ✔️              |
| Like a Dragon Gaiden: The Man Who Erased His Name | ✔️                               | ✔️                       | ✔️         | ✔️              |
| Lost Judgment                                     | ✔️                               | ✔️                       | ✔️         | ✔️              |
| Yakuza: Like A Dragon                             | ✔️                               | ✔️                       | ❌         | ✔️              |
| Judgment                                          | ✔️                               | ✔️                       | ❌         | ✔️              |
| Yakuza Kiwami 2                                   | ✔️                               | ✔️                       | ❌         | ✔️              |
| Yakuza 6: The Song of Life                        | ✔️                               | ✔️                       | ❌         | ✔️              |

## Installation  
- Download the latest release from [here](https://github.com/Lyall/DragonTweak/releases). 
- Extract the contents of the release zip in to the the game folder.  

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**  
- Open the game properties in Steam and add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the launch options.  

## Configuration
- Open **`DragonTweak.ini`** to adjust settings.

## Troubleshooting
- If the ASI loader name **`dinput8.dll`** is incompatible with your configuration, please refer to this [list of possible DLL proxies](https://github.com/ThirteenAG/Ultimate-ASI-Loader#description).
- **Yakuza 6 & Kiwami 2** use a different folder structure so remember to make sure that **`DragonTweak.asi`**, **`DragonTweak.ini`** and **`dinput8.dll`** are in the same folder as **`Yakuza6.exe`** or **`YakuzaKiwami2.exe`**.

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
