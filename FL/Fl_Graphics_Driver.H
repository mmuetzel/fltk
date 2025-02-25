//
// Declaration of classes Fl_Graphics_Driver, Fl_Scalable_Graphics_Driver,
// and Fl_Font_Descriptor for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010-2021 by Bill Spitzak and others.
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

/** \file Fl_Graphics_Driver.H
 \brief Declaration of class Fl_Graphics_Driver.
*/

#ifndef FL_GRAPHICS_DRIVER_H
#define FL_GRAPHICS_DRIVER_H

#include <FL/Fl_Device.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Bitmap.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_RGB_Image.H>

class Fl_Graphics_Driver;
class Fl_Font_Descriptor;
class Fl_Image_Surface;
/** \brief Points to the driver that currently receives all graphics requests */
FL_EXPORT extern Fl_Graphics_Driver *fl_graphics_driver;

/**
 signature of image generation callback function.
 \param[in]  data  user data passed to function
 \param[in]  x,y,w position and width of scan line in image
 \param[out] buf   buffer for generated image data. You must copy \p w
 pixels from scanline \p y, starting at pixel \p x
 to this buffer.
 */
typedef void (*Fl_Draw_Image_Cb)(void* data,int x,int y,int w,uchar* buf);

struct Fl_Fontdesc;
typedef struct _PangoFontDescription PangoFontDescription;

#define FL_REGION_STACK_SIZE 10
#define FL_MATRIX_STACK_SIZE 32

/**
 An abstract class subclassed for each graphics driver FLTK uses.
 Typically, FLTK applications do not use directly objects from this class. Rather, they perform
 drawing operations (e.g., fl_rectf()) that operate on the current drawing surface (see Fl_Surface_Device).
 Drawing operations are functionally presented in \ref drawing and as function lists
 in the \ref fl_drawings and \ref fl_attributes modules.

 <tt>Fl_Surface_Device::surface()->driver()</tt>
 gives at any time the graphics driver used by all drawing operations.
 For compatibility with older FLTK versions, the \ref fl_graphics_driver global variable gives the same result.
 Its value changes when
 drawing operations are directed to another drawing surface by Fl_Surface_Device::push_current() /
 Fl_Surface_Device::pop_current() / Fl_Surface_Device::set_current().

 The Fl_Graphics_Driver class is essential for developers of the FLTK library.
 Each platform supported by FLTK requires to create a derived class of Fl_Graphics_Driver that
 implements all its virtual member functions according to the platform.
 */
