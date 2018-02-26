#include <jack/jack.h>
#include <jack/midiport.h>

/**
 * @brief Jack mockup that more or less says yes to everything
 */

constexpr int JACK_NFRAMES = 128;
constexpr uint64_t FRAMETIME_64_SMP_44100 = 64 * 1000000 / 48000;
uint8_t midi_buffer[3] = {0x81, 60, 45};
float buffer[JACK_NFRAMES];

struct _jack_port
{
    int no{0};
};

struct _jack_client
{
    JackProcessCallback callback_function;
    void* instance;
    _jack_port mocked_ports[10];
};


jack_client_t * jack_client_open (const char* /*client_name*/,
                                  jack_options_t /*options*/,
                                  jack_status_t* status, ...)
{
    *status = JackClientZombie; /* I am zombie client! */
    return new jack_client_t;
}

int jack_client_close (jack_client_t *client)
{
    delete client;
    return 0;
}

jack_nframes_t jack_get_sample_rate(jack_client_t* /*client*/)
{
    return 48000;
}

jack_port_t * jack_port_register (jack_client_t* client,
                                  const char* /*port_name*/,
                                  const char* /*port_type*/,
                                  unsigned long /*flags*/,
                                  unsigned long /*buffer_size*/)
{
    static int i = 0;
    return &client->mocked_ports[i++];
}

int jack_set_process_callback (jack_client_t* client,
                               JackProcessCallback process_callback,
                               void *arg)
{
    client->instance = arg;
    client->callback_function = process_callback;
    return 0;
}

int jack_set_sample_rate_callback (jack_client_t* /*client*/,
                                   JackSampleRateCallback /*callback*/,
                                   void* /*arg*/)
{
    return 0;
}


int jack_set_latency_callback (jack_client_t* /*client*/,
                               JackLatencyCallback /*latency_callback*/,
                               void* /*arg*/)
{
    return 0;
}

int jack_activate (jack_client_t* client)
{
    client->callback_function(JACK_NFRAMES, client->instance);
    return 0;
}

void * jack_port_get_buffer (jack_port_t* /*port*/, jack_nframes_t)
{
    return buffer;
}

uint32_t jack_midi_get_event_count(void* /*port_buffer*/)
{
    return 1;
}

int jack_midi_event_get(jack_midi_event_t *event,
                        void* /*port_buffer*/,
                        uint32_t /*event_index*/)
{
    event->size = 3;
    event->buffer = midi_buffer;
    return 0;
}


int jack_get_cycle_times(const jack_client_t* /*client*/, jack_nframes_t* current_frames,
                         jack_time_t* current_usecs, jack_time_t* next_usecs, float* period_usecs)
{
    *current_frames = 128;
    *current_usecs = 1000;
    *next_usecs = 1000 + FRAMETIME_64_SMP_44100;
    *period_usecs = FRAMETIME_64_SMP_44100;
    return 0;
}

void jack_port_get_latency_range (jack_port_t* /*port*/, jack_latency_callback_mode_t /*mode*/,
                                  jack_latency_range_t* range)
{
    range->min = 0;
    range->max = 0;
    return;
}


/* Functions below are only added for completion, not implemented
 * and shouldn't be called*/
const char ** jack_get_ports (jack_client_t* /*client*/,
                              const char* /*port_name_pattern*/,
                              const char* /*type_name_pattern*/,
                              unsigned long /*flags*/)
{
    return nullptr;
}

int jack_connect (jack_client_t* /*client*/,
                  const char* /*source_port*/,
                  const char* /*destination_port*/)
{
    return 0;
}

const char * jack_port_name (const jack_port_t* /*port*/)
{
    return nullptr;
}

void jack_free(void* /*ptr*/) {}