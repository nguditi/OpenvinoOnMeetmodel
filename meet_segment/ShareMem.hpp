#ifndef SHAREDMEMORY_HPP_INCLUDED
#define SHAREDMEMORY_HPP_INCLUDED

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <tchar.h>
#include <iostream>
#include <map>

#define VIRTUAL_BACKGROUND_MEMSIZE 1280 * 720 * 4 + 1024 // 4 chanel frame + custom header

class SharedMemory
{
private:
	void* FromFile;
	void* hFileMap;
	void* pData;
	std::string MapName;
	uint32_t Size;
	bool Debug;
	std::map<std::string, void*> Events;

public:
	SharedMemory(std::string MapName);
	SharedMemory(std::string MapName, uint32_t size);
	~SharedMemory();

	SharedMemory(const SharedMemory& Shm) = delete;
	SharedMemory(SharedMemory && Shm) = delete;
	SharedMemory& operator = (const SharedMemory& Shm) = delete;
	SharedMemory& operator = (SharedMemory && Shm) = delete;

	void* GetDataPointer();

	bool OpenMemoryMap(uint32_t Size);

	bool MapMemory(uint32_t Size);

	bool ReleaseMemory();

	bool CreateNewEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, bool bManualReset, bool bInitialState, std::string EventName);

	std::uint32_t OpenSingleEvent(std::string EventName, bool InheritHandle, bool SaveHandle = false, std::uint32_t dwDesiredAccess = EVENT_ALL_ACCESS, std::uint32_t dwMilliseconds = INFINITE);

	bool SetEventSignal(std::string EventName, bool Signaled);

	bool DeleteSingleEvent(std::string EventName);

	bool DeleteAllEvents();

	void SetDebug(bool On);
};

#endif // SHAREDMEMORY_HPP_INCLUDED