class FL_EXPORT Fl_Graphics_Driver {
  friend class Fl_Surface_Device;
  friend class Fl_Display_Device;
  friend class Fl_Screen_Driver;
  friend class Fl_Window_Driver;
  friend class Fl_Pixmap;
  friend class Fl_Bitmap;
  friend class Fl_RGB_Image;
  friend class Fl_SVG_Image;
  friend void fl_draw_image(const uchar* buf, int X,int Y,int W,int H, int D, int L);
  friend void fl_draw_image_mono(const uchar* buf, int X,int Y,int W,int H, int D, int L);
  friend void fl_draw_image_mono(Fl_Draw_Image_Cb cb, void* data, int X,int Y,int W,int H, int D);
  friend void fl_draw_image(Fl_Draw_Image_Cb cb, void* data, int X,int Y,int W,int H, int D);
  friend void fl_copy_offscreen(int x, int y, int w, int h, Fl_Offscreen pixmap, int srcx, int srcy);
  friend int fl_convert_pixmap(const char*const* cdata, uchar* out, Fl_Color bg);
  friend FL_EXPORT void gl_start();
  friend FL_EXPORT void gl_finish();
  friend FL_EXPORT Fl_Bitmask fl_create_bitmask(int w, int h, const uchar *array);
  friend FL_EXPORT void fl_delete_bitmask(Fl_Bitmask);
  /* ============== Implementation note about image drawing =========================
   A graphics driver can implement up to 6 virtual member functions to draw images:
   virtual void draw_pixmap(Fl_Pixmap *pxm,int XP, int YP, int WP, int HP, int cx, int cy)
   virtual void draw_bitmap(Fl_Bitmap *bm,int XP, int YP, int WP, int HP, int cx, int cy)
   virtual void draw_rgb(Fl_RGB_Image *rgb,int XP, int YP, int WP, int HP, int cx, int cy)
and
   virtual void draw_fixed(Fl_Pixmap *pxm,int XP, int YP, int WP, int HP, int cx, int cy)
   virtual void draw_fixed(Fl_Bitmap *bm,int XP, int YP, int WP, int HP, int cx, int cy)
   virtual void draw_fixed(Fl_RGB_Image *rgb,int XP, int YP, int WP, int HP, int cx, int cy)
   - The 1st group of functions is used when the driver can directly map the image data,
   sized at data_w() x data_h(), to the image drawing area, sized at w()*scale x h()*scale
   where scale is the current GUI scale factor.
   - If the driver does not support such scale-and-draw operation for a given image type,
   it should implement the draw_fixed() function which is called by the library after the
   image has been internally resized to the drawing size and cached.
   - The platform-independent Fl_Graphics_Driver class implements the 1st group of functions.
   Each resizes the image, caches it, and calls the platform-specific implementation of
   draw_fixed(image-class *,....) with the cached image.
   - Consider an image object, say from class Fl_RGB_Image. Fl_RGB_Image::draw()
   calls the virtual member function draw_rgb(Fl_RGB_Image *,....). If Fl_XXX_Graphics_Driver
   re-implements this function, this code runs and is expected to draw the image
   adequately scaled to its drawing size. If Fl_XXX_Graphics_Driver does not re-implement
   this function, Fl_Graphics_Driver::draw_rgb(Fl_RGB_Image *,....) runs. It internally
   resizes the image, caches it, and calls Fl_XXX_Graphics_Driver::draw_fixed(Fl_RGB_Image *,....)
   that draws the image from its cached form which already has the adequate size.
   - Some drivers implement, for a given image class, only the function of the 1st group or
   only draw_fixed() as in these examples:
   - Fl_Quartz_Graphics_Driver implements only draw_rgb(Fl_RGB_Image *,....) because it
   can perform the scale-and-draw operation whatever the RGB image and the required scaling.
   - Fl_Xlib_Graphics_Driver implements only draw_fixed(Fl_Pixmap *,....). The library
   takes care of resizing and caching the Pixmap to the adequate drawing size.
   - Some drivers implement, for a given image class, the function of both groups, e.g. :
   Fl_GDI_Graphics_Driver implements both draw_rgb(Fl_RGB_Image *,....) and
   draw_fixed(Fl_RGB_Image *,....) because scale-and-draw may require function Alphablend()
   from MSIMG32.DLL. In the absence of that, the draw_rgb() implementation calls
   Fl_Graphics_Driver::draw_rgb() which runs Fl_GDI_Graphics_Driver::draw_fixed(Fl_RGB_Image*,...).

   Graphics drivers also implement cache(Fl_Pixmap*), cache(Fl_Bitmap*) and cache(Fl_RGB_Image*)
   to compute the cached form of all image types, and uncache(Fl_RGB_Image *,...),
   uncache_pixmap(fl_uintptr_t) and delete_bitmask(Fl_Bitmask) to destroy cached image forms.
   Graphics drivers that use the mask_ variable of class Fl_Pixmap to cache an Fl_Pixmap
   object also reimplement the uchar **Fl_Graphics_Driver::mask_bitmap() member function.
   */
private:
  virtual void draw_fixed(Fl_Pixmap *pxm,int XP, int YP, int WP, int HP, int cx, int cy);
  virtual void draw_fixed(Fl_Bitmap *bm,int XP, int YP, int WP, int HP, int cx, int cy);
  virtual void draw_fixed(Fl_RGB_Image *rgb,int XP, int YP, int WP, int HP, int cx, int cy);
  // the default implementation of make_unused_color_() is most probably enough
  virtual void make_unused_color_(unsigned char &r, unsigned char &g, unsigned char &b, int color_count, void **data);
  // some platforms may need to reimplement this
  virtual void set_current_();
  float scale_; // scale between FLTK and drawing coordinates: drawing = FLTK * scale_
public:
  /** Creates the graphics driver that is used for core operations. */
  static Fl_Graphics_Driver *newMainGraphicsDriver();
  /** A 2D coordinate transformation matrix */
  struct matrix {double a, b, c, d, x, y;};
  /** Features that a derived class may possess.  */
  typedef enum {
    NATIVE = 1, /**< native graphics driver for the platform */
    PRINTER = 2 /**< graphics driver for a printer drawing surface */
  } driver_feature;

protected:
  int fl_clip_state_number; ///< For internal use by FLTK
  static const matrix m0; ///< For internal use by FLTK
  Fl_Font font_; ///< current font
  Fl_Fontsize size_; ///< current font size
  Fl_Color color_; ///< current color
  int sptr;///< For internal use by FLTK
  static const int matrix_stack_size = FL_MATRIX_STACK_SIZE; ///< For internal use by FLTK
  matrix stack[FL_MATRIX_STACK_SIZE]; ///< For internal use by FLTK
  matrix m; ///< current transformation matrix
  int n; ///< For internal use by FLTK
  int gap_; ///< For internal use by FLTK
  int what; ///< For internal use by FLTK
  int rstackptr; ///< For internal use by FLTK
  static const int region_stack_max = FL_REGION_STACK_SIZE - 1; ///< For internal use by FLTK
  Fl_Region rstack[FL_REGION_STACK_SIZE]; ///< For internal use by FLTK
  Fl_Font_Descriptor *font_descriptor_; ///< For internal use by FLTK
  int p_size;
  typedef struct { float x; float y; } XPOINT;
  XPOINT *p;
#ifndef FL_DOXYGEN
  enum {LINE, LOOP, POLYGON, POINT_};
  inline int vertex_no() { return n; }
  inline int vertex_kind() {return what;}
#endif
  matrix *fl_matrix; /**< Points to the current coordinate transformation matrix */
  virtual void global_gc();
  virtual void cache(Fl_Pixmap *img);
  virtual void cache(Fl_Bitmap *img);
  virtual void cache(Fl_RGB_Image *img);
  virtual void uncache(Fl_RGB_Image *img, fl_uintptr_t &id_, fl_uintptr_t &mask_);
  // --- implementation is in src/drivers/xxx/Fl_xxx_Graphics_Driver_image.cxx
  virtual void draw_image(const uchar* buf, int X,int Y,int W,int H, int D=3, int L=0);
  virtual void draw_image_mono(const uchar* buf, int X,int Y,int W,int H, int D=1, int L=0);
  virtual void draw_image(Fl_Draw_Image_Cb cb, void* data, int X,int Y,int W,int H, int D=3);
  virtual void draw_image_mono(Fl_Draw_Image_Cb cb, void* data, int X,int Y,int W,int H, int D=1);
  virtual void draw_rgb(Fl_RGB_Image * rgb,int XP, int YP, int WP, int HP, int cx, int cy);
  virtual void draw_pixmap(Fl_Pixmap * pxm,int XP, int YP, int WP, int HP, int cx, int cy);
  virtual void draw_bitmap(Fl_Bitmap *bm, int XP, int YP, int WP, int HP, int cx, int cy);
  virtual void copy_offscreen(int x, int y, int w, int h, Fl_Offscreen pixmap, int srcx, int srcy);

