#include "WardenInjectData.h"

WardenInjectData::WardenInjectData()
{
    version = 1;
    compressed = false;
    cached = true;
    onLogin = false;
    payload = "";
    savedVars = {};
}

WardenInjectData::~WardenInjectData()
{
}

float WardenInjectData::GetVersion()
{
    return version;
}

void WardenInjectData::SetVersion(float _version)
{
    version = _version;
}

bool WardenInjectData::IsCompressed()
{
    return compressed;
}

void WardenInjectData::SetCompressed(bool _compressed)
{
    compressed = _compressed;
}

bool WardenInjectData::IsCached()
{
    return cached;
}

void WardenInjectData::SetCached(bool _cached)
{
    cached = _cached;
}

bool WardenInjectData::IsOnLogin()
{
    return onLogin;
}

void WardenInjectData::SetOnLogin(bool _onLogin)
{
    onLogin = _onLogin;
}

std::string WardenInjectData::GetPayload()
{
    return payload;
}

void WardenInjectData::SetPayload(std::string _payload)
{
    payload = _payload;
}

std::vector<std::string> WardenInjectData::GetSavedVars()
{
    return savedVars;
}

void WardenInjectData::SetSavedVars(std::vector<std::string> _savedVars)
{
    savedVars = _savedVars;
}

void WardenInjectData::AddSavedVar(std::string _savedVar)
{
    savedVars.push_back(_savedVar);
}

void WardenInjectData::RemoveSavedVar(std::string _savedVar)
{
    savedVars.erase(std::remove(savedVars.begin(), savedVars.end(), _savedVar), savedVars.end());
}

void WardenInjectData::ClearSavedVars()
{
    savedVars.clear();
}
