#ifndef GRAINMAP_H
#define GRAINMAP_H

#include <cairomm/cairomm.h>
#include <memory>
#include <map>

struct audio_data {
  float** data;
  int size;
  int channels;

  audio_data(int size, int channels);
  ~audio_data();
};

struct cairo_image {
  int width, height, stride;
  unsigned char* data;

  cairo_image(int width, int height);
  ~cairo_image();

  void set(int x, int y, unsigned char r, unsigned char g, unsigned char b);
  void mark(int x, int y);
  Cairo::RefPtr<Cairo::ImageSurface> create_surface();
};

class grainmap {
  static const int nsize = 10;

  std::map<int,int> region_starts; // (index -> sample) TODO merge with region_map
  std::unique_ptr<audio_data> adata;
  std::unique_ptr<cairo_image> cimg;
  Cairo::RefPtr<Cairo::ImageSurface> img;

public:
  grainmap();
  float** get_audio();
  int channel_count();
  void lookup(int x, int y, int& start, int& stop, int& starti, int& endi);
  Cairo::RefPtr<Cairo::ImageSurface> get_surface();
};

#endif //GRAINMAP_H