  virtual Fl_Bitmask create_bitmask(int w, int h, const uchar *array);
  virtual void delete_bitmask(Fl_Bitmask bm);
  /** For internal library use only */
  static void change_image_size(Fl_Image *img, int W, int H) {
    img->w(W);
    img->h(H);
  }
  // Support function for image drawing
  virtual void uncache_pixmap(fl_uintptr_t p);
  // accessor functions to protected image members
  int start_image(Fl_Image *img, int XP, int YP, int WP, int HP, int &cx, int &cy,
                int &X, int &Y, int &W, int &H);
  /** Accessor to a private member variable of Fl_RGB_Image */
  static fl_uintptr_t* id(Fl_RGB_Image *rgb) {return &(rgb->id_);}
  /** Accessor to a private member variable of Fl_Pixmap */
  static fl_uintptr_t* id(Fl_Pixmap *pm) {return &(pm->id_);}
  /** Accessor to a private member variable of Fl_Bitmap */
  static fl_uintptr_t* id(Fl_Bitmap *bm) {return &(bm->id_);}
  /** Accessor to a private member variable of Fl_RGB_Image */
  static fl_uintptr_t* mask(Fl_RGB_Image *rgb) {return &(rgb->mask_);}
  /** Accessor to a private member variable of Fl_Pixmap */
  static fl_uintptr_t* mask(Fl_Pixmap *pm) {return &(pm->mask_);}
  /** Accessor to private member variables of Fl_Pixmap */
  static void cache_w_h(Fl_Pixmap *pm, int*& pwidth, int*& pheight) {
    pwidth = &(pm->cache_w_);
    pheight = &(pm->cache_h_);
  }
  /** Accessor to private member variables of Fl_Bitmap */
  static void cache_w_h(Fl_Bitmap *bm, int*& pwidth, int*& pheight) {
    pwidth = &(bm->cache_w_);
    pheight = &(bm->cache_h_);
  }
  /** Accessor to private member variables of Fl_RGB_Image */
  static void cache_w_h(Fl_RGB_Image *rgb, int*& pwidth, int*& pheight) {
    pwidth = &(rgb->cache_w_);
    pheight = &(rgb->cache_h_);
  }
  static Fl_Offscreen get_offscreen_and_delete_image_surface(Fl_Image_Surface*);
  /** For internal library use only */
  static void draw_empty(Fl_Image* img, int X, int Y) {img->draw_empty(X, Y);}

