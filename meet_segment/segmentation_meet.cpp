/*
// Copyright (C) 2018-2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "segmentation_meet.h"
#include "samples/ocv_common.hpp"

using namespace InferenceEngine;

void SegmentationModel::prepareInputsOutputs(InferenceEngine::CNNNetwork& cnnNetwork) {
    // --------------------------- Configure input & output ---------------------------------------------
    // --------------------------- Prepare input blobs -----------------------------------------------------
    ICNNNetwork::InputShapes inputShapes = cnnNetwork.getInputShapes();
    if (inputShapes.size() != 1)
        throw std::runtime_error("Demo supports topologies only with 1 input");
    inputsNames.push_back(inputShapes.begin()->first);
    SizeVector& inSizeVector = inputShapes.begin()->second;
    inSizeVector[0] = 1;  // set batch size to 1
    cnnNetwork.reshape(inputShapes);
	inputMat = cv::Mat(cv::Size(inSizeVector[2], inSizeVector[1]), CV_8UC3);

    InputInfo& inputInfo = *cnnNetwork.getInputsInfo().begin()->second;
    inputInfo.getPreProcess().setResizeAlgorithm(ResizeAlgorithm::RESIZE_BILINEAR);
    inputInfo.setLayout(Layout::NCHW);
    inputInfo.setPrecision(Precision::U8);

    // --------------------------- Prepare output blobs -----------------------------------------------------
    const OutputsDataMap& outputsDataMap = cnnNetwork.getOutputsInfo();
    if (outputsDataMap.size() != 1) throw std::runtime_error("Demo supports topologies only with 1 output");

    outputsNames.push_back(outputsDataMap.begin()->first);
    Data& data = *outputsDataMap.begin()->second;
    // if the model performs ArgMax, its output type can be I32 but for models that return heatmaps for each
    // class the output is usually FP32. Reset the precision to avoid handling different types with switch in
    // postprocessing
    data.setPrecision(Precision::FP32);
    const SizeVector& outSizeVector = data.getTensorDesc().getDims();
    switch (outSizeVector.size()) {
    case 3:
        outChannels = 0;
        outHeight = (int)(outSizeVector[1]);
        outWidth = (int)(outSizeVector[2]);
        break;
    case 4:
        outChannels = (int)(outSizeVector[3]);
        outHeight = (int)(outSizeVector[1]);
        outWidth = (int)(outSizeVector[2]);
        break;
    default:
        throw std::runtime_error("Unexpected output blob shape. Only 4D and 3D output blobs are"
            "supported.");
    }
}

InferenceEngine::Blob::Ptr wrapMat2BlobF(const cv::Mat &mat) {
	size_t channels = mat.channels();
	size_t height = mat.size().height;
	size_t width = mat.size().width;
	InferenceEngine::TensorDesc tDesc(InferenceEngine::Precision::U8,
		{ 1, height, width, channels },
		InferenceEngine::Layout::NCHW);

	return InferenceEngine::make_shared_blob<uchar>(tDesc, (uchar*)mat.data);
}

std::shared_ptr<InternalModelData> SegmentationModel::preprocess(const InputData& inputData, InferenceEngine::InferRequest::Ptr& request) {
    auto imgData = inputData.asRef<ImageInputData>();
    auto& img = imgData.inputImage;
	cv::resize(img, inputMat, inputMat.size());

    request->SetBlob(inputsNames[0], wrapMat2BlobF(inputMat));
    return std::shared_ptr<InternalModelData>(new InternalImageModelData(img.cols, img.rows));
}

void softmax(float *x, int size)
{
	for (int i = 0; i < size; ++i)
	{
		float max = 0.0;
		float sum = 0.0;
		for (int s = 0; s < 2; ++s)
			if (max < x[i * 2 + s])
				max = x[i * 2 + s];
		for (int s = 0; s < 2; ++s)
		{
			x[i * 2 + s] = exp(x[i * 2 + s] - max);    // prevent data overflow
			sum += x[i * 2 + s];
		}
		for (int s = 0; s < 2; ++s) x[i * 2 + s] /= sum;
	}
}

std::unique_ptr<ResultBase> SegmentationModel::postprocess(InferenceResult& infResult) {
    SegmentationResult* result = new SegmentationResult;
    *static_cast<ResultBase*>(result) = static_cast<ResultBase&>(infResult);
    const auto& inputImgSize = infResult.internalModelData->asRef<InternalImageModelData>();

    LockedMemory<const void> outMapped = infResult.getFirstOutputBlob()->rmap();
    float * const predictions = outMapped.as<float*>();
	softmax(predictions, outHeight*outWidth);

    result->mask = cv::Mat(outHeight, outWidth, CV_32FC1);
    for (int rowId = 0; rowId < outHeight; ++rowId) {
        for (int colId = 0; colId < outWidth; ++colId) {
			float pre = predictions[(rowId * outWidth + colId) * 2 + 1];
            result->mask.at<float>(rowId, colId) = pre;
        }
    }
    cv::resize(result->mask, result->mask, cv::Size(inputImgSize.inputImgWidth, inputImgSize.inputImgHeight),0,0,cv::INTER_NEAREST);
	
	cv::blur(result->mask, result->mask, cv::Size(11, 11));
    return std::unique_ptr<ResultBase>(result);
}
