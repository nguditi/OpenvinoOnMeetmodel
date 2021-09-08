#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <gflags/gflags.h>
#include <opencv2/videoio.hpp>
#include <samples/slog.hpp>

#include "pipelines/async_pipeline.h"
#include "pipelines/config_factory.h"
#include "pipelines/metadata.h"
#include "FrameData.h"
#include "ShareMem.hpp"
#include "segmentation_meet.h"

using namespace InferenceEngine;
typedef std::chrono::duration<double, std::chrono::milliseconds::period> Ms;
static const char help_message[] = "Print a usage message.";
static const char video_message[] = "Required. Path to a video file (specify \"cam\" to work with camera).";
static const char model_message[] = "Required. Path to an .xml file with a trained model.";
static const char target_device_message[] = "Optional. Specify the target device to infer on (the list of available devices is shown below). "
"Default value is CPU. Use \"-d HETERO:<comma-separated_devices_list>\" format to specify HETERO plugin. "
"The demo will look for a suitable plugin for a specified device.";
static const char performance_counter_message[] = "Optional. Enables per-layer performance report.";
static const char custom_cldnn_message[] = "Required for GPU custom kernels. "
"Absolute path to the .xml file with the kernel descriptions.";
static const char custom_cpu_library_message[] = "Required for CPU custom layers. "
"Absolute path to a shared library with the kernel implementations.";
static const char num_inf_req_message[] = "Optional. Number of infer requests.";
static const char num_threads_message[] = "Optional. Number of threads.";
static const char num_streams_message[] = "Optional. Number of streams to use for inference on the CPU or/and GPU in "
"throughput mode (for HETERO and MULTI device cases use format "
"<device1>:<nstreams1>,<device2>:<nstreams2> or just <nstreams>)";
static const char no_show_processed_video[] = "Optional. Do not show processed video.";
static const char utilization_monitors_message[] = "Optional. List of monitors to show initially.";

DEFINE_bool(h, false, help_message);
DEFINE_string(i, "", video_message);
DEFINE_string(m, "", model_message);
DEFINE_string(d, "CPU", target_device_message);
DEFINE_bool(pc, false, performance_counter_message);
DEFINE_string(c, "", custom_cldnn_message);
DEFINE_string(l, "", custom_cpu_library_message);
DEFINE_uint32(nireq, 4, num_inf_req_message);
DEFINE_uint32(nthreads, 4, num_threads_message);
DEFINE_string(nstreams, "", num_streams_message);
DEFINE_bool(no_show, false, no_show_processed_video);
DEFINE_string(u, "", utilization_monitors_message);

bool ParseAndCheckCommandLine(int argc, char *argv[]) {
	// ---------------------------Parsing and validation of input args--------------------------------------
	gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
	if (FLAGS_h) {
		showAvailableDevices();
		return false;
	}
	slog::info << "Parsing input parameters" << slog::endl;

	if (FLAGS_i.empty()) {
		throw std::logic_error("Parameter -i is not set");
	}

	if (FLAGS_m.empty()) {
		throw std::logic_error("Parameter -m is not set");
	}

	return true;
}

float u_coverage_x = 0.5;
float u_coverage_y = 0.75;
float u_lightWrapping = 0.2;

float clamp(float x, float lowerlimit, float upperlimit) {
	if (x < lowerlimit)
		x = lowerlimit;
	if (x > upperlimit)
		x = upperlimit;
	return x;
}

float smoothstep(float edge0, float edge1, float x) {
	// Scale, bias and saturate x to 0..1 range
	x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	// Evaluate polynomial
	return x * x * (3.0 - 2.0 * x);
}

void renderSegmentationData(const SegmentationResult& result, cv::Mat& inputImg, const cv::Mat& background) {
	if (result.frameId == -1) {
		return;
	}
	for (int i = 0; i < inputImg.rows; ++i) {
		for (int j = 0; j < inputImg.cols; ++j) {
			float personMask = result.mask.at<cv::Vec<float, 1>>(i, j)[0];
			personMask = smoothstep(u_coverage_x, u_coverage_y, personMask);
			inputImg.at<cv::Vec3b>(i, j)[0] = inputImg.at<cv::Vec3b>(i, j)[0] * personMask + background.at<cv::Vec3b>(i, j)[0] * (1.0 - personMask);
			inputImg.at<cv::Vec3b>(i, j)[1] = inputImg.at<cv::Vec3b>(i, j)[1] * personMask + background.at<cv::Vec3b>(i, j)[1] * (1.0 - personMask);
			inputImg.at<cv::Vec3b>(i, j)[2] = inputImg.at<cv::Vec3b>(i, j)[2] * personMask + background.at<cv::Vec3b>(i, j)[2] * (1.0 - personMask);
		}
	}
}

