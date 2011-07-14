#ifndef GRAIN_AUDIO_H
#define GRAIN_AUDIO_H

#include <jack/jack.h>
#include <atomic>
#include <vector>
#include "grainmap.h"

struct point {
  float x, y;
};

// TODO clean up (maybe add another T)
template <class T>
struct atomic_assign {
  std::atomic<T*> next;
  T a, b;
  T* last;

  atomic_assign();
  void write(const T& t);
  T* read();
};

class grainaudio {
  struct out_port {
    jack_port_t* port;
    jack_default_audio_sample_t* buffer;
  };
  std::vector<out_port> output_ports;
  atomic_assign<point> cur_point;
  grainmap& gm;
  float** audio;

  float x0, y0, x1, y1;
  int start, end, starti, endi;
  int cur_sample;
  int cur_dir;
  int counter;

  jack_client_t* client;

public:
  grainaudio(grainmap& gm);
  ~grainaudio();
  void set_point(float x, float y);
  void process(jack_nframes_t nframes);
};

#endif //GRAIN_AUDIO_H
