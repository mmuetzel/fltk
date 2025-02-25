//
// Widget type header file for the Fast Light Tool Kit (FLTK).
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

#ifndef _FLUID_FL_GROUP_TYPE_H
#define _FLUID_FL_GROUP_TYPE_H

#include "Fl_Widget_Type.h"

#include <FL/Fl_Tabs.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Wizard.H>

void group_cb(Fl_Widget *, void *);
void ungroup_cb(Fl_Widget *, void *);

class igroup : public Fl_Group {
public:
  void resize(int,int,int,int);
  void full_resize(int X, int Y, int W, int H) { Fl_Group::resize(X, Y, W, H); }
  igroup(int X,int Y,int W,int H) : Fl_Group(X,Y,W,H) {Fl_Group::current(0);}
};

class itabs : public Fl_Tabs {
public:
  void resize(int,int,int,int);
  void full_resize(int X, int Y, int W, int H) { Fl_Group::resize(X, Y, W, H); }
  itabs(int X,int Y,int W,int H) : Fl_Tabs(X,Y,W,H) {}
};

class iwizard : public Fl_Wizard {
public:
  void resize(int,int,int,int);
  void full_resize(int X, int Y, int W, int H) { Fl_Group::resize(X, Y, W, H); }
  iwizard(int X,int Y,int W,int H) : Fl_Wizard(X,Y,W,H) {}
};

class Fl_Group_Type : public Fl_Widget_Type {
public:
  virtual const char *type_name() {return "Fl_Group";}
  virtual const char *alt_type_name() {return "fltk::Group";}
  Fl_Widget *widget(int X,int Y,int W,int H) {
    igroup *g = new igroup(X,Y,W,H); Fl_Group::current(0); return g;}
  Fl_Widget_Type *_make() {return new Fl_Group_Type();}
  Fl_Type *make();
  void write_code1();
  void write_code2();
  void add_child(Fl_Type*, Fl_Type*);
  void move_child(Fl_Type*, Fl_Type*);
  void remove_child(Fl_Type*);
  int is_parent() const {return 1;}
  int is_group() const {return 1;}
  int pixmapID() { return 6; }

  virtual Fl_Widget *enter_live_mode(int top=0);
  virtual void leave_live_mode();
  virtual void copy_properties();
};

extern const char pack_type_name[];
extern Fl_Menu_Item pack_type_menu[];

class Fl_Pack_Type : public Fl_Group_Type {
  Fl_Menu_Item *subtypes() {return pack_type_menu;}
public:
  virtual const char *type_name() {return pack_type_name;}
  virtual const char *alt_type_name() {return "fltk::PackedGroup";}
  Fl_Widget_Type *_make() {return new Fl_Pack_Type();}
  int pixmapID() { return 22; }
  void copy_properties();
};

extern const char table_type_name[];

class Fl_Table_Type : public Fl_Group_Type {
public:
  virtual const char *type_name() {return table_type_name;}
  virtual const char *alt_type_name() {return "fltk::TableGroup";}
  Fl_Widget_Type *_make() {return new Fl_Table_Type();}
  Fl_Widget *widget(int X,int Y,int W,int H);
  int pixmapID() { return 51; }
  virtual Fl_Widget *enter_live_mode(int top=0);
  void add_child(Fl_Type*, Fl_Type*);
  void move_child(Fl_Type*, Fl_Type*);
  void remove_child(Fl_Type*);
};

extern const char tabs_type_name[];

class Fl_Tabs_Type : public Fl_Group_Type {
public:
  virtual void ideal_spacing(int &x, int &y) {
     x = 10;
     fl_font(o->labelfont(), o->labelsize());
     y = fl_height() + o->labelsize() - 6;
  }
  virtual const char *type_name() {return tabs_type_name;}
  virtual const char *alt_type_name() {return "fltk::TabGroup";}
  Fl_Widget *widget(int X,int Y,int W,int H) {
    itabs *g = new itabs(X,Y,W,H); Fl_Group::current(0); return g;}
  Fl_Widget_Type *_make() {return new Fl_Tabs_Type();}
  Fl_Type* click_test(int,int);
  void add_child(Fl_Type*, Fl_Type*);
  void remove_child(Fl_Type*);
  int pixmapID() { return 13; }
  Fl_Widget *enter_live_mode(int top=0);
};

extern const char scroll_type_name[];
extern Fl_Menu_Item scroll_type_menu[];

class Fl_Scroll_Type : public Fl_Group_Type {
  Fl_Menu_Item *subtypes() {return scroll_type_menu;}
public:
  virtual const char *type_name() {return scroll_type_name;}
  virtual const char *alt_type_name() {return "fltk::ScrollGroup";}
  Fl_Widget_Type *_make() {return new Fl_Scroll_Type();}
  int pixmapID() { return 19; }
  Fl_Widget *enter_live_mode(int top=0);
  void copy_properties();
};

extern const char tile_type_name[];

class Fl_Tile_Type : public Fl_Group_Type {
public:
  virtual const char *type_name() {return tile_type_name;}
  virtual const char *alt_type_name() {return "fltk::TileGroup";}
  Fl_Widget_Type *_make() {return new Fl_Tile_Type();}
  int pixmapID() { return 20; }
  void copy_properties();
};

extern const char wizard_type_name[];

class Fl_Wizard_Type : public Fl_Group_Type {
public:
  virtual const char *type_name() {return wizard_type_name;}
  virtual const char *alt_type_name() {return "fltk::WizardGroup";}
  Fl_Widget *widget(int X,int Y,int W,int H) {
    iwizard *g = new iwizard(X,Y,W,H); Fl_Group::current(0); return g;}
  Fl_Widget_Type *_make() {return new Fl_Wizard_Type();}
  int pixmapID() { return 21; }
};

#endif // _FLUID_FL_GROUP_TYPE_H
