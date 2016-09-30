/**
 * @brief The public interface of an audio plugin class
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

/* The number of samples to process in one chunk. It is defined as a 
compile time constant to give more room for omptimizations */ 
static constexpr unsigned int AUDIO_CHUNK_SIZE = 64;

/* Return Status Enum */
enum class AudioProcessorStatus 
{
	OK,
	ERROR,
	PARAMETER_ERROR,
	MEMORY_ERROR,
};

struct AudioProcessorConfig
{
	unsigned int sample_rate;
};

class AudioProcessorBase 
{
public:

	virtual ~AudioProcessorBase() {};

/* (Re)initialize the plugin instance. This is called from the host's non-
realtime environment at startup and configuration changes. 
Not called at the same time as process(). When returning OK, the plugin 
should be in a default state, i.e. filter registers, reverb tails, etc should 
be zeroed */ 
	virtual AudioProcessorStatus init(const AudioProcessorConfig& configuration) = 0;

/* Optional, makes for the second most minimal interface if included.
Called before process() if called from the realtime environment.
More can be added for setting other types of parameters (bool, int..) */
	virtual void set_parameter(unsigned int parameter_id, float value) = 0;

/* Called by the host from the real time processing environment once for
every chunk. in_buffer and out_buffer are  AUDIO_CHUNK_SIZE long arrays 
of audio data. The function should handle the case when in_buffer and 
out_buffer points to the same memory location, for replacement processing.*/
	virtual void process(const float *in_buffer, float *out_buffer) = 0;
};


#endif // PLUGIN_INTERFACE_H