// meet_camera.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/core.hpp"

#include "ShareMem.hpp"
#include "FrameData.h"

SharedMemory mem("ShareMemVirtualBackgroundMap");
FrameDataPtrStore memFrame;

bool hasInit = false;
char backgroundImage[256] = "background0.jpg";
bool backgroundUpdate = true;

void tryStartSegmentProcess() {
	std::cout << "Start segment child process" << std::endl;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	LPTSTR szCmdline = _tcsdup(TEXT("meet_segment.exe"));
	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
		szCmdline,       // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		DETACHED_PROCESS,
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		) {
		int error = GetLastError();
		printf("CreateProcess failed (%d).\n", error);
		return;
	}
}

void processSegmentFrame(cv::Mat& frame) {
	if (!hasInit) {
		mem.MapMemory(VIRTUAL_BACKGROUND_MEMSIZE);
		mem.CreateNewEvent(nullptr, true, false, "ImageReplySignal");
		tryStartSegmentProcess();
		char* MemPtr = static_cast<char*>(mem.GetDataPointer());
		memFrame.mapMemPtr(MemPtr);

		hasInit = true;
	}
	if (backgroundUpdate) {
		memcpy(memFrame.backgroundPath, &backgroundImage, 256);
		*memFrame.hasNewPath = true;
		backgroundUpdate = false;
	}
	cv::Size s = frame.size();
	*memFrame.width = s.width;
	*memFrame.height = s.height;
	*memFrame.size = s.width * s.height * 3;
	memcpy(memFrame.data, frame.data, *memFrame.size);

	//signal segment process to run
	mem.SetEventSignal("ImageReplySignal", true);
	//wait segment process for 200ms;
	int res = mem.OpenSingleEvent("WriteImageReplySignal", false, true, EVENT_ALL_ACCESS, 200);
	if (res == WAIT_OBJECT_0) {
		//signal segment process to wait next frame
		mem.SetEventSignal("WriteImageReplySignal", false);
		memcpy(frame.data, memFrame.data, *memFrame.size);
	}
	else if (res == STATUS_TIMEOUT) {
		//stop using segment
		mem.DeleteSingleEvent("WriteImageReplySignal");
	}
}

int main()
{
	cv::Mat capImg(cv::Size(640, 480), CV_8UC3);
	cv::VideoCapture videoCapturer;
	videoCapturer.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH, 640);
	videoCapturer.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT, 480);
	videoCapturer.open(0);
	std::cout << "Camera running" << std::endl;
	while (true) {
		videoCapturer >> capImg;

		//process background in other process like Zoom Meeting
		processSegmentFrame(capImg);

		cv::imshow("Camera source", capImg);
		char key = (char)cv::waitKey(30);
		if (key == 27 || key == 'q'){
			break;
		}
	}
	std::cout << "End";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
