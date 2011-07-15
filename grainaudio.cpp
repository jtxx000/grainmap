/* grainaudio.cpp
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

#include "grainaudio.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>

#define CHK(stmt) if(!(stmt)) {puts("ERROR: "#stmt); exit(1);}

using namespace std;

static void step_toward(float& x0, float& y0, float x1, float y1) {
  float dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  float dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  if (dx < 1.0f && dy < 1.0f) {
    x0 = x1;
    y0 = y1;
    return;
  }
  if (dy > dx) {
    y0 += sy;
    x0 += (dx/dy)*sx;
  } else {
    x0 += sx;
    y0 += (dy/dx)*sy;
  }
}

struct audio_region {
  int start, end;
};

template <class T>
atomic_assign<T>::atomic_assign() : next(0), last(&a) {}

template <class T>
void atomic_assign<T>::write(const T& t) {
  T* val = next.exchange(0);
  if (val == last) {
    *last = t;
    next.exchange(last);
  } else {
    last = last == &a ? &b : &a;
    *last = t;
    next.exchange(last);
  }
}

template <class T>
T* atomic_assign<T>::read() {
  return next.exchange(0);
}

static int process(jack_nframes_t nframes, void *arg);
static void jack_shutdown(void* arg);

grainaudio::grainaudio(grainmap& gm)
  : gm(gm),
    audio(gm.get_audio()),
    x0(0), y0(0), x1(50), y1(50),
    start(-1), end(-1), starti(-1), endi(-1),
    cur_sample(-1),
    cur_dir(1),
    counter(0)
{
  CHK(client=jack_client_open("grainaudio", JackNullOption, 0));
  jack_set_process_callback(client, ::process, this);
  jack_on_shutdown(client, jack_shutdown, 0);
  for (int i=0; i<gm.channel_count(); i++) {
    ostringstream port_name;
    port_name << "out " << i;
    out_port p = {jack_port_register(client,
                                     port_name.str().c_str(),
                                     JACK_DEFAULT_AUDIO_TYPE,
                                     JackPortIsOutput | JackPortIsTerminal,
                                     0)};
    output_ports.push_back(p);
  }

  CHK(!jack_activate(client));

  const char **ports;

  CHK(ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput));
  for (auto it = output_ports.begin(); ports && it != output_ports.end(); ++it) {
    CHK(!jack_connect(client, jack_port_name(it->port), *ports++));
  }
}

grainaudio::~grainaudio() {
  jack_deactivate(client);
}

void grainaudio::set_point(float x, float y) {
  //printf("setting region %d %d\n", start, end);
  point new_point {x,y};
  cur_point.write(new_point);
}

void grainaudio::process(jack_nframes_t nframes) {
  point* next_point = cur_point.read();
  if (next_point) {
    x1 = next_point->x;
    y1 = next_point->y;
  }

  for (int chan=0; chan<output_ports.size(); chan++) {
    output_ports[chan].buffer =
      (jack_default_audio_sample_t*)
      jack_port_get_buffer(output_ports[chan].port, nframes);
  }

  for (int i=0; i<nframes; i++) {
    if (counter-- <= 0) {
      counter = 10;
      if (x0 != x1 || y0 != y1) {
        //puts("grabbing");
        step_toward(x0,y0,x1,y1);
        gm.lookup(x0, y0, start, end, starti, endi);
        if (cur_sample < start || cur_sample >= end) {
          //puts("new thing");
          // cross fade
          cur_sample = start;
          cur_dir = 1;
        }
      }
    }

    for (int chan=0; chan<output_ports.size(); chan++) {
      output_ports[chan].buffer[i] =
        start == -1 ? 0 : audio[chan][cur_sample];
    }

    cur_sample += cur_dir;
    if (cur_sample >= end) {
      cur_sample = end-1;
      cur_dir = -1;
    } else if (cur_sample < start) {
      cur_sample = start;
      cur_dir = 1;
    }
  }
}

static int process(jack_nframes_t nframes, void *arg) {
  ((grainaudio*)arg)->process(nframes);
  return 0;
}

static void jack_shutdown(void* arg) {
  exit(1);
}
