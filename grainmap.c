#if INTERFACE
#include <stdio.h>
#include <math.h>
#include <samplerate.h>
#include <sndfile.h>
#include <assert.h>
#include <cairo.h>
#include <stdlib.h>

#include "hilbert.h"

typedef struct {
  float level;
  float decay;
} EnvFol;
#endif

EnvFol* create_env_fol(float decay) {
  EnvFol* env = malloc(sizeof(EnvFol));
  env->level = 0;
  env->decay = decay;
  return env;
}

void destroy_env_fol(EnvFol* env) {
  free(env);
}

static void env_process(EnvFol* f, float* data, int num) {
  while (--num>=0) {
    float in = fabsf(*data);
    *data++ = f->level = (in >= f->level ? in : (in + f->decay*(f->level - in)));
  }
}

typedef struct {
  bitmask_t start, end;
  int nsize;
} Region;

static inline int contains(Region r, bitmask_t coords[2]) {
  bitmask_t max = 1 << r.nsize;
  if (coords[0] < 0 || coords[1] < 0 || coords[0] >= max || coords[1] >= max)
    return 0;

  bitmask_t index = hilbert_c2i(2, r.nsize, coords);
  return index >= r.start && index < r.end;
}

/* static inline Dir init_dir(Region r, bitmask_t coords[2], int* dx, int* dy) { */
/*   *dx = *dy = 0; */

/*   assert(contains(r, coords)); */

/*   // top */
/*   coords[1]--; */
/*   if (!contains(r, coords)) { */
/*     *dx = 1; */
/*     return; */
/*   } */

/*   // right */
/*   coords[1]++; */
/*   coords[0]++; */
/*   if (!contains(r, coords)) { */
/*     *dy = 1; */
/*     return; */
/*   } */

/*   // bottom */
/*   coords[0]--; */
/*   coords[1]++; */
/*   if (!contains(r, coords)) { */
/*     *dx = -1; */
/*     return; */
/*   } */

/*   coords[1]--; */
/*   coords[0]--; */
/*   // left */
/*   if (!contains(r, coords)) { */
/*     *dy = -1; */
/*     return; */
/*   } */

/* } */

static inline void traverse_region(Region r, bitmask_t coords[2]) {
  bitmask_t orig_coords[2], foreign_coords[2];
  orig_coords[0] = coords[0];
  orig_coords[1] = coords[1];

  int nx, ny;
  if (r.start) {
    hilbert_i2c(2, bits, r.start-1, foreign_coords);
    nx = foreign_coords[0] - coords[0];
    ny = foreign_coords[1] - coords[1];
  } else {
    nx = 0;
    ny = -1;
  }

  do {
    foreign_coords[0] = coords[0] + nx;
    foreign_coords[1] = coords[1] + ny;
    int index = find_index(2, r.nsize, foreign_coords);
    if (!region_contains(foreign_region, index)) {
      if (region_contains(r, index)) {
        coords[0] = foreign_coords[0];
        coords[1] = foreign_coords[1];
        // rotate normal counterclockwise
        int tnx = nx;
        nx = ny;
        ny = -tnx;
      } else {
        // new region
        foreign_region = find_vertex(index);
        add_edge(foreign_region);
      }
    }

    // move clockwise-normal
    coords[0] -= ny;
    coords[1] += nx;
    if (!contains(r, coords)) {
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

static inline void index_map_set(int* map, int sizen, int pos, int index) {
  bitmask_t coords[2];
  hilbert_i2c(2, bits, i, coords);
  map[(coords[1] << nsize) + coords[0]] = index;
}

static void index_map_traverse(int* map, int sizen) {
}

AudioEnv* create_grain_map(char* path, EnvFol* env, int nsize) {
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
  float* out = malloc(sizeof(float)*out_size);
  SRC_DATA data;
  data.data_in = buf;
  data.data_out = out;
  data.src_ratio = ratio;
  data.end_of_input = 0;
  int num = BUF_SIZE;
  while (num == BUF_SIZE) {
    num = data.input_frames = sf_readf_float(file, buf, BUF_SIZE);
    data.output_frames = size;

    env_process(env, buf, num);
    src_process(src, &data);
    assert(data.input_frames_used == num || size < BUF_SIZE);

    size-=data.output_frames_gen;
    data.data_out += data.output_frames_gen;
  }

  AudioEnv* audio = malloc(sizeof(AudioEnv));
  audio->data = out;
  audio->size = out_size - size;

  sf_close(file);
  src_delete(src);

  return audio;
}

int foobar() {
  return 42;
}