  Fl_Graphics_Driver();
  virtual void cache_size(Fl_Image *img, int &width, int &height);
  void cache_size_finalize(Fl_Image *img, int &width, int &height);
  static unsigned need_pixmap_bg_color;
public:
  virtual ~Fl_Graphics_Driver() {} ///< Destructor
  static Fl_Graphics_Driver &default_driver();
  /** Current scale factor between FLTK and drawing units: drawing = FLTK * scale() */
  float scale() { return scale_; }
  /** Sets the current value of the scaling factor */
  virtual void scale(float f);
  /** Return whether the graphics driver can do alpha blending */
  virtual char can_do_alpha_blending();
  // --- implementation is in src/fl_rect.cxx which includes src/drivers/xxx/Fl_xxx_Graphics_Driver_rect.cxx
  /** see fl_point() */
  virtual void point(int x, int y);
  virtual void rect(int x, int y, int w, int h);
  virtual void focus_rect(int x, int y, int w, int h);
  virtual void rectf(int x, int y, int w, int h);
  // the default implementation is most likely enough
  virtual void colored_rectf(int x, int y, int w, int h, uchar r, uchar g, uchar b);
  virtual void line(int x, int y, int x1, int y1);
  /** see fl_line(int, int, int, int, int, int) */
  virtual void line(int x, int y, int x1, int y1, int x2, int y2);
  /** see fl_xyline(int, int, int) */
  virtual void xyline(int x, int y, int x1);
  /** see fl_xyline(int, int, int, int) */
  virtual void xyline(int x, int y, int x1, int y2);
  /** see fl_xyline(int, int, int, int, int) */
  virtual void xyline(int x, int y, int x1, int y2, int x3);
  /** see fl_yxline(int, int, int) */
  virtual void yxline(int x, int y, int y1);
  /** see fl_yxline(int, int, int, int) */
  virtual void yxline(int x, int y, int y1, int x2);
  /** see fl_yxline(int, int, int, int, int) */
  virtual void yxline(int x, int y, int y1, int x2, int y3);
  /** see fl_loop(int, int, int, int, int, int) */
  virtual void loop(int x0, int y0, int x1, int y1, int x2, int y2);
  /** see fl_loop(int, int, int, int, int, int, int, int) */
  virtual void loop(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);
  virtual void polygon(int x0, int y0, int x1, int y1, int x2, int y2);
  /** see fl_polygon(int, int, int, int, int, int, int, int) */
  virtual void polygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);
  // --- clipping
  virtual void push_clip(int x, int y, int w, int h);
  virtual int clip_box(int x, int y, int w, int h, int &X, int &Y, int &W, int &H);
  virtual int not_clipped(int x, int y, int w, int h);
  /** see fl_push_no_clip() */
  virtual void push_no_clip();                  // has default implementation
  /** see fl_pop_clip() */
  virtual void pop_clip();                      // has default implementation
  virtual Fl_Region clip_region();              // has default implementation
  virtual void clip_region(Fl_Region r);        // has default implementation
  virtual void restore_clip();
  // --- implementation is in src/fl_vertex.cxx which includes src/drivers/xxx/Fl_xxx_Graphics_Driver_vertex.cxx
  virtual void push_matrix();
  virtual void pop_matrix();
  virtual void mult_matrix(double a, double b, double c, double d, double x, double y);
  virtual void rotate(double d);
  virtual void translate(double x,double y);
  virtual void begin_points();
  virtual void begin_line();
  virtual void begin_loop();
  virtual void begin_polygon();
  virtual void begin_complex_polygon();
  virtual double transform_x(double x, double y);
  virtual double transform_y(double x, double y);
  virtual double transform_dx(double x, double y);
  virtual double transform_dy(double x, double y);
  virtual void transformed_vertex(double xf, double yf);
  virtual void transformed_vertex0(float x, float y);
  virtual void vertex(double x, double y);
  virtual void end_points();
  virtual void end_line();
  virtual void end_loop();
  virtual void fixloop();
  virtual void end_polygon();
  virtual void end_complex_polygon();
  virtual void gap();
  virtual void circle(double x, double y, double r);
  // --- implementation is in src/fl_arc.cxx which includes src/drivers/xxx/Fl_xxx_Graphics_Driver_arc.cxx if needed
  virtual void arc(double x, double y, double r, double start, double end);
  // --- implementation is in src/fl_arci.cxx which includes src/drivers/xxx/Fl_xxx_Graphics_Driver_arci.cxx
  virtual void arc(int x, int y, int w, int h, double a1, double a2);
  virtual void pie(int x, int y, int w, int h, double a1, double a2);
  // --- implementation is in src/fl_curve.cxx which includes src/drivers/xxx/Fl_xxx_Graphics_Driver_curve.cxx if needed
  virtual void curve(double X0, double Y0, double X1, double Y1, double X2, double Y2, double X3, double Y3);
  // --- implementation is in src/fl_line_style.cxx which includes src/cfg_gfx/xxx_line_style.cxx
  virtual void line_style(int style, int width=0, char* dashes=0);
  // --- implementation is in src/fl_color.cxx which includes src/cfg_gfx/xxx_color.cxx
  virtual void color(Fl_Color c);
  virtual void set_color(Fl_Color i, unsigned int c);
  virtual void free_color(Fl_Color i, int overlay);
  virtual Fl_Color color();
  virtual void color(uchar r, uchar g, uchar b);
  virtual void draw(const char *str, int nChars, int x, int y);
  virtual void draw(const char *str, int nChars, float x, float y);
  virtual void draw(int angle, const char *str, int nChars, int x, int y);
  virtual void rtl_draw(const char *str, int nChars, int x, int y);
  virtual int has_feature(driver_feature feature);
  virtual void font(Fl_Font face, Fl_Fontsize fsize);
  virtual Fl_Font font();
  virtual Fl_Fontsize size();
  virtual double width(const char *str, int nChars);
  virtual double width(unsigned int c);
  virtual void text_extents(const char*, int n, int& dx, int& dy, int& w, int& h);
  virtual int height();
  virtual int descent();
  /** Return the current Fl_Font_Descriptor */
  inline Fl_Font_Descriptor *font_descriptor() { return font_descriptor_;}
  virtual void font_descriptor(Fl_Font_Descriptor *d);
  virtual void gc(void*);
  virtual void *gc(void);
  virtual uchar **mask_bitmap();
  // default implementation may be enough
  virtual float scale_font_for_PostScript(Fl_Font_Descriptor *desc, int s);
  // default implementation may be enough
  virtual float scale_bitmap_for_PostScript();
  virtual void set_spot(int font, int size, int X, int Y, int W, int H, Fl_Window *win);
  virtual void reset_spot();
  // each platform implements these 3 functions its own way
  virtual void add_rectangle_to_region(Fl_Region r, int x, int y, int w, int h);
  virtual Fl_Region XRectangleRegion(int x, int y, int w, int h);
  virtual void XDestroyRegion(Fl_Region r);
  virtual const char* get_font_name(Fl_Font fnum, int* ap);
  virtual int get_font_sizes(Fl_Font fnum, int*& sizep);
  virtual Fl_Font set_fonts(const char *name);
  virtual Fl_Fontdesc* calc_fl_fonts(void);
  virtual unsigned font_desc_size();
  virtual const char *font_name(int num);
  virtual void font_name(int num, const char *name);
  // Defaut implementation may be enough
  virtual void overlay_rect(int x, int y, int w , int h);
  virtual float override_scale();
  virtual void restore_scale(float);
  virtual PangoFontDescription* pango_font_description(Fl_Font) { return NULL; }
  virtual void antialias(int state);
  virtual int antialias();
};

