// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <vpu/frontend/frontend.hpp>

#include <vector>
#include <unordered_set>
#include <memory>
#include <set>
#include <string>

namespace vpu {

namespace {

class ReshapeStage final : public StageNode {
private:
    StagePtr cloneImpl() const override {
        return std::make_shared<ReshapeStage>(*this);
    }

    void propagateScaleFactorsImpl(
            const SmallVector<float>& inputScales,
            ScalePropagationStep step,
            StageDataInfo<float>& scaleInfo) override {
        if (step == ScalePropagationStep::Propagate) {
            scaleInfo.setOutput(outputEdge(0), inputScales[0]);
        } else {
            // Reshape can only propagate scaling.
            scaleInfo.setInput(inputEdge(0), 1.0f);
            scaleInfo.setOutput(outputEdge(0), 1.0f);
        }
    }

    void propagateDataOrderImpl(StageDataInfo<DimsOrder>& orderInfo) override {
        auto input = inputEdge(0)->input();
        auto output = outputEdge(0)->output();

        // Only default order is supported
        orderInfo.setInput(inputEdge(0), DimsOrder::fromNumDims(input->desc().numDims()));
        orderInfo.setOutput(outputEdge(0), DimsOrder::fromNumDims(output->desc().numDims()));
    }

    void getDataStridesRequirementsImpl(StageDataInfo<StridesRequirement>& stridesInfo) override {
        stridesInfo.setInput(inputEdge(0), StridesRequirement::compact());
        stridesInfo.setOutput(outputEdge(0), StridesRequirement::compact());
    }

    void finalizeDataLayoutImpl() override {
    }

    void getBatchSupportInfoImpl(StageDataInfo<BatchSupport>& batchInfo) override {
    }

    void initialCheckImpl() const override {
        const auto& firstInputPrecision = input(0)->desc().type();
        assertInputsOutputsTypes(this, {{firstInputPrecision}}, {{firstInputPrecision}});
        IE_ASSERT(input(0)->desc().totalDimSize() == output(0)->desc().totalDimSize());
    }

    void serializeParamsImpl(BlobSerializer&) const override {
        VPU_THROW_EXCEPTION << "Must never be called";
    }

    void serializeDataImpl(BlobSerializer&) const override {
        VPU_THROW_EXCEPTION << "Must never be called";
    }
};

}  // namespace

void FrontEnd::parseReshape(
        const Model::Ptr& model,
        const ie::CNNLayerPtr& layer,
        const DataVector& inputs,
        const DataVector& outputs) {
    IE_ASSERT(inputs.size() == 1);
    IE_ASSERT(outputs.size() == 1);
    _stageBuilder->addReshapeStage(model, layer->name, layer, inputs[0], outputs[0]);
}

Stage StageBuilder::addReshapeStage(
        const Model::Ptr& model,
        const std::string& name,
        const ie::CNNLayerPtr& layer,
        const Data& input,
        const Data& output) {
    IE_ASSERT(input->desc().totalDimSize() == output->desc().totalDimSize());

    return model->addNewStage<ReshapeStage>(
        name,
        StageType::Reshape,
        layer,
        {input},
        {output});
}

}  // namespace vpu
