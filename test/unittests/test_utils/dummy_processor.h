#ifndef SUSHI_DUMMY_PROCESSOR_H
#define SUSHI_DUMMY_PROCESSOR_H

#include "library/processor.h"

using namespace sushi;

class DummyProcessor : public Processor
{
public:
    explicit DummyProcessor(HostControl host_control) : Processor(host_control)
    {
        _max_input_channels = 2;
        _max_output_channels = 2;
        _current_input_channels = _max_input_channels;
        _current_output_channels = _max_output_channels;
        set_name("processor");
        this->register_parameter(new ParameterDescriptor("param 1", "param 1", "", ParameterType::FLOAT));
        this->register_parameter(new ParameterDescriptor("gain", "gain", "", ParameterType::FLOAT));
    }

    ProcessorReturnCode init(float /* sample_rate */) override
    {
        return ProcessorReturnCode::OK;
    }

    void process_event(const RtEvent& /*event*/) override {}
    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override
    {
        out_buffer = in_buffer;
    }
};

class DummyMonoProcessor : public DummyProcessor
{
public:
    explicit DummyMonoProcessor(HostControl host_control) :DummyProcessor(host_control)
    {
        _max_input_channels = 1;
        _max_output_channels = 1;
        _current_input_channels = _max_input_channels;
        _current_output_channels = _max_output_channels;
    }
};

#endif //SUSHI_DUMMY_PROCESSOR_H
