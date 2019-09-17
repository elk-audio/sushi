#include "processor.h"

namespace sushi {

ProcessorReturnCode Processor::connect_cv_from_parameter(ObjectId parameter_id, int cv_output_id)
{
    if (cv_output_id >= static_cast<int>(_cv_out_connections.size()) ||
        _outgoing_cv_connections >= static_cast<int>(_cv_out_connections.size()))
    {
        return ProcessorReturnCode::ERROR;
    }
    bool param_exists = false;
    for (const auto& p : this->all_parameters())
    {
        // Loop over all parameters since parameter ids don't necessarily match indexes (VST 3 for instance)
        if (p->id() == parameter_id)
        {
            param_exists = true;
            break;
        }
    }
    if (param_exists == false)
    {
        return ProcessorReturnCode::PARAMETER_NOT_FOUND;
    }
    _cv_out_connections[_outgoing_cv_connections].parameter_id = parameter_id;
    _cv_out_connections[_outgoing_cv_connections].cv_id = cv_output_id;
    _outgoing_cv_connections++;
    return ProcessorReturnCode::OK;
}

ProcessorReturnCode Processor::connect_gate_from_processor(int gate_output_id, int channel, int note_no)
{
    assert(gate_output_id < MAX_ENGINE_GATE_PORTS || note_no <= MAX_ENGINE_GATE_NOTE_NO);
    GateKey key = to_gate_key(channel, note_no);
    if (_outgoing_gate_connections.count(key) > 0)
    {
        return ProcessorReturnCode::ERROR;
    }
    GateOutConnection con;
    con.channel = channel;
    con.note = note_no;
    con.gate_id = gate_output_id;
    _outgoing_gate_connections[key] = con;

    return ProcessorReturnCode::OK;
}

bool Processor::register_parameter(ParameterDescriptor* parameter, ObjectId id)
{
    for (auto& p : _parameters_by_index)
    {
        if (p->id() == id) return false; // Don't allow duplicate parameter id:s
    }
    bool inserted = true;
    std::tie(std::ignore, inserted) = _parameters.insert(std::pair<std::string, std::unique_ptr<ParameterDescriptor>>(parameter->name(), std::unique_ptr<ParameterDescriptor>(parameter)));
    if (!inserted)
    {
        return false;
    }
    parameter->set_id(id);
    _parameters_by_index.push_back(parameter);
    return true;
}

bool Processor::maybe_output_cv_value(ObjectId parameter_id, float value)
{
    // Linear complexity lookup, though the number of outgoing connections are a handful at max
    // and this is in memory which should already be cached, so very efficient
    for (int i = 0; i < _outgoing_cv_connections; ++i)
    {
        auto& connection = _cv_out_connections[i];
        if (parameter_id == connection.parameter_id)
        {
            output_event(RtEvent::make_cv_event(this->id(), 0, connection.cv_id, value));
            return true;
        }
    }
    return false;
}

bool Processor::maybe_output_gate_event(int channel, int note, bool note_on)
{
    auto con = _outgoing_gate_connections.find(to_gate_key(channel, note));
    if (con == _outgoing_gate_connections.end())
    {
        return false;
    }
    output_event(RtEvent::make_gate_event(this->id(), 0, con->second.gate_id, note_on));
    return true;
}

void Processor::bypass_process(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if (_current_input_channels == 0)
    {
        out_buffer.clear();
    }
    else if (_current_input_channels == _current_output_channels || _current_input_channels == 1)
    {
        out_buffer = in_buffer;
    }
    else
    {
        for (int c = 0; c < _current_output_channels; ++c)
        {
            out_buffer.replace(c, c % _current_input_channels, in_buffer);
        }
    }
}

void BypassManager::crossfade_output(const ChunkSampleBuffer& input_buffer, ChunkSampleBuffer& output_buffer,
                                     int input_channels, int output_channels)
{
    auto [start, end] = get_ramp();
    output_buffer.ramp(start, end);
    if (input_channels > 0)
    {
        for (int c = 0; c < output_channels; ++c)
        {
            // Add the input with an inverse ramp to crossfade between input and output
            output_buffer.add_with_ramp(c, c % input_channels, input_buffer, 1.0f - start, 1.0f - end);
        }
    }
}

std::pair<float, float> BypassManager::get_ramp()
{
    int prev_count = 0;
    if (_state == BypassState::RAMPING_DOWN)
    {
        prev_count = _ramp_count--;
        if (_ramp_count == 0)
        {
            _state = BypassState::BYPASSED;
        }
    }
    else if (_state == BypassState::RAMPING_UP)
    {
        prev_count = _ramp_count++;
        if (_ramp_count == _ramp_chunks)
        {
            _state = BypassState::NOT_BYPASSED;
        }
    }
    else
    {
        return {1.0f, 1.0f};
    }
    return {static_cast<float>(prev_count) / _ramp_chunks,
            static_cast<float>(_ramp_count) / _ramp_chunks};
}

} // namespace sushi
