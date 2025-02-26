#include "stdafx.h"
#include "helper.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <inipp/inipp.h>
#include <safetyhook.hpp>

#define spdlog_confparse(var) spdlog::info("Config Parse: {}: {}", #var, var)

HMODULE exeModule = GetModuleHandle(NULL);
HMODULE thisModule;

// Fix details
std::string sFixName = "DragonTweak";
std::string sFixVersion = "0.0.3";
std::filesystem::path sFixPath;

// Ini
inipp::Ini<char> ini;
std::string sConfigFile = sFixName + ".ini";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::string sLogFile = sFixName + ".log";
std::filesystem::path sExePath;
std::string sExeName;

// Ini variables
bool bIntroSkip;
int iShadowResolution;
bool bShadowDrawDistance;
bool bAdjustLOD;
bool bDisableBarsCutscene;
bool bDisableBarsGlobal;

// Variables
int iCurrentResX;
int iCurrentResY;

// Game info
struct GameInfo
{
    std::string GameTitle;
    std::string ExeName;
};

enum class Game 
{
    Unknown,
    OgreF,      // Yakuza 6: The Song of Life
    Lexus2,     // Yakuza Kiwami 2
    Judge,      // Judgment
    Yazawa,     // Yakuza: Like A Dragon
    Coyote,     // Lost Judgment
    Aston,      // Like a Dragon Gaiden: The Man Who Erased His Name
    Elvis,      // Like a Dragon: Infinite Wealth
    Sparrow     // Like a Dragon: Pirate Yakuza in Hawaii
};

const std::map<Game, GameInfo> kGames = {
    {Game::OgreF,   {"Yakuza 6: The Song of Life", "Yakuza6.exe"}},
    {Game::Lexus2,  {"Yakuza Kiwami 2", "YakuzaKiwami2.exe"}},
    {Game::Judge,   {"Judgment", "Judgment.exe"}},
    {Game::Yazawa,  {"Yakuza: Like a Dragon", "YakuzaLikeADragon.exe"}},
    {Game::Coyote,  {"Lost Judgment", "LostJudgment.exe"}},
    {Game::Aston,   {"Like a Dragon Gaiden: The Man Who Erased His Name", "LikeADragonGaiden.exe"}},
    {Game::Elvis,   {"Like a Dragon: Infinite Wealth", "LikeADragon8.exe"}},
    {Game::Sparrow, {"Like a Dragon: Pirate Yakuza in Hawaii", "LikeADragonPirates.exe"}},
};

const GameInfo* game = nullptr;
Game eGameType = Game::Unknown;

void Logging()
{
    // Get path to DLL
    WCHAR dllPath[_MAX_PATH] = {0};
    GetModuleFileNameW(thisModule, dllPath, MAX_PATH);
    sFixPath = dllPath;
    sFixPath = sFixPath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = {0};
    GetModuleFileNameW(exeModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // Spdlog initialisation
    try
    {
        logger = spdlog::basic_logger_st(sFixName, sExePath.string() + sLogFile, true);
        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::debug);

        spdlog::info("----------");
        spdlog::info("{:s} v{:s} loaded.", sFixName, sFixVersion);
        spdlog::info("----------");
        spdlog::info("Log file: {}", sFixPath.string() + sLogFile);
        spdlog::info("----------");
        spdlog::info("Module Name: {:s}", sExeName);
        spdlog::info("Module Path: {:s}", sExePath.string());
        spdlog::info("Module Address: 0x{:x}", (uintptr_t)exeModule);
        spdlog::info("Module Timestamp: {:d}", Memory::ModuleTimestamp(exeModule));
        spdlog::info("----------");
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        AllocConsole();
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        FreeLibraryAndExitThread(thisModule, 1);
    }
}

