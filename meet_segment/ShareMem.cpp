#include "ShareMem.hpp"

SharedMemory::SharedMemory(std::string MapName) : hFileMap(nullptr), pData(nullptr), MapName(MapName), Size(0), Debug(false), Events() {}
SharedMemory::SharedMemory(std::string MapName, uint32_t Size) : hFileMap(nullptr), pData(nullptr), MapName(MapName), Size(Size), Debug(false), Events() {}
SharedMemory::~SharedMemory()
{
	ReleaseMemory();
	DeleteAllEvents();
}

void* SharedMemory::GetDataPointer()
{
	void* Ptr = pData;
	return Ptr;
}

bool SharedMemory::OpenMemoryMap(uint32_t Size)
{
	this->Size = Size;

#if defined _WIN32 || defined _WIN64
	if ((hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, MapName.c_str())) == nullptr)
	{
		if (Debug) std::cout << _T("\nCould Not Open Shared Memory Map.\n");
		return false;
	}

	if ((pData = MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, Size)) == nullptr)
	{
		if (Debug) std::cout << _T("\nCould Not Map View Of File.\n");
		CloseHandle(hFileMap);
		return false;
	}

#else

	if ((hFileMap = open(MapName.c_str(), O_RDWR | O_CREAT, 438)) == -1)
	{
		if (Debug) std::cout << _T("\nCould Not Open Shared Memory Map.\n");
		return false;
	}

	if ((pData = mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, hFileMap, 0)) == MAP_FAILED)
	{
		if (Debug) std::cout << _T("\nCould Not Map View Of File.\n");
		close(hFileMap);
		return false;
	}
#endif

	if (Debug) std::cout << _T("\nInter-Process Communication Successful.\n");
	return true;
}

bool SharedMemory::MapMemory(uint32_t Size)
{
	this->Size = Size;

#if defined _WIN32 || defined _WIN64
	if ((hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, Size, MapName.c_str())) == nullptr)
	{
		if (Debug) std::cout << _T("\nCould Not Create Shared Memory Map.\n");
		return false;
	}

	if ((pData = MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, Size)) == nullptr)
	{
		if (Debug) std::cout << _T("\nCould Not Map View Of File.\n");
		CloseHandle(hFileMap);
		return false;
	}

#else

	if ((hFileMap = open(MapName.c_str(), O_RDWR | O_CREAT, 438)) == -1)
	{
		if (Debug) std::cout << _T("\nCould Not Create Shared Memory Map.\n");
		return false;
	}

	if ((pData = mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, hFileMap, 0)) == MAP_FAILED)
	{
		if (Debug) std::cout << _T("\nCould Not Map View Of File.\n");
		close(hFileMap);
		return false;
	}
#endif

	if (Debug) std::cout << _T("\nMapped Shared Memory Successfully.\n");
	return true;
}

bool SharedMemory::ReleaseMemory()
{
	bool Result = false;
#if defined _WIN32 || defined _WIN64
	if (pData)
	{
		Result = UnmapViewOfFile(pData);
		pData = nullptr;
		if (Result && Debug)
		{
			std::cout << _T("\nMemory Un-Mapped Successfully.\n");
		}
	}

	if (hFileMap)
	{
		if (CloseHandle(hFileMap))
		{
			hFileMap = nullptr;
			Result = Result && true;
			if (Debug) std::cout << _T("\nMemory Map Closed Successfully.\n");
		}
	}

#else

	if (pData)
	{
		Result = munmap(pData, Size);
		if (!Result && Debug)
		{
			std::cout << _T("\nMemory Un-Mapped Successfully.\n");
		}
		pData = nullptr;
		return true;
	}

	if (hFileMap)
	{
		if (!close(hFileMap))
		{
			hFileMap = nullptr;
			if (Debug) std::cout << _T("\nMemory Map Closed Successfully.\n");
		}
	}
#endif
	return Result;
}

