#include <stdio.h>
#include <math.h>
#include <samplerate.h>
#include <sndfile.h>
#include <assert.h>
#include <cairo.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <limits.h>
#include <map>
#include <aubio.h>

#include "hilbert.h"
#include "five-color.h"

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

typedef map<int,vertex*> region_map;

static inline region_map::iterator find_vertex(region_map& regions, int index) {
  assert(index >= 0);
  return --regions.upper_bound(index);
}

static inline void traverse_region(region r,
                                   vertex* vtx,
                                   five_color& fc,
                                   region_map& regions,
                                   int nsize)
{
  bitmask_t coords[2], orig_coords[2];
  hilbert_i2c(2, nsize, r.start, coords);
  orig_coords[0] = coords[0];
  orig_coords[1] = coords[1];

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

  region foreign_region;
  do {
    bitmask_t foreign_coords[2];
    foreign_coords[0] = coords[0] + nx;
    foreign_coords[1] = coords[1] + ny;
    int index;
    if (in_bounds(nsize, foreign_coords) &&
        !(foreign_region.contains(index=hilbert_c2i(2, nsize, foreign_coords))))
    {
      if (r.contains(index)) {
        coords[0] = foreign_coords[0];
        coords[1] = foreign_coords[1];
        // rotate normal counterclockwise
        int tnx = nx;
        nx = ny;
        ny = -tnx;
      } else {
        // new region
        auto it = find_vertex(regions, index);
        assert(it != regions.end());
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
      // go back
      coords[0] += ny;
      coords[1] -= nx;

      // rotate normal clockwise
      int tnx = nx;
      nx = -ny;
      ny = tnx;
    }
  } while (coords[0] != orig_coords[0] || coords[1] != orig_coords[1]);
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

struct cairo_image {
  int width, height, stride;
  unsigned char* data;

  cairo_image(int width, int height)
    : stride(cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width)),
      width(width),
      height(height)
  {
    int size = height*stride;
    data = new unsigned char[size];
  }

  ~cairo_image() {
    delete data;}

  void set(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    *(uint32_t*)(data + y*stride + x*4) = r << 16 | g << 8 | b;
  }

  cairo_surface_t* create_surface() {
    return cairo_image_surface_create_for_data(data, CAIRO_FORMAT_RGB24, width, height, stride);
  }
};

static const float colors[][3] = {
  {1,1,1},
  {1,0,1},
  {1,1,0},
  {0,1,1},
  {0,0,1}
};

void draw_region(audio_file& resampled_audio, int nsize, region& r, int color) {
  int w = 1 << nsize;
  cairo_image image(w, w);
  for (int i=r.start; i<r.end; i++) {
    bitmask_t coords[2];
    hilbert_i2c(2, nsize, i, coords);
    const float* col = colors[color];
    float v = resampled_audio[i];
    image.set(coords[0], coords[1], col[0]*v, col[1]*v, col[2]*v);
  }
}

void draw(audio_file& audio, int nsize, region_map& regions) {
  for (auto it=regions.begin(); it != regions.end();) {
    region r;
    r.start = it->first;
    int color = it->second->color;
    ++it;
    r.end = it == regions.end() ? audio.size : it->first;
    draw_region(audio, nsize, r, color);
  }
}

void grain() {
  audio_file audio("foo");
  region_map regions;
  five_color fc;

  // detect

  // resample

  const int nsize = 8;
  for (auto it=regions.begin(); it != regions.end();) {
    region r;
    r.start = it->first;
    vertex* v = it->second;
    ++it;
    r.end = it == regions.end() ? 0 : it->first;
    traverse_region(r, v, fc, regions, nsize);
  }

  fc.color();
  draw(audio, nsize, regions);
}

void create_grain_map(char* path, env_fol* env, int nsize) {
  SF_INFO info = {0};
  SNDFILE* file;
  file = sf_open(path, SFM_READ, &info);
  assert(file);
  assert(info.channels == 1);

  int out_size = 1 << nsize;

  int err;
  //SRC_STATE* src = src_new(SRC_SINC_BEST_QUALITY, 1, &err);
  SRC_STATE* src = src_new(SRC_SINC_FASTEST, 1, &err);
  assert(src);

  double ratio = out_size/(double)info.frames;

  float buf[BUF_SIZE];
  int size = out_size;
  float* out = new float[out_size];
  SRC_DATA data;
  data.data_in = buf;
  data.data_out = out;
  data.src_ratio = ratio;
  data.end_of_input = 0;
  int num = BUF_SIZE;
  while (num == BUF_SIZE) {
    num = data.input_frames = sf_readf_float(file, buf, BUF_SIZE);
    data.output_frames = size;

    env->process(buf, num);
    src_process(src, &data);
    assert(data.input_frames_used == num || size < BUF_SIZE);

    size-=data.output_frames_gen;
    data.data_out += data.output_frames_gen;
  }

  // AudioEnv* audio = malloc(sizeof(AudioEnv));
  // audio->data = out;
  // audio->size = out_size - size;

  sf_close(file);
  src_delete(src);
}