#ifndef FL_DOXYGEN

/* This class is not part of FLTK's public API.
 Platforms usually define a derived class called Fl_XXX_Font_Descriptor
 containing extra platform-specific data/functions.
 This is a class for an actual system font, with junk to
 help choose it and info on character sizes.  Each Fl_Fontdesc has a
 linked list of these.  These are created the first time each system
 font/size combination is used.
 */
class Fl_Font_Descriptor {
public:
  /** linked list for this Fl_Fontdesc */
  Fl_Font_Descriptor *next;
  Fl_Fontsize size; /**< font size */
  Fl_Font_Descriptor(const char* fontname, Fl_Fontsize size);
  FL_EXPORT ~Fl_Font_Descriptor() {}
  short ascent, descent, q_width;
  unsigned int listbase;// base of display list, 0 = none
};

// This struct is not part of FLTK's public API.
struct Fl_Fontdesc {
  const char *name;
  char fontname[128];  // "Pretty" font name
  Fl_Font_Descriptor *first;  // linked list of sizes of this style
};

/* Abstract class Fl_Scalable_Graphics_Driver is platform-independent.
 It supports the scaling of all graphics coordinates by a
 float factor helpful to support HiDPI displays.
 This class does :
 - compute scaled coordinates
 - scale the cached offscreen of image objects
 - scale the pixel arrays used when performing direct image draws
 - call the member functions of a platform-specific,
 Fl_Scalable_Graphics_Driver-derived class that do the drawings with adequately
 scaled coordinates. The member functions are named with the _unscaled suffix.
 - scale and unscale the clipping region.

 This class is presently used by the X11 and Windows platforms to support HiDPI displays.
 In the future, it may also be used by other platforms.
 */