bool SharedMemory::CreateNewEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, bool bManualReset, bool bInitialState, std::string EventName)
{
	std::map<std::string, void*>::iterator it = Events.find(EventName);
	if (it != Events.end())
	{
		if (Debug)
		{
			std::cout << _T("\nCreateNewEvent Error: An Event With That Key Already Exists!\n");
		}
		return false;
	}
#if defined _WIN32 || defined _WIN64
	Events.insert(std::pair<std::string, void*>(EventName, CreateEvent(lpEventAttributes, bManualReset, bInitialState, EventName.c_str())));
#else
	Events.insert(std::pair<std::string, void*>(EventName, sem_open(EventName.c_str(), O_CREAT | O_EXCL, 0666, 0);)
#endif
	it = Events.end();
	return ((--it)->second != nullptr);
}

std::uint32_t SharedMemory::OpenSingleEvent(std::string EventName, bool InheritHandle, bool SaveHandle, std::uint32_t dwDesiredAccess, std::uint32_t dwMilliseconds)
{
#if defined _WIN32 || defined _WIN64
	void* hEvent = OpenEvent(dwDesiredAccess, InheritHandle, EventName.c_str());
	if (hEvent)
	{
#else
	void* hEvent = sem_open(EventName.c_str(), 0);
	if (hEvent != SEM_FAILED)
	{
#endif
		if (SaveHandle)
		{
			std::map<std::string, void*>::iterator it = Events.find(EventName);
			if (it != Events.end())
			{
#if defined _WIN32 || defined _WIN64
				CloseHandle(it->second);
#else
				sem_close(it->second);
				sem_unlink(EventName.c_str());
#endif
				it->second = hEvent;
			}
			else
				Events.insert(std::pair<std::string, void*>(EventName, hEvent));
		}
#if defined _WIN32 || defined _WIN64
		std::uint32_t Result = WaitForSingleObject(hEvent, dwMilliseconds);
		if (!SaveHandle) CloseHandle(hEvent);
#else
		int Result;
		struct timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts) == -1){
			/* handle error */
			return -1;
		}
		ts.tv_nsec += dwMilliseconds * 1000000;
		while ((Result = sem_timedwait(hEvent, &ts)) == -1 && errno == EINTR)
			continue;       /* Restart if interrupted by handler */
		///* Check what happened */
		//if (s == -1){
		//	if (errno == ETIMEDOUT)
		//		printf("sem_timedwait() timed out\n");
		//	else
		//		perror("sem_timedwait");
		//}
		//else
		//	printf("sem_timedwait() succeeded\n");
		if (!SaveHandle) sem_close(hEvent);
#endif
		return Result;
	}
#if defined _WIN32 || defined _WIN64
	CloseHandle(hEvent);
#else
	sem_close(hEvent);
#endif
	return WAIT_FAILED;
}

bool SharedMemory::SetEventSignal(std::string EventName, bool Signaled)
{
	std::map<std::string, void*>::iterator it = Events.find(EventName);
	if (it == Events.end())
	{
		if (Debug)
		{
			std::cout << _T("\nSetEventSignal Error: No Event With That Key Exists!\n");
		}
		return false;
	}
#if defined _WIN32 || defined _WIN64
	if (Signaled) 
		return SetEvent(it->second);
	return ResetEvent(it->second);
#else
	if (Signaled)
		return sem_post(it->second);
	//sem auto reset after post;
#endif
}

bool SharedMemory::DeleteSingleEvent(std::string EventName)
{
	std::map<std::string, void*>::iterator it = Events.find(EventName);
	if (it == Events.end()) return true;
#if defined _WIN32 || defined _WIN64
	bool Result =  CloseHandle(it->second);
#else
	bool Result = sem_close(it->second);
	sem_unlink(EventName.c_str());
#endif
	Events.erase(it);
	return Result;
}

bool SharedMemory::DeleteAllEvents()
{
	bool Result = false;
	for (std::map<std::string, void*>::iterator it = Events.begin(); it != Events.end(); ++it)
	{
#if defined _WIN32 || defined _WIN64
		Result = Result && CloseHandle(it->second);
#else
		Result = Result && sem_close(it->second);
#endif
	}
	Events.clear();
	return Result;
}

void SharedMemory::SetDebug(bool On)
{
	Debug = On;
}
