#pragma once
// =============================================================================
//  ProfileManager.h - Save/load click profiles to an INI file in %APPDATA%.
// =============================================================================

#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <sstream>
#include "ClickSettings.h"

class ProfileManager
{
public:
    ProfileManager()
    {
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path)))
        {
            iniPath = std::wstring(path) + L"\\AutoClickerProfiles.ini";
        }
    }

    const std::wstring& getIniPath() const { return iniPath; }

    // Save all profiles to the INI file.
    void saveAll(const std::vector<SettingsProfile>& profiles)
    {
        // Build the pipe-delimited name list.
        std::wstring allNames;
        for (const auto& p : profiles)
            allNames += p.name + L"|";
        WritePrivateProfileStringW(L"__PROFILES__", L"Names", allNames.c_str(), iniPath.c_str());

        for (const auto& profile : profiles)
        {
            const wchar_t* section = profile.name.c_str();
            const auto& s = profile.settings;

            WritePrivateProfileStringW(section, L"UseRandom",      s.useRandom ? L"1" : L"0",       iniPath.c_str());
            WritePrivateProfileStringW(section, L"IntervalFixed",  std::to_wstring(s.intervalFixed).c_str(),  iniPath.c_str());
            WritePrivateProfileStringW(section, L"IntervalOffset", std::to_wstring(s.intervalOffset).c_str(), iniPath.c_str());
            WritePrivateProfileStringW(section, L"ClickMode",      std::to_wstring(s.clickMode).c_str(),      iniPath.c_str());
            WritePrivateProfileStringW(section, L"ClickPosFixed",  s.clickPosFixed ? L"1" : L"0",   iniPath.c_str());
            WritePrivateProfileStringW(section, L"UseAreaRandom",  s.useAreaRandom ? L"1" : L"0",   iniPath.c_str());
            WritePrivateProfileStringW(section, L"AreaRadius",     std::to_wstring(s.areaRadius).c_str(),     iniPath.c_str());
            WritePrivateProfileStringW(section, L"UseRandomPos",   s.useRandomPos ? L"1" : L"0",    iniPath.c_str());
            WritePrivateProfileStringW(section, L"LimitMode",      std::to_wstring(static_cast<DWORD>(s.limitMode)).c_str(), iniPath.c_str());
            WritePrivateProfileStringW(section, L"CountLimit",     std::to_wstring(s.clickCountLimit).c_str(),  iniPath.c_str());
            WritePrivateProfileStringW(section, L"DurationSec",    std::to_wstring(s.clickDurationSec).c_str(), iniPath.c_str());
            WritePrivateProfileStringW(section, L"EnableRest",     s.enableRest ? L"1" : L"0",       iniPath.c_str());
            WritePrivateProfileStringW(section, L"RestTime",       std::to_wstring(s.restTime).c_str(),        iniPath.c_str());

            // Serialize click points as x,y,interval|x,y,interval|...
            std::wstringstream ss;
            for (const auto& pt : s.clickPoints)
                ss << pt.pos.x << L"," << pt.pos.y << L"," << pt.interval << L"|";
            WritePrivateProfileStringW(section, L"MultiClickPoints", ss.str().c_str(), iniPath.c_str());
        }
    }

    // Load all profiles from the INI file.
    std::vector<SettingsProfile> loadAll()
    {
        std::vector<SettingsProfile> result;

        wchar_t namesBuffer[2048] = {};
        GetPrivateProfileStringW(L"__PROFILES__", L"Names", L"", namesBuffer, 2048, iniPath.c_str());

        std::wstringstream namesStream(namesBuffer);
        std::wstring profileName;
        while (std::getline(namesStream, profileName, L'|'))
        {
            if (profileName.empty()) continue;

            SettingsProfile profile;
            profile.name = profileName;
            auto& s = profile.settings;
            const wchar_t* sec = profileName.c_str();

            s.useRandom       = GetPrivateProfileIntW(sec, L"UseRandom",      1,    iniPath.c_str()) != 0;
            s.intervalFixed   = GetPrivateProfileIntW(sec, L"IntervalFixed",  1500, iniPath.c_str());
            s.intervalOffset  = GetPrivateProfileIntW(sec, L"IntervalOffset", 500,  iniPath.c_str());
            s.clickMode       = GetPrivateProfileIntW(sec, L"ClickMode",      0,    iniPath.c_str());
            s.clickPosFixed   = GetPrivateProfileIntW(sec, L"ClickPosFixed",  1,    iniPath.c_str()) != 0;
            s.useAreaRandom   = GetPrivateProfileIntW(sec, L"UseAreaRandom",  0,    iniPath.c_str()) != 0;
            s.areaRadius      = GetPrivateProfileIntW(sec, L"AreaRadius",     20,   iniPath.c_str());
            s.useRandomPos    = GetPrivateProfileIntW(sec, L"UseRandomPos",   0,    iniPath.c_str()) != 0;
            s.limitMode       = static_cast<LimitMode>(GetPrivateProfileIntW(sec, L"LimitMode", 2, iniPath.c_str()));
            s.clickCountLimit = GetPrivateProfileIntW(sec, L"CountLimit",     1000, iniPath.c_str());
            s.clickDurationSec= GetPrivateProfileIntW(sec, L"DurationSec",    3600, iniPath.c_str());
            s.enableRest      = GetPrivateProfileIntW(sec, L"EnableRest",     0,    iniPath.c_str()) != 0;
            s.restTime        = GetPrivateProfileIntW(sec, L"RestTime",       600,  iniPath.c_str());

            // Deserialize click points.
            wchar_t ptsBuf[4096] = {};
            GetPrivateProfileStringW(sec, L"MultiClickPoints", L"", ptsBuf, 4096, iniPath.c_str());
            std::wstringstream ptsStream(ptsBuf);
            std::wstring seg;
            while (std::getline(ptsStream, seg, L'|'))
            {
                if (seg.empty()) continue;
                std::wistringstream ss(seg);
                std::wstring val;
                ClickPoint pt;
                if (std::getline(ss, val, L',')) pt.pos.x = _wtol(val.c_str());
                if (std::getline(ss, val, L',')) pt.pos.y = _wtol(val.c_str());
                if (std::getline(ss, val, L',')) pt.interval = _wtoi(val.c_str());
                s.clickPoints.push_back(pt);
            }

            result.push_back(profile);
        }

        return result;
    }

    // Delete a profile section from the INI file.
    void deleteProfile(const std::wstring& name)
    {
        WritePrivateProfileStringW(name.c_str(), nullptr, nullptr, iniPath.c_str());
    }

private:
    std::wstring iniPath;
};
