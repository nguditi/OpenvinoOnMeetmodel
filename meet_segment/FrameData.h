#pragma once
#include <iostream>
#define PATH_SIZE 256
struct FrameDataPtrStore
{
	bool* hasNewPath = NULL;
	char* backgroundPath = NULL;
	int* width = NULL;
	int* height = NULL;
	int* size = NULL;
	char* data = NULL;

	FrameDataPtrStore() {

	}

	FrameDataPtrStore(char* MemPtr) {
		hasNewPath = (reinterpret_cast<bool*>(MemPtr));
		MemPtr += sizeof(bool);
		backgroundPath = MemPtr;
		MemPtr += PATH_SIZE;
		width = (reinterpret_cast<int*>(MemPtr));
		MemPtr += 4;
		height = (reinterpret_cast<int*>(MemPtr));
		MemPtr += 4;
		size = (reinterpret_cast<int*>(MemPtr));
		MemPtr += 4;
		data = MemPtr;
	}

	void mapMemPtr(char* MemPtr) {
		hasNewPath = (reinterpret_cast<bool*>(MemPtr));
		MemPtr += sizeof(bool);
		backgroundPath = MemPtr;
		MemPtr += PATH_SIZE;
		width = (reinterpret_cast<int*>(MemPtr));
		MemPtr += 4;
		height = (reinterpret_cast<int*>(MemPtr));
		MemPtr += 4;
		size = (reinterpret_cast<int*>(MemPtr));
		MemPtr += 4;
		data = MemPtr;
	}
};