std::string ExePath() {
	CHAR buffer[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

int main(int argc, char *argv[]) {
	static char szSemName[] = "zrtcProcessBacground";
	HANDLE hSem;
	hSem = CreateSemaphoreA(NULL, 1, 1, szSemName);
	if (WaitForSingleObject(hSem, 0) == WAIT_TIMEOUT) {
		return -1;
	}

	std::string pathDir = ExePath();
	SharedMemory mem("ShareMemVirtualBackgroundMap");
	mem.MapMemory(VIRTUAL_BACKGROUND_MEMSIZE);
	char* MemPtr = static_cast<char*>(mem.GetDataPointer());
	FrameDataPtrStore frameMem(MemPtr);
	try {
		slog::info << "InferenceEngine: " << printable(*GetInferenceEngineVersion()) << slog::endl;
		FLAGS_d = "CPU";
		FLAGS_m = pathDir + "\\meet3.xml";
		FLAGS_i = "0";
		Core ie;
		AsyncPipeline pipeline(std::unique_ptr<SegmentationModel>(new SegmentationModel(FLAGS_m)),
			ConfigFactory::getUserConfig(FLAGS_d, FLAGS_l, FLAGS_c, FLAGS_pc, FLAGS_nireq, FLAGS_nstreams, FLAGS_nthreads),
			ie);

		std::unique_ptr<ResultBase> result;

		cv::Mat background(cv::Size(640, 480), CV_8UC3);
		cv::Mat capImg(cv::Size(640, 480), CV_8UC3);
		cv::resize(background, background, capImg.size());

		std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration latency{ 0 };
		std::cout << "ready\n";
		mem.CreateNewEvent(nullptr, true, false, "WriteImageReplySignal");
		while (true) {
			int res = mem.OpenSingleEvent("ImageReplySignal", false, true, EVENT_ALL_ACCESS, 1000);
			if (res == WAIT_OBJECT_0)
			{
				t0 = std::chrono::steady_clock::now();
				uchar* MemPtr = static_cast<uchar*>(mem.GetDataPointer());
				if (MemPtr != NULL) {
					if (*frameMem.width != capImg.size().width || *frameMem.height != capImg.size().height) {
						std::cout << "Resize capImg:" << *frameMem.width << " " << *frameMem.height << std::endl;
						cv::resize(capImg, capImg, cv::Size(*frameMem.width, *frameMem.height));
						cv::resize(background, background, cv::Size(*frameMem.width, *frameMem.height));
					}
					if (*frameMem.hasNewPath) {
						std::cout << "update background path: " << pathDir + "\\" + frameMem.backgroundPath << std::endl;
						background = cv::imread(pathDir + "\\" + frameMem.backgroundPath);
						cv::resize(background, background, capImg.size());
						*frameMem.hasNewPath = false;
					}
					capImg.data = (uchar*)frameMem.data;
				}

				if (pipeline.isReadyToProcess()) {
					//--- Capturing frame. If previous frame hasn't been inferred yet, reuse it instead of capturing new one
					pipeline.submitData(ImageInputData(capImg),
						std::make_shared<ImageMetaData>(capImg, t0));

					//--- Waiting for free input slot or output data available. Function will return immediately if any of them are available.
					pipeline.waitForData();

					//--- Checking for results and rendering data if it's ready
					//--- If you need just plain data without rendering - cast result's underlying pointer to SegmentationResult*
					//    and use your own processing instead of calling renderSegmentationData().
					while ((result = pipeline.getResult())) {
						renderSegmentationData(result->asRef<SegmentationResult>(), capImg, background);
					}
				}
				//// ------------ Waiting for completion of data processing and rendering the rest of results ---------
				pipeline.waitForTotalCompletion();
				while (result = pipeline.getResult()) {
					renderSegmentationData(result->asRef<SegmentationResult>(), capImg, background);
				}
				mem.SetEventSignal("ImageReplySignal", false);
				mem.SetEventSignal("WriteImageReplySignal", true);
			}
			else {
				std::cout << "camera not being run\n";
				cv::destroyAllWindows();
				goto Exit_Prog;
			}
		}
	}
	catch (const std::exception& error) {
		slog::err << error.what() << slog::endl;
		goto Exit_Prog;
	}
	catch (...) {
		slog::err << "Unknown/internal exception happened." << slog::endl;
		goto Exit_Prog;
	}
	slog::info << "Execution successful" << slog::endl;

Exit_Prog:
	ReleaseSemaphore(hSem, 1, NULL);
	CloseHandle(hSem);
	return 1;
}
