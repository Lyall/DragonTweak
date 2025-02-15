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
std::string sFixVersion = "0.0.1";
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
int iShadowResolution;
bool bDisablePillarboxing;

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
    Spr         // Like a Dragon: Pirate Yakuza in Hawaii
};

const std::map<Game, GameInfo> kGames = {
    {Game::OgreF,  {"Yakuza 6: The Song of Life", "Yakuza6.exe"}},
    {Game::Lexus2, {"Yakuza Kiwami 2", "YakuzaKiwami2.exe"}},
    {Game::Judge,  {"Judgment", "Judgment.exe"}},
    {Game::Yazawa, {"Yakuza: Like a Dragon", "YakuzaLikeADragon.exe"}},
    {Game::Coyote, {"Lost Judgment", "LostJudgment.exe"}},
    {Game::Aston,  {"Like a Dragon Gaiden: The Man Who Erased His Name", "LikeADragonGaiden.exe"}},
    {Game::Elvis,  {"Like a Dragon: Infinite Wealth", "LikeADragon8.exe"}},
    {Game::Spr,    {"Like a Dragon: Pirate Yakuza in Hawaii", "LikeADragonPirates.exe"}},
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
    inipp::get_value(ini.sections["Shadow Quality"], "Resolution", iShadowResolution);
    inipp::get_value(ini.sections["Disable Pillarboxing"], "Enabled", bDisablePillarboxing);

    // Clamp settings
    iShadowResolution = std::clamp(iShadowResolution, 64, 8192);

    // Log ini parse
    spdlog_confparse(iShadowResolution);
    spdlog_confparse(bDisablePillarboxing);

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

void DisablePillarboxing()
{
    std::vector<const char*> DrawBarsPatterns = {
        "40 ?? ?? 41 ?? 48 8D ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 33 ?? 48 89 ?? ?? B9 ?? ?? ?? ??",                                              // Pirate/LAD8/Gaiden
        "40 ?? 48 8D ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 33 ?? 48 89 ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0",                                  // LAD7/Judgment
        "48 89 ?? ?? ?? 55 56 57 48 8D ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 48 8B ?? E8 ?? ?? ?? ?? 3B ?? ?? ?? ?? ?? 0F 83 ?? ?? ?? ??",                                  // Kiwami2
        "48 8B ?? 48 89 ?? ?? 55 56 57 41 ?? 41 ?? 48 83 ?? ?? 48 C7 ?? ?? ?? ?? ?? ?? 48 89 ?? ?? C5 ?? ?? ?? ?? C5 ?? ?? ?? ??",                                  // Yakuza 6
        "40 ?? 57 41 ?? 41 ?? 41 ?? 48 8D ?? ?? ?? ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 33 ?? 48 89 ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 45 33 ??"  // Lost Judgment
    };

    // All: Disable pillarboxing/letterboxing
    std::vector<std::uint8_t*> DrawBarsScanResults = Memory::MultiPatternScanAll(exeModule, DrawBarsPatterns);
    if (!DrawBarsScanResults.empty())
    {
        for (auto& DrawBarsScanResult : DrawBarsScanResults) 
        {
            spdlog::info("Disable Pillarboxing: Address: {:s}+0x{:x}", sExeName, DrawBarsScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(DrawBarsScanResult, "\xC3\x90", 2);
            spdlog::info("Disable Pillarboxing: Patched instruction.");
        }
    }
    else
    {
        spdlog::error("Disable Pillarboxing: Pattern scan(s) failed.");
    }
}

void ShadowResolution()
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
            spdlog::info("Shadow Resolution: Patched instruction.");
        }
        else
        {
            spdlog::error("Shadow Resolution: Pattern scan(s) failed.");
        }
    }

    if (eGameType == Game::Lexus2) 
    {
        // Kiwami 2: Shadow resolution
        std::uint8_t* ShadowResolutionScanResult = Memory::PatternScan(exeModule, "E8 ?? ?? ?? ?? BA 00 08 00 00 41 ?? 00 04 00 00");
        if (ShadowResolutionScanResult)
        {
            spdlog::info("Shadow Resolution: Address: {:s}+0x{:x}", sExeName, ShadowResolutionScanResult - (std::uint8_t*)exeModule);
            Memory::Write(ShadowResolutionScanResult + 0x6, iShadowResolution);
            Memory::Write(ShadowResolutionScanResult + 0xC, iShadowResolution / 2);
            spdlog::info("Shadow Resolution: Patched instruction.");
        }
        else
        {
            spdlog::error("Shadow Resolution: Pattern scan(s) failed.");
        }
    }

    if (eGameType != Game::OgreF || eGameType != Game::Lexus2)
    {
          // Newer: Shadow resolution
          std::uint8_t* ShadowResolutionScanResult = Memory::PatternScan(exeModule, "39 ?? ?? ?? ?? ?? 75 ?? 39 ?? ?? ?? ?? ?? 75 ?? ?? ?? C3");
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

DWORD __stdcall Main(void*)
{
    Logging();
    Configuration();

    if (DetectGame())
    {
        DisablePillarboxing();
        ShadowResolution();
    }

    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        thisModule = hModule;

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