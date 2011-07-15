/* grainaudio.h
 *
 * Copyright 2011 Caleb Reach
 * 
 * This file is part of Grainmap
 *
 * Grainmap is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Grainmap is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Grainmap.  If not, see <http://www.gnu.org/licenses/>.
 */

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
