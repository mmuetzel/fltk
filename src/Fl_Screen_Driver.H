//
// All screen related calls in a driver style class.
//
// Copyright 1998-2021 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//
// Please see the following page on how to report bugs and issues:
//
//     https://www.fltk.org/bugs.php
//

/**
 \cond DriverDev
 \addtogroup DriverDeveloper
 \{
 */

#ifndef FL_SCREEN_DRIVER_H
#define FL_SCREEN_DRIVER_H

#include <FL/fl_types.h>
#include <FL/Fl.H> // for Fl_Timeout_Handler
#include <FL/Fl_Text_Editor.H>


// TODO: add text composition?
// TODO: add Fl::display
// TODO: add copy/paste, drag/drop?
// TODO: get key/get mouse?
// TODO: system colors/colormaps
// TODO: system menu?
// TODO: native filechooser
// TODO: native message boxes
// TODO: read screen to image
// TODO: application shortcuts

class Fl_Window;
class Fl_RGB_Image;
class Fl_Group;
class Fl_Input;

/**
  A base class describing the interface between FLTK and screen-related operations.

  This class is only for internal use by the FLTK library.

  Each supported platform implements several of the virtual methods of this class.
*/
class FL_EXPORT Fl_Screen_Driver {

protected:
  Fl_Screen_Driver();
  virtual ~Fl_Screen_Driver();

  static const int MAX_SCREENS = 16;

  int num_screens;
  static float fl_intersection(int x1, int y1, int w1, int h1,
                               int x2, int y2, int w2, int h2);

public:
  static int keyboard_screen_scaling; // true means ctrl/+/-/0/ resize windows
  static char bg_set;
  static char bg2_set;
  static char fg_set;

  virtual float scale(int) { return 1; }
  virtual void scale(int /*n*/, float /*f*/) {}
  static Fl_Screen_Driver *newScreenDriver();
  // --- display management
  virtual void display(const char *disp);
  virtual int visual(int flags);
  // --- screen configuration
  virtual void init() {}
  virtual int x() { return 0; }
  virtual int y() { return 0; }
  virtual int w() { return 800; } // default, override in driver!
  virtual int h() { return 600; } // default, override in driver!
  virtual int screen_count();
  void screen_xywh(int &X, int &Y, int &W, int &H, int mx, int my);
  virtual void screen_xywh(int &X, int &Y, int &W, int &H, int /*n*/) {
    X = 0;
    Y = 0;
    W = 800;
    H = 600;
  }
  void screen_xywh(int &X, int &Y, int &W, int &H, int mx, int my, int mw, int mh);
  virtual int screen_num(int x, int y);
  virtual int screen_num(int x, int y, int w, int h);
  virtual void screen_dpi(float &h, float &v, int n = 0) { // override in driver!
    h = 72;
    v = 72;
    (void)n;
  }
  void screen_work_area(int &X, int &Y, int &W, int &H, int mx, int my);
  virtual void screen_work_area(int &X, int &Y, int &W, int &H, int n) {
    screen_xywh(X, Y, W, H, n);
  }
  // --- audible output
  virtual void beep(int) {}
  // --- global events
  virtual void flush() {}                                  // must override
  virtual double wait(double) { return 0.0; } // must override
  virtual int ready() { return 0; }                        // must override
  virtual void grab(Fl_Window *) {}
  // --- global colors
  /* the default implementation of parse_color() may be enough */
  virtual int parse_color(const char *p, uchar &r, uchar &g, uchar &b);
  virtual void get_system_colors() {}
  /* the default implementation of get_system_scheme() may be enough */
  virtual const char *get_system_scheme();
  // --- global timers
  virtual void add_timeout(double, Fl_Timeout_Handler, void *) {}
  virtual void repeat_timeout(double, Fl_Timeout_Handler, void *) {}
  virtual int has_timeout(Fl_Timeout_Handler, void *) { return 0; }
  virtual void remove_timeout(Fl_Timeout_Handler, void *) {}

