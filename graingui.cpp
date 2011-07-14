#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <grainmap.h>
#include <grainaudio.h>
#include <memory>
#include <assert.h>

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

  bool on_expose_event(GdkEventExpose* event) {
    Glib::RefPtr<Gdk::Window> window = get_window();
    if(window) {
      Gtk::Allocation allocation = get_allocation();
      const int width = allocation.get_width();
      const int height = allocation.get_height();

      RefPtr<Context> c = window->create_cairo_context();

      if(event) {
        c->rectangle(event->area.x, event->area.y, event->area.width, event->area.height);
        c->clip();
      }
      RefPtr<ImageSurface> img = gm.get_surface();
      double s = get_scale();
      if (s < 1.0)
        c->scale(s,s);
      c->set_source_rgb(0, 0, 0);
      c->paint();
      c->set_source(img, 0, 0);
      c->paint();
    }
    return true;
  }

};

int main(int argc, char* argv[]) {
  Gtk::Main kit(argc, argv);
  Gtk::Window window;
  FileChooserDialog dialog("Choose an audio sample to load", FILE_CHOOSER_ACTION_OPEN);
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button("Select", RESPONSE_OK);
  if (dialog.run() != RESPONSE_OK)
    return 1;
  dialog.hide();
  // MessageDialog msg("loading... please wait", false, MESSAGE_INFO, BUTTONS_NONE);
  // msg.run();
  grain_widget grain(dialog.get_filename());
  // msg.hide();
  window.add(grain);
  grain.show();
  Gtk::Main::run(window);
  return 0;
}
