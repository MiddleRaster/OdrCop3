#pragma once

#include <windows.h>
#include <io.h>
#include <fcntl.h>

class StderrSuppressor
{
    int saved, devNull;
public:
    StderrSuppressor()
    {
        saved = _dup(2);
        devNull = -1;
        _sopen_s(&devNull, "nul", _O_WRONLY, _SH_DENYNO, _S_IWRITE);
        if (saved != -1 && devNull != -1)
            _dup2(devNull, 2);
    }
    ~StderrSuppressor()
    {
        if (saved != -1)
        {
            _dup2(saved, 2);
            _close(saved);
        }
        if (devNull != -1)
            _close(devNull);
    }
};