  static int secret_input_character;
  /* Implement to indicate whether complex text input may involve marked text.
    When it does, has_marked_text returns non zero and reset_marked_text() and
    insertion_point_location() must also be implemented.
  */
  virtual int has_marked_text() const { return 0; }
  virtual void reset_marked_text() {}
  virtual void insertion_point_location(int /*x*/, int /*y*/, int /*height*/) {}
  // implement so text-editing widgets support dead keys
  virtual int compose(int &del) {
    del = 0;
    return 0;
  }
  // default implementation may be enough
  virtual void compose_reset();
  // implement to support drag-n-drop. use_selection = 1 means the GUI is welcome to display
  // the selected text during the D&D operation
  virtual int dnd(int use_selection = 0) { (void)use_selection; return 0; }
  // null means no platform-specific key bindings for Fl_Text_Editor
  Fl_Text_Editor::Key_Binding *text_editor_extra_key_bindings;
  // default implementation may be enough
  virtual int text_display_can_leak() const { return 0; }

  // if no keyboard is connected on a touch or pen device, the system on-screen keyboard is
  // requested
  virtual void request_keyboard() {}
  // we no longer need the on-screen keyboard; it's up to the system to hide it
  virtual void release_keyboard() {}

  /* Member function read_win_rectangle() supports public functions
    fl_read_image() and fl_capture_window() which capture pixel data from
    a window (or also from an offscreen buffer with fl_read_image).

    If 'may_capture_subwins' is true, an implementation may or may not capture
    also the content of subwindows embedded in 'win'. If subwindows were captured,
    *'did_capture_subwins' is returned set to true. If read_win_rectangle()
    is called with 'may_capture_subwins' set to true, 'did_capture_subwins' should
    be set before the call to the address of a boolean set to false.
    The implementation of this virtual function for the macOS platform has the
    capability of capturing subwindows when asked for.

    A platform may also use its read_win_rectangle() implementation to capture
    window decorations (e.g., title bar). In that case, it is called by
    Fl_XXX_Window_Driver::capture_titlebar_and_borders().

    win is the window to capture from, or NULL to capture from the current offscreen
  */
  virtual Fl_RGB_Image *read_win_rectangle(int /*X*/, int /*Y*/, int /*w*/, int /*h*/, Fl_Window *,
                                           bool may_capture_subwins = false,
                                           bool *did_capture_subwins = NULL) {
    (void)may_capture_subwins;
    (void)did_capture_subwins;
    return NULL;
  }
  static void write_image_inside(Fl_RGB_Image *to, Fl_RGB_Image *from, int to_x, int to_y);
  static Fl_RGB_Image *traverse_to_gl_subwindows(Fl_Group *g, int x, int y, int w, int h,
                                                 Fl_RGB_Image *full_img);
  // optional platform-specific key handling for Fl_Input widget
  // the default implementation may be enough
  virtual int input_widget_handle_key(int key, unsigned mods, unsigned shift, Fl_Input *input);
  // implement to support Fl::get_mouse()
  virtual int get_mouse(int &/*x*/, int &/*y*/) { return 0; }
  // optional methods to enable/disable input methods for complex scripts
  virtual void enable_im() {}
  virtual void disable_im() {}
  // calls open_display_platform() and then does platform-independent work
  void open_display();
  // implement to open access to the display
  virtual void open_display_platform() {}
  // optional method to close display access
  virtual void close_display() {}
  // compute dimensions of an Fl_Offscreen
  virtual void offscreen_size(Fl_Offscreen, int &/*width*/, int &/*height*/) {}

  void rescale_all_windows_from_screen(int screen, float f);
  static void transient_scale_display(float f, int nscreen);
  static int scale_handler(int event);
  virtual void desktop_scale_factor() {}
  void use_startup_scale_factor();
  enum APP_SCALING_CAPABILITY {
    NO_APP_SCALING = 0,     ///< The platform does not support rescaling.
    SYSTEMWIDE_APP_SCALING, ///< The platform supports rescaling with the same factor for all screens.
    PER_SCREEN_APP_SCALING  ///< The platform supports rescaling with one factor for each screen.
  };
  /** Returns the platform's support for rescaling the application with ctrl/+/-/0 keys.
   */
  virtual APP_SCALING_CAPABILITY rescalable() { return NO_APP_SCALING; }
  // supports Fl_Window::default_icons()
  virtual void default_icons(const Fl_RGB_Image *icons[], int count);
};

#endif // !FL_SCREEN_DRIVER_H

/**
 \}
 \endcond
 */
