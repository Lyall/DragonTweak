# DragonTweak

**DragonTweak** is an ASI plugin for Dragon Engine games that can:
- Disable pillarboxing/letterboxing.
- Adjust shadow resolution.
- Skip intro logos.

### Current Features 

| Game                                              | Disable Pillarboxing/Letterboxing | Adjust Shadow Resolution | Skip Intro Logos |
|---------------------------------------------------|-----------------------------------|--------------------------|------------------|
| Yakuza 6: The Song of Life                        | ✔️                               | ✔️                      | ✔️              |
| Yakuza Kiwami 2                                   | ✔️                               | ✔️                      | ✔️              |
| Judgment                                          | ✔️                               | ✔️                      | ✔️              |
| Yakuza: Like A Dragon                             | ✔️                               | ✔️                      | ✔️              |
| Lost Judgment                                     | ✔️                               | ✔️                      | ✔️              |
| Like a Dragon Gaiden: The Man Who Erased His Name | ✔️                               | ✔️                      | ✔️              |
| Like a Dragon: Infinite Wealth                    | ✔️                               | ✔️                      | ✔️              |
| Like a Dragon: Pirate Yakuza in Hawaii (Demo)     | ✔️                               | ✔️                      | ✔️              |

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
- Yakuza 6/Kiwami 2 use a different folder structure so remember to make sure that **`DragonTweak.asi`**, **`DragonTweak.ini`** and **`dinput8.dll`** are in the same folder as **`Yakuza6.exe`**.

### Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
