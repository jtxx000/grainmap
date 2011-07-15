/* graingui.cpp
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

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <grainmap.h>
#include <grainaudio.h>
#include <memory>
#include <assert.h>
#include <thread>
#include <unistd.h>

using namespace std;
using namespace Gtk;
using namespace Cairo;

class grain_widget : public DrawingArea {
private:
  grainmap gm;
  grainaudio audio;

public:
  grain_widget(const string& path) : gm(path), audio(gm) {
    add_events(Gdk::BUTTON_PRESS_MASK           |
               Gdk::POINTER_MOTION_MASK         |
               Gdk::POINTER_MOTION_HINT_MASK);
  }

  void set_point(int x, int y) {
    int start, end;
    double s = get_scale();
    audio.set_point(x/s, y/s);
  }

  bool on_button_press_event(GdkEventButton* event) {
    // printf("down %d %d\n", event->x, event->y);
    set_point(event->x, event->y);
    return true;
  }

  double get_scale() {
    Glib::RefPtr<Gdk::Window> window = get_window();
    assert(window);
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    RefPtr<ImageSurface> img = gm.get_surface();
    return min(min(width/(double)img->get_width(), height/(double)img->get_height()), 1.0);
  }

  bool on_motion_notify_event(GdkEventMotion* event) {
    if (!(event->state & GDK_BUTTON1_MASK)) return true;

    // printf("%d %d\n", event->x, event->y);
    set_point(event->x, event->y);
    return true;
  }

  bool on_draw(const Cairo::RefPtr<Cairo::Context>& c) {
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    RefPtr<ImageSurface> img = gm.get_surface();
    double s = get_scale();
    if (s < 1.0)
      c->scale(s,s);
    c->set_source_rgb(0, 0, 0);
    c->paint();
    c->set_source(img, 0, 0);
    c->paint();

    return true;
  }

};

static bool msg_callback(Dialog* progress, ProgressBar* bar, atomic<bool>* grain_loaded) {
  if (!grain_loaded->load()) {
    bar->pulse();
    return true;
  }

  progress->hide();

  return false;
}

static void print_usage(char* name) {
  printf("usage: %s [file]\n", name);
  exit(1);
}

int main(int argc, char* argv[]) {
  Gtk::Main kit(argc, argv);
  Gtk::Window window;
  string path;
  char c;

  while ((c = getopt(argc, argv, "h")) != -1) {
    switch (c) {
    case 'h':
    default:
      print_usage(argv[0]);
    }
  }

  if (optind < argc-1)
    print_usage(argv[0]);

  if (optind < argc)
    path = argv[optind];
  else {
    FileChooserDialog dialog("Choose an audio sample to load", FILE_CHOOSER_ACTION_OPEN);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button("Select", RESPONSE_OK);
    if (dialog.run() != RESPONSE_OK)
      return 1;
    dialog.hide();
    path = dialog.get_filename();
  }

  unique_ptr<grain_widget> grain;
  atomic<bool> grain_loaded(false);
  thread t([&]() {
      grain = unique_ptr<grain_widget>(new grain_widget(path));
      grain_loaded.store(true);
    });
  t.detach();
  Dialog progress;
  ProgressBar bar;
  //progress.set_default_size(640,100);
  progress.set_resizable(false);
  progress.get_vbox()->pack_end(bar, PACK_SHRINK);
  progress.show_all();
  Glib::signal_timeout().connect(sigc::bind(sigc::ptr_fun(msg_callback), &progress, &bar, &grain_loaded), 100);
  progress.run();
  //printf("foo bar\n");
  // msg.hide();
  window.add(*grain);
  grain->show();
  Gtk::Main::run(window);
  return 0;
}
