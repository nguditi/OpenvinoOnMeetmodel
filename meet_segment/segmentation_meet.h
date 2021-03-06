/*
// Copyright (C) 2018-2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writingb  software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "models/model_base.h"
#include "opencv2/core.hpp"

#pragma once
class SegmentationModel : public ModelBase {
public:
    /// Constructor
    /// @param model_nameFileName of model to load
    SegmentationModel(const std::string& modelFileName) : ModelBase(modelFileName) {}

    virtual std::shared_ptr<InternalModelData> preprocess(const InputData& inputData, InferenceEngine::InferRequest::Ptr& request) override;
    virtual std::unique_ptr<ResultBase> postprocess(InferenceResult& infResult);

protected:
    virtual void prepareInputsOutputs(InferenceEngine::CNNNetwork & cnnNetwork) override;

    int outHeight = 0;
    int outWidth = 0;
    int outChannels = 0;
	cv::Mat inputMat;
};
