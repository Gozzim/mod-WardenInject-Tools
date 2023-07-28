#ifndef _WARDENINJECTDATA_H_
#define _WARDENINJECTDATA_H_

#include "WardenInjectMgr.h"

class WardenInjectData
{
public:
    WardenInjectData();
    ~WardenInjectData();

    float GetVersion();
    void SetVersion(float _version);

    bool IsCompressed();
    void SetCompressed(bool _compressed);

    bool IsCached();
    void SetCached(bool _cached);

    bool IsOnLogin();
    void SetOnLogin(bool _onLogin);

    uint16 GetOrderNum();
    void SetOrderNum(uint16 _orderNum);

    std::string GetPayload();
    void SetPayload(std::string _payload);

    std::vector<std::string> GetSavedVars();
    void SetSavedVars(std::vector<std::string> _savedVars);
    void AddSavedVar(std::string _savedVar);
    void RemoveSavedVar(std::string _savedVar);
    void ClearSavedVars();

private:
    float version;
    bool compressed;
    bool cached;
    bool onLogin;
    uint16 orderNum;
    std::string payload;
    std::vector<std::string> savedVars;
};

#endif
