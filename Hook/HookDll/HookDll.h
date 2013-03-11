#ifndef HOOKAPI
#define HOOKAPI  __declspec(dllimport)
#endif

HOOKAPI BOOL WINAPI setHook();
HOOKAPI void unsetHook();