class FL_EXPORT Fl_Scalable_Graphics_Driver : public Fl_Graphics_Driver {
public:
  Fl_Scalable_Graphics_Driver();
  // This function aims to compute accurately int(x * s) in
  // presence of rounding errors existing with floating point numbers
  // and that sometimes differ between 32 and 64 bits.
  static inline int floor(int x, float s) { return int(x * s + 0.001f); }
  inline int floor(int x) { return Fl_Scalable_Graphics_Driver::floor(x, scale()); }
protected:
  int line_width_;
  virtual Fl_Region scale_clip(float f);
  void unscale_clip(Fl_Region r);
  virtual void point(int x, int y);
  virtual void point_unscaled(float x, float y);
  virtual void rect(int x, int y, int w, int h);
  virtual void rectf(int x, int y, int w, int h);
  virtual void rectf_unscaled(int x, int y, int w, int h);
  virtual void line(int x, int y, int x1, int y1);
  virtual void line_unscaled(int x, int y, int x1, int y1);
  virtual void line(int x, int y, int x1, int y1, int x2, int y2);
  virtual void line_unscaled(int x, int y, int x1, int y1, int x2, int y2);
  virtual void xyline(int x, int y, int x1);
  virtual void xyline_unscaled(int x, int y, int x1);
  virtual void yxline(int x, int y, int y1);
  virtual void yxline_unscaled(int x, int y, int y1);
  virtual void loop(int x0, int y0, int x1, int y1, int x2, int y2);
  virtual void loop_unscaled(int x0, int y0, int x1, int y1, int x2, int y2);
  virtual void loop(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);
  virtual void loop_unscaled(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);
  virtual void polygon(int x0, int y0, int x1, int y1, int x2, int y2);
  virtual void polygon_unscaled(int x0, int y0, int x1, int y1, int x2, int y2);
  virtual void polygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);
  virtual void polygon_unscaled(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);
  virtual void circle(double x, double y, double r);
  virtual void ellipse_unscaled(double xt, double yt, double rx, double ry);
  virtual void font(Fl_Font face, Fl_Fontsize size);
  virtual Fl_Font font();
  virtual void font_unscaled(Fl_Font face, Fl_Fontsize size);
  virtual double width(const char *str, int n);
  virtual double width(unsigned int c);
  virtual double width_unscaled(const char *str, int n);
  virtual double width_unscaled(unsigned int c);
  virtual Fl_Fontsize size();
  virtual Fl_Fontsize size_unscaled();
  virtual void text_extents(const char *str, int n, int &dx, int &dy, int &w, int &h);
  virtual void text_extents_unscaled(const char *str, int n, int &dx, int &dy, int &w, int &h);
  virtual int height();
  virtual int descent();
  virtual int height_unscaled();
  virtual int descent_unscaled();
  virtual void draw(const char *str, int n, int x, int y);
  virtual void draw_unscaled(const char *str, int n, int x, int y);
  virtual void draw(int angle, const char *str, int n, int x, int y);
  virtual void draw_unscaled(int angle, const char *str, int n, int x, int y);
  virtual void draw(const char *str, int nChars, float x, float y);
  virtual void rtl_draw(const char* str, int n, int x, int y);
  virtual void rtl_draw_unscaled(const char* str, int n, int x, int y);
  virtual void arc(double x, double y, double r, double start, double end);
  virtual void arc(int x, int y, int w, int h, double a1, double a2);
  virtual void arc_unscaled(int x, int y, int w, int h, double a1, double a2);
  virtual void pie(int x, int y, int w, int h, double a1, double a2);
  virtual void pie_unscaled(int x, int y, int w, int h, double a1, double a2);
  virtual void line_style(int style, int width=0, char* dashes=0);
  virtual void line_style_unscaled(int style, int width, char* dashes);
  void draw_image_rescale(void *buf, Fl_Draw_Image_Cb cb, int X, int Y, int W, int H, int D, int L, bool mono);
  virtual void draw_image_unscaled(const uchar* buf, int X,int Y,int W,int H, int D=3, int L=0);
  virtual void draw_image_unscaled(Fl_Draw_Image_Cb cb, void* data, int X,int Y,int W,int H, int D=3);
  void draw_image(const uchar* buf, int X,int Y,int W,int H, int D=3, int L=0);
  void draw_image(Fl_Draw_Image_Cb cb, void* data, int X,int Y,int W,int H, int D=3);
  virtual void draw_image_mono_unscaled(const uchar* buf, int x, int y, int w, int h, int d, int l);
  void draw_image_mono(const uchar* buf, int X,int Y,int W,int H, int D=1, int L=0);
  virtual void draw_image_mono_unscaled(Fl_Draw_Image_Cb cb, void* data, int X,int Y,int W,int H, int D=1);
  void draw_image_mono(Fl_Draw_Image_Cb cb, void* data, int X,int Y,int W,int H, int D=1);

  void transformed_vertex(double xf, double yf);
  void vertex(double x, double y);
  virtual float override_scale();
  virtual void restore_scale(float);
  virtual void *change_pen_width(int lwidth);
  virtual void reset_pen_width(void *data);
};
#endif // FL_DOXYGEN

#endif // FL_GRAPHICS_DRIVER_H

/**
 \}
 \endcond
 */
