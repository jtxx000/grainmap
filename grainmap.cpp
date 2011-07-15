/* grainmap.cpp
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

#include "grainmap.h"
#include "hilbert.h"
#include "five-color.h"

#include <stdio.h>
#include <math.h>
#include <samplerate.h>
#include <sndfile.h>
#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <limits.h>
#include <string.h>
#include <aubio.h>

using namespace std;

class env_fol {
  float level;
  float decay;

public:
  env_fol(float decay) : level(0), decay(decay) {}

  void process(float* data, int num) {
    while (--num>=0) {
      float in = fabsf(*data);
      *data++ = level = (in >= level ? in : (in + decay*(level - in)));
    }
  }
};

static inline bool in_bounds(int nsize, bitmask_t coords[2]) {
  bitmask_t max = 1 << nsize;
  return coords[0] >= 0 && coords[1] >= 0 && coords[0] < max && coords[1] < max;
}

struct region {
  int start, end;

  bool contains(int point) {
    return point >= start && point < end;
  }
};

typedef map<int,five_color::vertex*> region_map;

static inline region_map::iterator find_vertex(region_map& regions, int index) {
  assert(index >= 0);
  return --regions.upper_bound(index);
}

static inline void traverse_region(region r,
                                   five_color::vertex* vtx,
                                   five_color& fc,
                                   region_map& regions,
                                   int nsize)
{
  //printf("traverse begin\n");

  bitmask_t coords[2], orig_coords[2];
  hilbert_i2c(2, nsize, r.start, coords);

  int nx, ny;
  if (r.start) {
    bitmask_t last_coords[2];
    hilbert_i2c(2, nsize, r.start-1, last_coords);
    nx = last_coords[0] - coords[0];
    ny = last_coords[1] - coords[1];
  } else {
    nx = 0;
    ny = -1;
  }
  bitmask_t foreign_coords[2];
  orig_coords[0] = foreign_coords[0] = coords[0] + nx;
  orig_coords[1] = foreign_coords[1] = coords[1] + ny;

  int iter = 0;
  region foreign_region = {0,-1};
  do {
    int index;
    if (in_bounds(nsize, foreign_coords) &&
        !(foreign_region.contains(index=hilbert_c2i(2, nsize, foreign_coords))))
    {
      //printf("traverse handle %d %d\n", index, r.contains(index));
      if (r.contains(index)) {
        coords[0] = foreign_coords[0];
        coords[1] = foreign_coords[1];
        // rotate normal counterclockwise
        //printf("traverse rotate normal counterclockwise\n");
        int tnx = nx;
        nx = ny;
        ny = -tnx;
      } else {
        //printf("traverse new region\n");
        // new region
        auto it = find_vertex(regions, index);
        assert(it != regions.end());
        assert(it->second != vtx);
        //printf("add edge %p -> %p\n", vtx, it->second);
        fc.add_edge(vtx, it->second);
        foreign_region.start = it->first;
        foreign_region.end = it == regions.end() ? INT_MAX : (++it)->first;
      }
    }

    // move clockwise-normal
    coords[0] -= ny;
    coords[1] += nx;
    if (!(in_bounds(nsize, coords) &&
          r.contains(hilbert_c2i(2, nsize, coords))))
    {
      //printf("traverse go back\n");
      // go back
      coords[0] += ny;
      coords[1] -= nx;

      // rotate normal clockwise
      int tnx = nx;
      nx = -ny;
      ny = tnx;
    }
    iter++;

    foreign_coords[0] = coords[0] + nx;
    foreign_coords[1] = coords[1] + ny;
  } while (foreign_coords[0] != orig_coords[0] || foreign_coords[1] != orig_coords[1]);
}

static const int BUF_SIZE = 256;

struct audio_file {
  float* data;
  int size;

  audio_file(const string& path) {
    SF_INFO info = {0};
    SNDFILE* file;
    file = sf_open(path.c_str(), SFM_READ, &info);
    size = info.frames;
    assert(info.channels == 1);
    data = new float[size];
    sf_readf_float(file, data, size);
    sf_close(file);
  }

  ~audio_file() {
    delete data;
  }

  inline float operator[](int i) const {
    return data[i];
  }
};

static const float colors[][3] = {
  {119,32,61},
  {211,80,63},
  {217,152,28},
  {202,239,85},
  {78,108,103}

  // {140,30,29},
  // {212,66,31},
  // {75,138,121},
  // {208,217,105},
  // {242,140,62}

  // {125,130,88},
  // {207,185,124},
  // {229,137,79},
  // {199,92,68},
  // {166,62,56}
};

struct grain_draw {
  region_map::iterator it, end;
  int color, end_samp, i;

  grain_draw(region_map::iterator begin, region_map::iterator end)
    : it(begin),
      end(end),
      end_samp(-1),
      i(0)
  {
  }

  void process(cairo_image& img, int nsize, float* data, int size) {
    while (--size>=0) {
      if (i >= end_samp) {
        color = it->second->color;
        end_samp = ++it == end ? INT_MAX : it->first;
        //printf("process color %d %d\n", color, end_samp);
      }

      bitmask_t coords[2];
      hilbert_i2c(2, nsize, i++, coords);
      const float* col = colors[color];
      //float v = (*data++)*0.85 + 0.15;
      double p = 1 + log(*data++)*0.15;
      if (p > 1) p = 1;
      if (p < 0) p = 0;
      if (std::isnan(p)) p = 0;
      assert(p >= 0 && p <= 1);
      //double p = 1.4;
      img.set(coords[0], coords[1],
              col[0]*p,
              col[1]*p,
              col[2]*p);
      //printf("    v %f\n", v);
    }
  }
};

static unique_ptr<audio_data> read_and_detect(const std::string& path,
                                              map<int,int>& regions,
                                              int out_size)
{
  SF_INFO info = {0};
  SNDFILE* file = sf_open(path.c_str(), SFM_READ, &info);
  assert(file);
  double ratio = out_size/(double)info.frames;
  unique_ptr<audio_data> adata(new audio_data(info.frames, info.channels));
  unique_ptr<float[]> buf(new float[info.frames*info.channels]);
  aubio_onset_t* onset = new_aubio_onset(aubio_onset_kl, BUF_SIZE*2, BUF_SIZE, 1);
  auto ins = regions.begin();
  fvec_t* in_vec = new_fvec(BUF_SIZE, info.channels);
  fvec_t* onset_vec = new_fvec(1,1);
  int cur_sample = 0;
  int num;
  while ((num=sf_readf_float(file, buf.get(), BUF_SIZE))) {
    //printf("num %d\n", num);

    for (int chan=0; chan<info.channels; chan++)
      for (int i=0; i<num; i++) {
        adata->data[chan][cur_sample+i] =
          in_vec->data[chan][i] =
          buf[i*info.channels + chan];
      }

    aubio_onset(onset, in_vec, onset_vec);
    if (**onset_vec->data || !cur_sample) {
      // TODO backtrack to zero crossing
      // TODO record actual cur_sample
      int samp = max(cur_sample - BUF_SIZE*4, 0);
      int pos = max((int)(samp*ratio), 0);
      //printf("onset %d %d\n", pos, cur_sample);
      ins = regions.insert(ins, map<int,int>::value_type(pos, samp));
    }

    cur_sample += num;
  }
  del_aubio_onset(onset);
  del_fvec(in_vec);
  del_fvec(onset_vec);
  sf_close(file);

  return adata;
}

static void construct_edges(five_color& fc, region_map& regions, int nsize) {
  for (auto it=regions.begin(); it != regions.end();) {
    region r;
    r.start = it->first;
    five_color::vertex* v = it->second;
    ++it;
    r.end = it == regions.end() ? INT_MAX : it->first;
    traverse_region(r, v, fc, regions, nsize);
  }
}

// TODO maybe use this elsewhere
static int lookup_index(int nsize, bitmask_t x, bitmask_t y) {
  bitmask_t coords[2];
  coords[0] = x;
  coords[1] = y;
  return hilbert_c2i(2, nsize, coords);
}

static unique_ptr<cairo_image> resample_and_draw(region_map& regions,
                                                 audio_data& adata,
                                                 int nsize,
                                                 int out_size)
{
  grain_draw draw(regions.begin(), regions.end());

  int src_err;
  SRC_STATE* src = src_new(SRC_SINC_FASTEST, 1, &src_err);
  assert(src);

  float buf[BUF_SIZE];
  int w = 1 << nsize;
  unique_ptr<cairo_image> img(new cairo_image(w, w));
  env_fol env(.99);
  SRC_DATA data;
  data.src_ratio = out_size/(double)adata.size;
  data.end_of_input = 0;
  data.input_frames = BUF_SIZE;
  data.output_frames = (int)(BUF_SIZE*data.src_ratio + BUF_SIZE);
  unique_ptr<float[]> out(new float[data.output_frames]);
  data.data_in = buf;
  data.data_out = out.get();

  for (int i=0; i<adata.size; i+=BUF_SIZE) {
    int size = data.input_frames = min(adata.size-i, BUF_SIZE);
    memset(buf, 0, sizeof(float)*size);
    for (int chan=0; chan<adata.channels; chan++)
      for (int j=0; j<BUF_SIZE; j++)
        buf[j] = max(buf[j], adata.data[chan][i]);

    env.process(buf, data.input_frames);
    src_err = src_process(src, &data);
    assert(!src_err);
    assert(data.input_frames_used == data.input_frames);
    draw.process(*img, nsize, out.get(), data.output_frames_gen);
  }

  bitmask_t x, y;
  region cur_reg = {0,-1};
  for (bitmask_t y=0; y<w; y++) {
    for (bitmask_t x=0; x<w; x++) {
      int index = lookup_index(nsize, x, y);
      if (!cur_reg.contains(index)) {
        auto it = find_vertex(regions, index);
        // TODO clean
        cur_reg.start = it->first;
        ++it;
        cur_reg.end = it == regions.end() ? INT_MAX : it->first;
      }
      if (x == 0 || x == w-1 ||
          y == 0 || y == w-1 ||
          !(cur_reg.contains(lookup_index(nsize, x-1, y-1)) &&
            cur_reg.contains(lookup_index(nsize, x-1, y  )) &&
            cur_reg.contains(lookup_index(nsize, x-1, y+1)) &&

            cur_reg.contains(lookup_index(nsize, x,   y-1)) &&
            cur_reg.contains(lookup_index(nsize, x,   y+1)) &&

            cur_reg.contains(lookup_index(nsize, x+1, y-1)) &&
            cur_reg.contains(lookup_index(nsize, x+1, y  )) &&
            cur_reg.contains(lookup_index(nsize, x+1, y+1))))
        img->mark(x,y);

    }
  }


  src_delete(src);

  return img;
}

audio_data::audio_data(int size, int channels)
  : data(new float*[channels]),
    size(size),
    channels(channels)
{
  for (int i=0; i<channels; i++)
    data[i] = new float[size];
}

audio_data::~audio_data() {
  for (int i=0; i<channels; i++)
    delete[] data[i];
  delete[] data;
}

cairo_image::cairo_image(int width, int height)
  : stride(cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width)),
    width(width),
    height(height)
{
  int size = height*stride;
  data = new unsigned char[size];
}

cairo_image::~cairo_image() {
  delete[] data;}

void cairo_image::set(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
  *(uint32_t*)(data + y*stride + x*4) = r << 16 | g << 8 | b;
}

void cairo_image::mark(int x, int y) {
  // TODO abstract
  uint32_t c = *(uint32_t*)(data + y*stride + x*4);
  unsigned char r = (c >> 16) & 0xFF;
  unsigned char g = (c >> 8) & 0xFF;
  unsigned char b = c & 0xFF;
  double p = 0.10;
  double q = 1-p;
  set(x, y, r*q + 255*p, g*q + 255*p, b*q + 255*p);
  //set(x, y, 255, 255, 255);
}

Cairo::RefPtr<Cairo::ImageSurface> cairo_image::create_surface() {
  return Cairo::ImageSurface::create(data, Cairo::FORMAT_RGB24, width, height, stride);
}

grainmap::grainmap(const std::string& path) {
  int out_size = 1 << nsize;
  out_size = out_size*out_size;
  region_map regions;
  five_color fc;
  // printf("## reading\n");
  adata = read_and_detect(path, region_starts, out_size);
  for (auto it=region_starts.begin(); it!=region_starts.end(); ++it)
    regions.insert(region_map::value_type(it->first, fc.create_vertex()));
  // printf("## constructing edges\n");
  construct_edges(fc, regions, nsize);
  // printf("## coloring\n");
  fc.color();
  // printf("## drawing\n");
  cimg = resample_and_draw(regions, *adata, nsize, out_size);
  img = cimg->create_surface();
  // printf("## writing\n");
  // cairo_surface_t* surface = img->create_surface();
  // img->write_to_png("bin/out.png");
  // cairo_surface_destroy(surface)
}

float** grainmap::get_audio() {
  return adata->data;
}

int grainmap::channel_count() {
  return adata->channels;
}

// start and end are samples; starti, endi are hilbert indexes
void grainmap::lookup(int x, int y, int& start, int& stop, int& starti, int& endi) {
  // TODO merge with find_vertex
  bitmask_t coords[2];
  coords[0] = x;
  coords[1] = y;
  int index = hilbert_c2i(2, nsize, coords);
  assert(index >= 0);
  if (index >= starti && index < endi)
    return;
  auto it = --region_starts.upper_bound(index);
  starti = it->first;
  start = it->second;
  assert(start >= 0);
  ++it;
  // TODO this sort of thing appears in several places
  endi = it == region_starts.end() ? INT_MAX : it->first;
  stop = it == region_starts.end() ? adata->size : it->second;
}

Cairo::RefPtr<Cairo::ImageSurface> grainmap::get_surface() {
  return img;
}