void Configuration()
{
    // Inipp initialisation
    std::ifstream iniFile(sFixPath / sConfigFile);
    if (!iniFile)
    {
        AllocConsole();
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVersion.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sFixPath.string().c_str() << std::endl;
        spdlog::error("ERROR: Could not locate config file {}", sConfigFile);
        spdlog::shutdown();
        FreeLibraryAndExitThread(thisModule, 1);
    }
    else
    {
        spdlog::info("Config file: {}", sFixPath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Parse config
    ini.strip_trailing_comments();
    spdlog::info("----------");

    // Load settings from ini
    inipp::get_value(ini.sections["Intro Skip"], "Enabled", bIntroSkip);
    inipp::get_value(ini.sections["Shadow Quality"], "Resolution", iShadowResolution);
    inipp::get_value(ini.sections["Shadow Quality"], "DrawDistance", bShadowDrawDistance);
    inipp::get_value(ini.sections["Adjust LOD"], "Enabled", bAdjustLOD);
    inipp::get_value(ini.sections["Disable Pillarboxing"], "CutscenesOnly", bDisableBarsCutscene);
    inipp::get_value(ini.sections["Disable Pillarboxing"], "AllScenes", bDisableBarsGlobal);

    // Clamp settings
    iShadowResolution = std::clamp(iShadowResolution, 64, 8192);

    // Log ini parse
    spdlog_confparse(bIntroSkip);
    spdlog_confparse(iShadowResolution);
    spdlog_confparse(bShadowDrawDistance);
    spdlog_confparse(bAdjustLOD);
    spdlog_confparse(bDisableBarsCutscene);
    spdlog_confparse(bDisableBarsGlobal);

    spdlog::info("----------");
}

bool DetectGame()
{
    eGameType = Game::Unknown;

    for (const auto& [type, info] : kGames)
    {
        if (Util::string_cmp_caseless(info.ExeName, sExeName))
        {
            spdlog::info("Detect Game: Detected: {} ({})", info.GameTitle, sExeName);
            eGameType = type;
            game = &info;
            return true;
        }
    }

    spdlog::error("Detect Game: Failed to detect supported game, {} isn't supported by DragonTweak.", sExeName);
    return false;

}

bool bHasSkippedIntro = false;
std::string sSceneID;
int iStageID;
const std::string sLexus2SkipID = "lexus2_studio_logo";
const std::string sOgreFSkipID  = "title_logo";
const std::string sYazawaSkipID = "title_logo";
const std::string sCoyoteSkipID = "title_photosensitive";
const std::string sAstonSkipID  = "title_photosensitive";
const std::string sJudgeSkipID  = "judge_photosensitive";

void IntroSkip()
{
    if (bIntroSkip)
    {
        if (eGameType == Game::Elvis || eGameType == Game::Sparrow)
        {
            // Intro skip
            std::uint8_t* IntroSkipScanResult = Memory::PatternScan(exeModule, "48 89 ?? ?? 31 ?? 48 89 ?? E8 ?? ?? ?? ?? 4C 8B ?? ??");
            if (IntroSkipScanResult)
            {
                spdlog::info("Intro Skip: Address: {:s}+0x{:x}", sExeName, IntroSkipScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid IntroSkipMidHook{};
                IntroSkipMidHook = safetyhook::create_mid(IntroSkipScanResult,
                    [](SafetyHookContext &ctx)
                    {
                        if (!bHasSkippedIntro)
                        {
                            spdlog::info("Intro Skip: Skipping intro logos.");
                            LPSTR launchArgs = (LPSTR)ctx.rsi;
                            static std::string sLaunchArgs = launchArgs;
                            sLaunchArgs += " -skiplogo";
                            ctx.rsi = reinterpret_cast<uintptr_t>(sLaunchArgs.c_str());
                            bHasSkippedIntro = true;
                        }
                    });
            }
            else
            {
                spdlog::error("Intro Skip: Pattern scan(s) failed.");
            }
        }
    
        if (eGameType != Game::Elvis && eGameType != Game::Sparrow) 
        {
            // create_config_scene
            std::vector<const char*> CreateConfigScenePatterns = {
                "?? 8B ?? 8B ?? 4C 8B ?? 8B ?? E8 ?? ?? ?? ?? 84 C0 75 ?? 45 33 ?? 45 89 ?? ?? E9 ?? ?? ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ??",   // Yakuza 6/Kiwami 2
                "49 8B ?? 8B ?? 4C 8B ?? 8B ?? E8 ?? ?? ?? ?? 84 ?? 75 ?? 33 ?? 41 ?? ?? E9 ?? ?? ?? ??",                                       // Lost Judgment/Gaiden
                "8B ?? 4C ?? ?? 85 ?? 0F 84 ?? ?? ?? ?? B9 ?? ?? 00 00 E8 ?? ?? ?? ?? 48 8B ?? 48 85 ??"                                        // LAD7/Judgment
            };
    
            std::uint8_t* CreateConfigSceneScanResult = Memory::MultiPatternScan(exeModule, CreateConfigScenePatterns);
            if (CreateConfigSceneScanResult)
            {
                static int iRunCount = 0;
                spdlog::info("Intro Skip: Create Config Scene: Address: {:s}+0x{:x}", sExeName, CreateConfigSceneScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid CreateConfigSceneMidHook{};
                CreateConfigSceneMidHook = safetyhook::create_mid(CreateConfigSceneScanResult,
                    [](SafetyHookContext &ctx)
                    {
                        if (ctx.rbx && ctx.r8 && ctx.rdi && !bHasSkippedIntro)
                        {
                            if (eGameType == Game::OgreF)
                                sSceneID = *reinterpret_cast<char**>(ctx.rdi + 0x10);
                            else if (eGameType == Game::Lexus2)
                                sSceneID = *reinterpret_cast<char**>(ctx.rbx + 0x08);
                            else
                                sSceneID = *reinterpret_cast<char**>(ctx.rbx + 0x10);
    
                            iStageID = *reinterpret_cast<int*>(ctx.r8 + 0x4);                      
                            spdlog::info("Intro Skip: Scene ID = {} (0x{:x}) | Stage: {:x}", sSceneID, ctx.rdx, iStageID);
                            
                            // Lost Judgment
                            if (eGameType == Game::Coyote && Util::string_cmp_caseless(sSceneID, sCoyoteSkipID))
                            {
                                ctx.rdx = 0x10D4; // Set to coyote_title
                                *reinterpret_cast<int*>(ctx.r8 + 0x4) = 0xF4; // Stage change!
                                bHasSkippedIntro = true;
                            }

                            // Yakuza: Like a Dragon
                            if (eGameType == Game::Yazawa && Util::string_cmp_caseless(sSceneID, sYazawaSkipID))
                            {
                                ctx.rdx = 0x2096; // Set ID to "yazawa_title"
                                *reinterpret_cast<int*>(ctx.r8 + 0x4) = 0xCF; // Stage change!
                                bHasSkippedIntro = true;
                            }

                            // Like a Dragon: Gaiden
                            if (eGameType == Game::Aston && Util::string_cmp_caseless(sSceneID, sAstonSkipID)) 
                            {
                                ctx.rdx = 0x177; // Set id to "aston_title"
                                *reinterpret_cast<int*>(ctx.r8 + 0x4) = 0xF4; // Stage change!
                                bHasSkippedIntro = true;
                            } 
                            
                            // Judgment
                            if (eGameType == Game::Judge && Util::string_cmp_caseless(sSceneID, sJudgeSkipID)) 
                            {
                                ctx.rdx = 0xC88; // Set id to "judge_title"
                                bHasSkippedIntro = true;
                            }
    
                            // Yakuza 6
                            if (eGameType == Game::OgreF && Util::string_cmp_caseless(sSceneID, sOgreFSkipID))
                            {
                                ctx.rdx = 0x62E; // Set id to "title"
                                bHasSkippedIntro = true;
                            }

                            // Yakuza Kiwami 2
                            if (eGameType == Game::Lexus2 && Util::string_cmp_caseless(sSceneID, sLexus2SkipID)) 
                            {
                                ctx.rdx = 0xE10; // Set id to "lexus2_title"
                                bHasSkippedIntro = true;
                            }

                            if (bHasSkippedIntro)
                                spdlog::info("Intro Skip: Skipped intro logos.");
                        }
                    });
            }
            else
            {
                spdlog::error("Intro Skip: Pattern scan(s) failed.");
            }
        }
    
        if (eGameType != Game::Aston && eGameType != Game::Coyote) 
        {
            // Press any key delay
            std::vector<const char*> PressAnyKeyDelayPatterns = {
                "84 C0 74 ?? C5 ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? 72 ?? 48 8B ?? ?? 48 85 ?? 74 ?? 48 C7 ?? ?? 00 00 00 00 BA 01 00 00 00",   // Yakuza 6
                "72 ?? 45 33 ?? 48 8B ?? 41 ?? ?? ?? E8 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? F3 0F ?? ?? ?? 48 83 ?? ?? 5B C3",   // Kiwami 2
                "72 ?? 48 8B ?? E8 ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? ?? C5 ?? ?? ?? ?? 48 83 ?? ?? 5B C3",                        // LAD7/Judgment
                "72 ?? 48 8B ?? E8 ?? ?? ?? ?? C5 ?? 10 ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? C5 ?? 11 ?? ??",                        // LAD8
                "72 ?? 48 8B ?? E8 ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? C5 ?? ?? ?? 48 8B ?? ?? ?? 48 83 ?? ?? ?? C3"                // Pirate
            };
    
            std::uint8_t* PressAnyKeyDelayScanResult = Memory::MultiPatternScan(exeModule, PressAnyKeyDelayPatterns);
            if (PressAnyKeyDelayScanResult)
            {
                // Remove delay on "press any key" appearing
                spdlog::info("Intro Skip: Press Any Key Delay: {:s}+0x{:x}", sExeName, PressAnyKeyDelayScanResult - (std::uint8_t*)exeModule);
                if (eGameType == Game::OgreF)
                    Memory::PatchBytes(PressAnyKeyDelayScanResult + 0x11, "\x90\x90", 2);
                else
                    Memory::PatchBytes(PressAnyKeyDelayScanResult, "\x90\x90", 2); 
            }
            else 
            {
                spdlog::error("Intro Skip: Press Any Key Delay: Pattern scan(s) failed.");
            }
        }
    }
}

std::uint8_t* MovieStatus = nullptr;

void DisablePillarboxing()
{
    if (bDisableBarsGlobal) 
    {
        // job_draw_bars() patterns
        std::vector<const char*> DrawBarsPatterns = {          
            "40 ?? ?? 41 ?? 48 8D ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 33 ?? 48 89 ?? ?? B9 ?? ?? ?? ??",                                                           // Pirate/LAD8
            "40 ?? 56 57 41 ?? 41 ?? 48 8D ?? ?? ?? ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 33 ?? 48 89 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 45 33 ?? BE ?? ?? ?? ??",  // Gaiden
            "40 ?? 48 8D ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 33 ?? 48 89 ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0",                                               // LAD7/Judgment
            "48 89 ?? ?? ?? 55 56 57 48 8D ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 48 8B ?? E8 ?? ?? ?? ?? 3B ?? ?? ?? ?? ?? 0F 83 ?? ?? ?? ??",                                               // Kiwami2
            "40 ?? ?? 41 ?? 41 ?? 41 ?? 48 83 ?? ?? 48 C7 ?? ?? ?? ?? ?? ?? ?? 48 89 ?? ?? ?? 48 89 ?? ?? ?? ?? ?? ?? 4C ?? ?? B9 08 00 00 00",                                      // Yakuza 6
            "40 ?? 57 41 ?? 41 ?? 41 ?? 48 8D ?? ?? ?? ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 33 ?? 48 89 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 45 33 ??"               // Lost Judgment
        };

        // All: Disable pillarboxing/letterboxing everywhere
        std::vector<std::uint8_t*> DrawBarsScanResults = Memory::MultiPatternScanAll(exeModule, DrawBarsPatterns);
        if (!DrawBarsScanResults.empty())
        {
            spdlog::info("Disable Pillarboxing/Letterboxing: Global: Found {} pattern match(es).", DrawBarsScanResults.size());
            for (auto& DrawBarsScanResult : DrawBarsScanResults) 
            {
                spdlog::info("Disable Pillarboxing/Letterboxing: Global: Address: {:s}+0x{:x}", sExeName, DrawBarsScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(DrawBarsScanResult, "\xC3\x90", 2);
            }
        }
        else
        {
            spdlog::error("Disable Pillarboxing/Letterboxing: Global: Pattern scan(s) failed.");
        }
    }

    if (bDisableBarsCutscene)
    {
        if (eGameType == Game::Sparrow) 
        {
            // Pirate: Cutscene pillarboxing
            std::uint8_t* PillarboxingScanResult = Memory::PatternScan(exeModule, "75 ?? BA ?? ?? ?? ?? 48 8B ?? E8 ?? ?? ?? ?? 84 C0 75 ?? BA ?? ?? ?? ?? 48 8B ?? E8 ?? ?? ?? ?? 84 ?? 74 ?? 81 ?? ?? ?? ?? ?? 77 ??");
            if (PillarboxingScanResult)
            {
                spdlog::info("Disable Pillarboxing: Pillarboxing: Address: {:s}+0x{:x}", sExeName, PillarboxingScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(PillarboxingScanResult, "\x90\x90", 2); // Cutscenes
                Memory::PatchBytes(PillarboxingScanResult + 0x11, "\x90\x90", 2); // Talk
            }
            else
            {
                spdlog::error("Disable Pillarboxing: Pillarboxing: Pattern scan(s) failed.");
            }

            // Pirate: Title card pillarboxing
            std::uint8_t* TitleCardsScanResult = Memory::PatternScan(exeModule, "C5 F8 ?? ?? 72 ?? 48 39 ?? ?? ?? ?? ?? 75 ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ??");
            if (TitleCardsScanResult)
            {
                spdlog::info("Disable Pillarboxing: Title Cards: Address: {:s}+0x{:x}", sExeName, TitleCardsScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid TitleCardsMidHook{};
                TitleCardsMidHook = safetyhook::create_mid(TitleCardsScanResult + 0x24,
                    [](SafetyHookContext &ctx)
                    {
                        if (ctx.rbx)
                        {
                            // Set ZF to jump over pillarboxing so that title cards are not pillarboxed
                            if (*reinterpret_cast<int*>(ctx.rbx + 0x8) == 2) 
                                ctx.rflags |= (1ULL << 6);
                        }
                    });
            }
            else
            {
                spdlog::error("Disable Pillarboxing: Title Cards: Pattern scan(s) failed.");
            }
        }
        else if (eGameType == Game::Elvis) 
        {
            // LAD8: Cutscene pillarboxing
            std::uint8_t* CutscenePillarboxingScanResult = Memory::PatternScan(exeModule, "74 ?? 32 ?? EB ?? 05 ?? ?? ?? ?? 3D ?? ?? ?? ?? 77 ?? 48 8D ?? ?? ?? ?? ??");
            std::uint8_t* TalkPillarboxingScanResult = Memory::PatternScan(exeModule, "0F 85 ?? ?? ?? ?? 8B ?? ?? ?? 45 ?? ?? 75 ?? 45 ?? ?? 75 ?? 45 ?? ?? 75 ??");
            if (CutscenePillarboxingScanResult && TalkPillarboxingScanResult)
            {
                spdlog::info("Disable Pillarboxing: Cutscene Pillarboxing: Address: {:s}+0x{:x}", sExeName, CutscenePillarboxingScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(CutscenePillarboxingScanResult, "\x90\x90", 2); // Cutscenes
                spdlog::info("Disable Pillarboxing: Talk Pillarboxing: Address: {:s}+0x{:x}", sExeName, TalkPillarboxingScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(TalkPillarboxingScanResult, "\x90\x90\x90\x90\x90\x90", 6); // Talk
            }
            else
            {
                spdlog::error("Disable Pillarboxing: Pillarboxing: Pattern scan(s) failed.");
            }
        }
        else if (eGameType == Game::Aston || eGameType == Game::Coyote) 
        {
            // Gaiden/LJ: Cutscene pillarboxing
            std::uint8_t* CutsceneBarsScanResult = Memory::PatternScan(exeModule, "84 C0 0F 85 ?? ?? ?? ?? B0 01 48 8B ?? ?? ?? 48 83 ?? ?? 41 ??");
            if (CutsceneBarsScanResult) 
            {
                spdlog::info("Disable Pillarboxing/Letterboxing: Cutscene: Address: {:s}+0x{:x}", sExeName, CutsceneBarsScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(CutsceneBarsScanResult + 0x3, "\x84", 1);
            }
            else 
            {
                spdlog::error("Disable Pillarboxing/Letterboxing: Pattern scan(s) failed.");
            }
        }
        else if (eGameType == Game::Yazawa)
        {
            // LAD7: Cutscene pillarboxing
            std::uint8_t* CutsceneBarsScanResult = Memory::PatternScan(exeModule, "0F 85 ?? ?? ?? ?? 44 38 ?? ?? 75 ?? 44 38 ?? ?? 75 ?? 44 38 ?? ?? 75 ??");
            if (CutsceneBarsScanResult) 
            {
                spdlog::info("Disable Pillarboxing/Letterboxing: Cutscene: Address: {:s}+0x{:x}", sExeName, CutsceneBarsScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(CutsceneBarsScanResult, "\x90\x90\x90\x90\x90\x90", 6);
            }
            else 
            {
                spdlog::error("Disable Pillarboxing/Letterboxing: Pattern scan(s) failed.");
            }
        }
        else if (eGameType == Game::Judge) 
        {
            // Judgment: Cutscene pillarboxing
            std::uint8_t* CutsceneBarsScanResult = Memory::PatternScan(exeModule, "40 ?? ?? 74 ?? B0 01 EB ?? 32 C0 48 8B ?? ?? ?? 48 8B ?? ?? ?? 48 8B ?? ?? ??");
            if (CutsceneBarsScanResult) 
            {
                spdlog::info("Disable Pillarboxing/Letterboxing: Cutscene: Address: {:s}+0x{:x}", sExeName, CutsceneBarsScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(CutsceneBarsScanResult + 0x6, "\x00", 1);
            }
            else 
            {
                spdlog::error("Disable Pillarboxing/Letterboxing: Pattern scan(s) failed.");
            }
        }
        else if (eGameType == Game::Lexus2) 
        {
            // Kiwami 2: Cutscene pillarboxing
            std::uint8_t* CutsceneBarsScanResult = Memory::PatternScan(exeModule, "84 C0 74 ?? B0 01 48 8B ?? ?? ?? 48 83 ?? ?? ?? C3 E8 ?? ?? ?? ??");
            if (CutsceneBarsScanResult) 
            {
                spdlog::info("Disable Pillarboxing/Letterboxing: Cutscene: Address: {:s}+0x{:x}", sExeName, CutsceneBarsScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(CutsceneBarsScanResult + 0x5, "\x00", 1);
            }
            else 
            {
                spdlog::error("Disable Pillarboxing/Letterboxing: Pattern scan(s) failed.");
            }
        }
        else if (eGameType == Game::OgreF) 
        {
            // Yakuza 6: Cutscene pillarboxing
            std::uint8_t* CutsceneBarsScanResult = Memory::PatternScan(exeModule, "49 ?? ?? E8 ?? ?? ?? ?? C5 ?? ?? ?? E9 ?? ?? ?? ?? 0F ?? ?? ?? 0F 83 ?? ?? ?? ?? 41 ?? 03 00 00 00");
            if (CutsceneBarsScanResult) 
            {
                spdlog::info("Disable Pillarboxing/Letterboxing: Cutscene: Address: {:s}+0x{:x}", sExeName, CutsceneBarsScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid CutsceneBarsMidHook{};
                CutsceneBarsMidHook = safetyhook::create_mid(CutsceneBarsScanResult,
                    [](SafetyHookContext &ctx)
                    {
                        ctx.xmm2.f32[0] = 1000.00f;
                    });
            }
            else 
            {
                spdlog::error("Disable Pillarboxing/Letterboxing: Pattern scan(s) failed.");
            }
        } 
    }

    // Yakuza 6 is a special case in that it forces letterboxing when played at <16:9.
    if ((bDisableBarsCutscene || bDisableBarsGlobal)  && eGameType == Game::OgreF) 
    {
        // Yakuza 6: Disable letterboxing
        std::uint8_t* LetterboxingScanResult = Memory::PatternScan(exeModule, "76 ?? C5 ?? ?? ?? C5 ?? ?? ?? C5 ?? ?? ?? 44 89 ?? C5 ?? ?? ?? ?? 4C 89 ?? ??");
        std::uint8_t* ForcedAspectRatioScanResult = Memory::PatternScan(exeModule, "7E ?? C5 ?? ?? ?? ?? ?? ?? ?? EB ?? C5 ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? 4C 8B ?? ?? ?? ?? ??");
        if (LetterboxingScanResult && ForcedAspectRatioScanResult) 
        {
            spdlog::info("Disable Pillarboxing/Letterboxing: Letterboxing: Address: {:s}+0x{:x}", sExeName, LetterboxingScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(LetterboxingScanResult, "\xEB", 1);
            spdlog::info("Disable Pillarboxing/Letterboxing: Forced Aspect Ratio: Address: {:s}+0x{:x}", sExeName, ForcedAspectRatioScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(ForcedAspectRatioScanResult, "\xEB", 1);
        }
        else 
        {
            spdlog::error("Disable Pillarboxing/Letterboxing: Pattern scan(s) failed.");
        }
    }
}

void Graphics()
{
    if (iShadowResolution != 2048) 
    {
        if (eGameType == Game::OgreF) 
        {
            // Yakuza 6: Shadow resolution
            std::uint8_t* ShadowResolutionScanResult = Memory::PatternScan(exeModule, "C7 ?? ?? ?? ?? ?? 00 08 00 00 C7 ?? ?? ?? ?? ?? 00 08 00 00 C6 ?? ?? ?? ?? ?? 00");
            if (ShadowResolutionScanResult)
            {
                spdlog::info("Shadow Resolution: Address: {:s}+0x{:x}", sExeName, ShadowResolutionScanResult - (std::uint8_t*)exeModule);
                Memory::Write(ShadowResolutionScanResult + 0x6, iShadowResolution);
                Memory::Write(ShadowResolutionScanResult + 0x10, iShadowResolution);
            }
            else
            {
                spdlog::error("Shadow Resolution: Pattern scan(s) failed.");
            }
        }
        else if (eGameType == Game::Lexus2) 
        {
            // Kiwami 2: Shadow resolution
            std::uint8_t* ShadowResolutionScanResult = Memory::PatternScan(exeModule, "E8 ?? ?? ?? ?? BA 00 08 00 00 41 ?? 00 04 00 00");
            if (ShadowResolutionScanResult)
            {
                spdlog::info("Shadow Resolution: Address: {:s}+0x{:x}", sExeName, ShadowResolutionScanResult - (std::uint8_t*)exeModule);
                Memory::Write(ShadowResolutionScanResult + 0x6, iShadowResolution);
                Memory::Write(ShadowResolutionScanResult + 0xC, iShadowResolution / 2);
            }
            else
            {
                spdlog::error("Shadow Resolution: Pattern scan(s) failed.");
            }
        }
        else 
        {
            // Newer: Shadow resolution
            std::uint8_t* ShadowResolutionScanResult = Memory::PatternScan(exeModule, "39 0D ?? ?? ?? ?? 75 ?? 39 15 ?? ?? ?? ?? ?? ??");
            if (ShadowResolutionScanResult)
            {
                spdlog::info("Shadow Resolution: Address: {:s}+0x{:x}", sExeName, ShadowResolutionScanResult - (std::uint8_t*)exeModule);
                static SafetyHookMid ShadowResolutionMidHook{};
                ShadowResolutionMidHook = safetyhook::create_mid(ShadowResolutionScanResult,
                    [](SafetyHookContext &ctx)
                    {
                        // Check if shadowmap resolution is 2048x2048
                        if (ctx.rcx == 0x800)
                            ctx.rcx = ctx.rdx = static_cast<uintptr_t>(iShadowResolution);
                    });
            }
            else
            {
                spdlog::error("Shadow Resolution: Pattern scan(s) failed.");
            }
        }
    }

    if (bShadowDrawDistance) 
    {
        std::uint8_t* ShadowDrawDistanceScanResult = nullptr;

        if (eGameType == Game::Sparrow)
        {
            // Pirate: Shadow draw distance
            ShadowDrawDistanceScanResult = Memory::PatternScan(exeModule, "75 ?? C5 ?? 10 ?? ?? ?? ?? ?? C5 ?? ?? ?? 48 8D ?? ?? ?? 49 ?? ?? C5 ?? 11 ?? ?? ??");
        }
        else if (eGameType == Game::Elvis || eGameType == Game::Aston || eGameType == Game::Coyote)
        {
            // IW/Gaiden/LJ: Shadow draw distance
            ShadowDrawDistanceScanResult = Memory::PatternScan(exeModule, "75 ?? C5 ?? 57 ?? C4 ?? ?? ?? ?? C5 ?? ?? ?? C5 ?? 57 ?? C5 ?? 10 ??");
        }
        else if (eGameType == Game::Yazawa || eGameType == Game::Judge) 
        {
            // LAD7/Judgment: Shadow draw distance
            ShadowDrawDistanceScanResult = Memory::PatternScan(exeModule, "75 ?? C5 ?? ?? ?? C5 ?? 57 ?? C5 ?? 10 ?? C5 ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? 10 ?? ?? ??");
        }
        else if (eGameType == Game::Lexus2 || eGameType == Game::OgreF)
        {
            spdlog::info("Shadow Draw Distance: Unsupported game for this feature.");
        }

        if (ShadowDrawDistanceScanResult)
        {
            spdlog::info("Shadow Draw Distance: Address: {:s}+0x{:x}", sExeName, ShadowDrawDistanceScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(ShadowDrawDistanceScanResult, "\xEB", 1);
        }
        else
        {
            spdlog::error("Shadow Draw Distance: Pattern scan(s) failed.");
        }
    }
    
    if (bAdjustLOD) 
    {
        if (eGameType == Game::Sparrow || eGameType == Game::Elvis) 
        {
            // Pirate/IW: LOD
            std::uint8_t* ObjectLODSwitchScanResult = Memory::PatternScan(exeModule, "0F 85 ?? ?? ?? ?? 0F B6 ?? ?? ?? 0F 84 ?? ?? ?? ?? 83 ?? 01 0F 84 ?? ?? ?? ??");
            std::uint8_t* FoliageLODSwitchScanResult = Memory::PatternScan(exeModule, "C5 F8 ?? ?? 72 ?? ?? ?? EB ?? C4 C1 ?? ?? ?? ?? C5 F8 ?? ??");
            if (ObjectLODSwitchScanResult && FoliageLODSwitchScanResult)
            {
                spdlog::info("LOD: Object: Address: {:s}+0x{:x}", sExeName, ObjectLODSwitchScanResult - (std::uint8_t*)exeModule);
                if (eGameType == Game::Sparrow)
                    Memory::PatchBytes(ObjectLODSwitchScanResult + 0x6, "\x31\xC9\x90", 3); // xor ecx,ecx
                else if (eGameType == Game::Elvis)
                    Memory::PatchBytes(ObjectLODSwitchScanResult + 0x6, "\x31\xC1\x90", 3); // xor eax,eax

                spdlog::info("LOD: Foliage: Address: {:s}+0x{:x}", sExeName, FoliageLODSwitchScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(FoliageLODSwitchScanResult + 0x4, "\x90\x90", 2);
            }
            else
            {
                spdlog::error("LOD: Pattern scan(s) failed.");
            }
        }
        else if (eGameType == Game::Aston || eGameType == Game::Coyote)
        {
            // Gaiden/LJ: LOD
            std::uint8_t* ObjectLODSwitchScanResult = Memory::PatternScan(exeModule, "C5 F8 ?? ?? 72 ?? ?? ?? ?? EB ?? C4 C1 ?? ?? ?? ?? C5 F8 ?? ?? 72 ??");
            std::uint8_t* FoliageLODSwitchScanResult = Memory::PatternScan(exeModule, "76 ?? 41 ?? ?? EB ?? C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? 76 ?? B9 01 00 00 00");
            if (ObjectLODSwitchScanResult && FoliageLODSwitchScanResult)
            {
                spdlog::info("LOD: Object: Address: {:s}+0x{:x}", sExeName, ObjectLODSwitchScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(ObjectLODSwitchScanResult + 0x4, "\x90\x90", 2);

                spdlog::info("LOD: Foliage: Address: {:s}+0x{:x}", sExeName, FoliageLODSwitchScanResult - (std::uint8_t*)exeModule);
                Memory::PatchBytes(FoliageLODSwitchScanResult, "\x90\x90", 2);
            }
            else
            {
                spdlog::error("LOD: Pattern scan(s) failed.");
            }
        }
    }
}

std::mutex mainThreadFinishedMutex;
std::condition_variable mainThreadFinishedVar;
bool mainThreadFinished = false;

DWORD __stdcall Main(void*)
{
    Logging();
    Configuration();
    if (DetectGame())
    {
        IntroSkip();
        DisablePillarboxing();
        Graphics();
    }

    {
        std::lock_guard lock(mainThreadFinishedMutex);
        mainThreadFinished = true;
        mainThreadFinishedVar.notify_all();
    }

    return true;
}

std::mutex getCommandLineMutex;
bool getCommandLineHookCalled = false;
LPSTR(WINAPI* GetCommandLineA_Fn)();
LPSTR WINAPI GetCommandLineA_Hook()
{
    std::lock_guard lock(getCommandLineMutex);
    if (!getCommandLineHookCalled)
    {
        getCommandLineHookCalled = true;
        Memory::HookIAT(exeModule, "kernel32.dll", GetCommandLineA_Hook, GetCommandLineA_Fn);
        if (!mainThreadFinished)
        {
            std::unique_lock finishedLock(mainThreadFinishedMutex);
            mainThreadFinishedVar.wait(finishedLock, [] { return mainThreadFinished; });
        }
    }
    return GetCommandLineA_Fn();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Detach from "startup.exe"
        char exeName[MAX_PATH];
        GetModuleFileNameA(NULL, exeName, MAX_PATH);
        std::string exeStr(exeName);
        if (exeStr.find("startup.exe") != std::string::npos)
            return FALSE;

        thisModule = hModule;
        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
        if (kernel32)
        {
            GetCommandLineA_Fn = reinterpret_cast<decltype(GetCommandLineA_Fn)>(GetProcAddress(kernel32, "GetCommandLineA"));
            if (GetCommandLineA_Fn)
            {
                Memory::HookIAT(exeModule, "kernel32.dll", GetCommandLineA_Fn, GetCommandLineA_Hook);
            }
        }

        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST);
            CloseHandle(mainHandle);
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}