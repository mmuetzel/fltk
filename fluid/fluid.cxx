//
// FLUID main entry for the Fast Light Tool Kit (FLTK).
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

#include "fluid.h"

#include "Fl_Type.h"
#include "Fl_Function_Type.h"
#include "Fl_Group_Type.h"
#include "Fl_Window_Type.h"
#include "widget_browser.h"
#include "shell_command.h"
#include "factory.h"
#include "pixmaps.h"
#include "undo.h"
#include "file.h"
#include "code.h"

#include "alignment_panel.h"
#include "function_panel.h"
#include "template_panel.h"
#include "about_panel.h"

#include <FL/Fl.H>
#ifdef __APPLE__
#include <FL/platform.H> // for fl_open_callback
#endif
#include <FL/Fl_Help_Dialog.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Printer.H>
#include <FL/fl_string.h>
#include <locale.h>     // setlocale()..
#include "../src/flstring.h"

extern "C"
{
#if defined(HAVE_LIBPNG) && defined(HAVE_LIBZ)
#  include <zlib.h>
#  ifdef HAVE_PNG_H
#    include <png.h>
#  else
#    include <libpng/png.h>
#  endif // HAVE_PNG_H
#endif // HAVE_LIBPNG && HAVE_LIBZ
}

/// \defgroup globals Fluid Global Variables, Functions and Calbacks
/// \{

//
// Globals..
//

/// Fluid-wide help dialog.
static Fl_Help_Dialog *help_dialog = 0;

Fl_Menu_Bar *main_menubar = NULL;
Fl_Window *main_window;

/// Fluid application preferences, allways accessible, will be flushed when app closes.
Fl_Preferences  fluid_prefs(Fl_Preferences::USER, "fltk.org", "fluid");

/// Align widget position and size when designing, saved in app preferences and project file.
int gridx = 5;

/// Align widget position and size when designing, saved in app preferences and project file.
int gridy = 5;

/// Activate snapping to grid, saved in app preferences and project file.
int snap = 1;

/// Show guides in the design window when positioning widgets, saved in app preferences.
int show_guides = 1;

/// Show widget comments in the browser, saved in app preferences.
int show_comments = 1;

/// Use external editor for editing Fl_Code_Type, saved in app preferences.
int G_use_external_editor = 0;

/// Debugging help for external Fl_Code_Type editor.
int G_debug = 0;

/// Run this command to load an Fl_Code_Type into an external editor, save in app preferences.
char G_external_editor_command[512];


/// \todo Functionality unclear.
int force_parent = 0;

/// This is set to create different labels when creating new widgets.
/// \todo Details unclear.
int reading_file = 0;


// File history info...

/// Stores the absolute filename of the last 10 design files, saved in app preferences.
char    absolute_history[10][FL_PATH_MAX];

/// This list of filenames is computed from \c absolute_history and displayed in the main menu.
char    relative_history[10][FL_PATH_MAX];

/// Menuitem to save a .fl design file, will be deactivated if the design is unchanged.
Fl_Menu_Item *save_item = NULL;

/// First Menuitem that shows the .fl design file hisory.
Fl_Menu_Item *history_item = NULL;

/// Menuitem to show or hide the widget bin, label will change if bin is visible.
Fl_Menu_Item *widgetbin_item = NULL;

/// Menuitem to show or hide the source view, label will change if view is visible.
Fl_Menu_Item *sourceview_item = NULL;

/// Menuitem to show or hide the editing overlay, label will change if overlay visibility changes.
Fl_Menu_Item *overlay_item = NULL;

////////////////////////////////////////////////////////////////

/// Filename of the current .fl design file
static const char *filename = NULL;

/// Set if the current design has been modified compared to the associated .fl design file.
int modflag = 0;

/// Set if the code files are older than the current design.
int modflag_c = 0;

/// Application work directory, stored here when temporarily changing to the source code directory.
/// \see goto_source_dir()
static char* pwd = NULL;

/// Set, if the current working directory is in the source code folder vs. the app working space.
/// \see goto_source_dir()
static char in_source_dir = 0;

/// Set, if Fluid was started with the command line argument -u
int update_file = 0;            // fluid -u

/// Set, if Fluid was started with the command line argument -c
int compile_file = 0;           // fluid -c

/// Set, if Fluid was started with the command line argument -cs
int compile_strings = 0;        // fluic -cs

/// Set, if Fluid runs in batch mode, and no user interface is activated.
int batch_mode = 0;             // if set (-c, -u) don't open display

/// If set, commandline overrides header file name in .fl file.
int header_file_set = 0;

/// If set, commandline overrides source code file name in .fl file.
int code_file_set = 0;

/// Hold the default extension for header files, or the entire filename if set via command line.
const char* header_file_name = ".h";

/// Hold the default extension for source code  files, or the entire filename if set via command line.
const char* code_file_name = ".cxx";


/// Saved in the .fl design file.
/// \todo document me
int i18n_type = 0;

/// Saved in the .fl design file.
/// \todo document me
const char* i18n_include = "";

/// Saved in the .fl design file.
/// \todo document me
const char* i18n_function = "";

/// Saved in the .fl design file.
/// \todo document me
const char* i18n_file = "";

/// Saved in the .fl design file.
/// \todo document me
const char* i18n_set = "";

/// \todo document me
char i18n_program[FL_PATH_MAX] = "";

/// \todo document me
int pasteoffset = 0;

/// \todo document me
static int ipasteoffset = 0;


// ---- Sourceview definition

void update_sourceview_position();
void update_sourceview_position_cb(Fl_Tabs*, void*);
void update_sourceview_cb(Fl_Button*, void*);
void update_sourceview_timer(void*);

// ----

/**
 Change the current working directory to the source code folder.
 Remember the the previous directory, so \c leave_source_dir() can return there.
 \see leave_source_dir(), pwd, in_source_dir
 */
void goto_source_dir() {
  if (in_source_dir) return;
  if (!filename || !*filename) return;
  const char *p = fl_filename_name(filename);
  if (p <= filename) return; // it is in the current directory
  char buffer[FL_PATH_MAX];
  strlcpy(buffer, filename, sizeof(buffer));
  int n = (int)(p-filename); if (n>1) n--; buffer[n] = 0;
  if (!pwd) {
    pwd = fl_getcwd(0, FL_PATH_MAX);
    if (!pwd) {fprintf(stderr, "getwd : %s\n",strerror(errno)); return;}
  }
  if (fl_chdir(buffer) < 0) {
    fprintf(stderr, "Can't chdir to %s : %s\n", buffer, strerror(errno));
    return;
  }
  in_source_dir = 1;
}

/**
 Change the current working directory to its previous directory.
 \see goto_source_dir(), pwd, in_source_dir
 */
void leave_source_dir() {
  if (!in_source_dir) return;
  if (fl_chdir(pwd) < 0) {
    fprintf(stderr, "Can't chdir to %s : %s\n", pwd, strerror(errno));
  }
  in_source_dir = 0;
}

/**
 Position the given window window based on entries in the app preferences.
 Customizable by user; feature can be switched off.
 The window is not shown or hidden by this function, but a value is returned
 to indicate the state to the caller.
 \param[in] w position this window
 \param[in] prefsName name of the preferences item that stores the window settings
 \param[in] Visible default value if window is hidden or shown
 \param[in] X, Y, W, H default size and position if nothing is specified in the preferences
 \return 1 if the caller should make the window visible, 0 if hidden.
 */
char position_window(Fl_Window *w, const char *prefsName, int Visible, int X, int Y, int W=0, int H=0 ) {
  Fl_Preferences pos(fluid_prefs, prefsName);
  if (prevpos_button->value()) {
    pos.get("x", X, X);
    pos.get("y", Y, Y);
    if ( W!=0 ) {
      pos.get("w", W, W);
      pos.get("h", H, H);
      w->resize( X, Y, W, H );
    }
    else
      w->position( X, Y );
  }
  pos.get("visible", Visible, Visible);
  return Visible;
}

/**
 Save the position and visibility state of a window to the app preferences.
 \param[in] w save this window data
 \param[in] prefsName name of the preferences item that stores the window settings
 */
void save_position(Fl_Window *w, const char *prefsName) {
  Fl_Preferences pos(fluid_prefs, prefsName);
  pos.set("x", w->x());
  pos.set("y", w->y());
  pos.set("w", w->w());
  pos.set("h", w->h());
  pos.set("visible", (int)(w->shown() && w->visible()));
}

/**
 Return the path and filename of a temporary file for cut or duplicated data.
 \param[in] which 0 gets the cut/copy/paste buffer, 1 gets the duplication buffer
 \return a pointer to a string in a static buffer
 */
static char* cutfname(int which = 0) {
  static char name[2][FL_PATH_MAX];
  static char beenhere = 0;

  if (!beenhere) {
    beenhere = 1;
    fluid_prefs.getUserdataPath(name[0], sizeof(name[0]));
    strlcat(name[0], "cut_buffer", sizeof(name[0]));
    fluid_prefs.getUserdataPath(name[1], sizeof(name[1]));
    strlcat(name[1], "dup_buffer", sizeof(name[1]));
  }

  return name[which];
}

/**
 Timer to watch for external editor modifications.

 If one or more external editors open, check if their files were modified.
 If so: reload to ram, update size/mtime records, and change fluid's
 'modified' state.
 */
static void external_editor_timer(void*) {
  int editors_open = ExternalCodeEditor::editors_open();
  if ( G_debug ) printf("--- TIMER --- External editors open=%d\n", editors_open);
  if ( editors_open > 0 ) {
    // Walk tree looking for files modified by external editors.
    int modified = 0;
    for (Fl_Type *p = Fl_Type::first; p; p = p->next) {
      if ( p->is_code() ) {
        Fl_Code_Type *code = (Fl_Code_Type*)p;
        // Code changed by external editor?
        if ( code->handle_editor_changes() ) {  // updates ram, file size/mtime
          modified++;
        }
        if ( code->is_editing() ) {             // editor open?
          code->reap_editor();                  // Try to reap; maybe it recently closed
        }
      }
    }
    if ( modified ) set_modflag(1);
  }
  // Repeat timeout if editors still open
  //    The ExternalCodeEditor class handles start/stopping timer, we just
  //    repeat_timeout() if it's already on. NOTE: above code may have reaped
  //    only open editor, which would disable further timeouts. So *recheck*
  //    if editors still open, to ensure we don't accidentally re-enable them.
  //
  if ( ExternalCodeEditor::editors_open() ) {
    Fl::repeat_timeout(2.0, external_editor_timer);
  }
}

/**
 Save the current design to the file given by \c filename.
 If automatic, this overwrites an existing file. If iinteractive, if will
 verify with the user.
 \param[in] v if v is not NULL, or no filename is set, open a filechooser.
 */
void save_cb(Fl_Widget *, void *v) {
  Fl_Native_File_Chooser fnfc;
  const char *c = filename;
  if (v || !c || !*c) {
    fnfc.title("Save To:");
    fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    fnfc.filter("FLUID Files\t*.f[ld]");
    if (fnfc.show() != 0) return;
    c = fnfc.filename();
    if (!fl_access(c, 0)) {
      const char *basename;
      if ((basename = strrchr(c, '/')) != NULL)
        basename ++;
#if defined(_WIN32)
      if ((basename = strrchr(c, '\\')) != NULL)
        basename ++;
#endif // _WIN32
      else
        basename = c;

      if (fl_choice("The file \"%s\" already exists.\n"
                    "Do you want to replace it?", "Cancel",
                    "Replace", NULL, basename) == 0) return;
    }

    if (v != (void *)2) set_filename(c);
  }
  if (!write_file(c)) {
    fl_alert("Error writing %s: %s", c, strerror(errno));
    return;
  }

  if (v != (void *)2) {
    set_modflag(0, 1);
    undo_save = undo_current;
  }
}

/**
 Save a design template.
 \todo We should document the concept of templates.
 */
void save_template_cb(Fl_Widget *, void *) {
  // Setup the template panel...
  if (!template_panel) make_template_panel();

  template_clear();
  template_browser->add("New Template");
  template_load();

  template_name->show();
  template_name->value("");

  template_instance->hide();

  template_delete->show();
  template_delete->deactivate();

  template_submit->label("Save");
  template_submit->deactivate();

  template_panel->label("Save Template");

  // Show the panel and wait for the user to do something...
  template_panel->show();
  while (template_panel->shown()) Fl::wait();

  // Get the template name, return if it is empty...
  const char *c = template_name->value();
  if (!c || !*c) return;

  // Convert template name to filename_with_underscores
  char safename[FL_PATH_MAX], *safeptr;
  strlcpy(safename, c, sizeof(safename));
  for (safeptr = safename; *safeptr; safeptr ++) {
    if (isspace(*safeptr)) *safeptr = '_';
  }

  // Find the templates directory...
  char filename[FL_PATH_MAX];
  fluid_prefs.getUserdataPath(filename, sizeof(filename));

  strlcat(filename, "templates", sizeof(filename));
  if (fl_access(filename, 0)) fl_mkdir(filename, 0777);

  strlcat(filename, "/", sizeof(filename));
  strlcat(filename, safename, sizeof(filename));

  char *ext = filename + strlen(filename);
  if (ext >= (filename + sizeof(filename) - 5)) {
    fl_alert("The template name \"%s\" is too long!", c);
    return;
  }

  // Save the .fl file...
  strcpy(ext, ".fl");

  if (!fl_access(filename, 0)) {
    if (fl_choice("The template \"%s\" already exists.\n"
                  "Do you want to replace it?", "Cancel",
                  "Replace", NULL, c) == 0) return;
  }

  if (!write_file(filename)) {
    fl_alert("Error writing %s: %s", filename, strerror(errno));
    return;
  }

#if defined(HAVE_LIBPNG) && defined(HAVE_LIBZ)
  // Get the screenshot, if any...
  Fl_Type *t;

  for (t = Fl_Type::first; t; t = t->next) {
    // Find the first window...
    if (t->is_window()) break;
  }

  if (!t) return;

  // Grab a screenshot...
  Fl_Window_Type *wt = (Fl_Window_Type *)t;
  uchar *pixels;
  int w, h;

  if ((pixels = wt->read_image(w, h)) == NULL) return;

  // Save to a PNG file...
  strcpy(ext, ".png");

  errno = 0;
  if (fl_write_png(filename, pixels, w, h, 3) != 0) {
    delete[] pixels;
    fl_alert("Error writing %s: %s", filename, strerror(errno));
    return;
  }

#  if 0 // The original PPM output code...
  strcpy(ext, ".ppm");
  fp = fl_fopen(filename, "wb");
  fprintf(fp, "P6\n%d %d 255\n", w, h);
  fwrite(pixels, w * h, 3, fp);
  fclose(fp);
#  endif // 0

  delete[] pixels;
#endif // HAVE_LIBPNG && HAVE_LIBZ
}

/**
 Reload the file set by \c filename, replacing the current design.
 If the design was modified, a dialog will ask for confirmation.
 */
void revert_cb(Fl_Widget *,void *) {
  if (modflag) {
    if (!fl_choice("This user interface has been changed. Really revert?",
                   "Cancel", "Revert", NULL)) return;
  }
  undo_suspend();
  if (!read_file(filename, 0)) {
    undo_resume();
    fl_message("Can't read %s: %s", filename, strerror(errno));
    return;
  }
  undo_resume();
  set_modflag(0, 0);
  undo_clear();
}

/**
 Exit Fluid; we hope you had a nice experience.
 If the design was modified, a dialog will ask for confirmation.
 */
void exit_cb(Fl_Widget *,void *) {

  // Stop any external editor update timers
  ExternalCodeEditor::stop_update_timer();

  if (modflag)
    switch (fl_choice("Do you want to save changes to this user\n"
                      "interface before exiting?", "Cancel",
                      "Save", "Don't Save"))
    {
      case 0 : /* Cancel */
          return;
      case 1 : /* Save */
          save_cb(NULL, NULL);
          if (modflag) return;  // Didn't save!
    }

  save_position(main_window,"main_window_pos");

  if (widgetbin_panel) {
    save_position(widgetbin_panel,"widgetbin_pos");
    delete widgetbin_panel;
  }
  if (sourceview_panel) {
    Fl_Preferences svp(fluid_prefs, "sourceview");
    svp.set("autorefresh", sv_autorefresh->value());
    svp.set("autoposition", sv_autoposition->value());
    svp.set("tab", sv_tab->find(sv_tab->value()));
    save_position(sourceview_panel,"sourceview_pos");
    delete sourceview_panel;
    sourceview_panel = 0;
  }
  if (about_panel)
    delete about_panel;
  if (help_dialog)
    delete help_dialog;

  undo_clear();

  // Destroy tree
  //    Doing so causes dtors to automatically close all external editors
  //    and cleans up editor tmp files. Then remove fluid tmpdir /last/.
  delete_all();
  ExternalCodeEditor::tmpdir_clear();

  exit(0);
}

#ifdef __APPLE__

/**
 Handle app launch with an associated filename (macOS only).
 Should there be a modified design already, Fluid asks for user confirmation.
 \param[in] c the filename of the new design
 */
void apple_open_cb(const char *c) {
  if (modflag) {
    switch (fl_choice("Do you want to save changes to this user\n"
                      "interface before opening another one?", "Don't Save",
                      "Save", "Cancel"))
    {
      case 0 : /* Cancel */
          return;
      case 1 : /* Save */
          save_cb(NULL, NULL);
          if (modflag) return;  // Didn't save!
    }
  }
  const char *oldfilename;
  oldfilename = filename;
  filename    = NULL;
  set_filename(c);
  undo_suspend();
  if (!read_file(c, 0)) {
    undo_resume();
    fl_message("Can't read %s: %s", c, strerror(errno));
    free((void *)filename);
    filename = oldfilename;
    if (main_window) main_window->label(filename);
    return;
  }

  // Loaded a file; free the old filename...
  set_modflag(0, 0);
  undo_resume();
  undo_clear();
  if (oldfilename) free((void *)oldfilename);
}
#endif // __APPLE__

/**
 Open a file chooser and load a new file.
 If the current design was modified, Fluid will ask for user confirmation.
 \param[in] v if v is set, Fluid will not ask for confirmation.
 */
void open_cb(Fl_Widget *, void *v) {
  if (!v && modflag) {
    switch (fl_choice("Do you want to save changes to this user\n"
                      "interface before opening another one?", "Cancel",
                      "Save", "Don't Save"))
    {
      case 0 : /* Cancel */
          return;
      case 1 : /* Save */
          save_cb(NULL, NULL);
          if (modflag) return;  // Didn't save!
    }
  }
  const char *c;
  const char *oldfilename;
  Fl_Native_File_Chooser fnfc;
  fnfc.title("Open:");
  fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
  fnfc.filter("FLUID Files\t*.f[ld]\n");
  if (fnfc.show() != 0) return;
  c = fnfc.filename();
  oldfilename = filename;
  filename    = NULL;
  set_filename(c);
  if (v != 0) undo_checkpoint();
  undo_suspend();
  if (!read_file(c, v!=0)) {
    undo_resume();
    fl_message("Can't read %s: %s", c, strerror(errno));
    free((void *)filename);
    filename = oldfilename;
    if (main_window) set_modflag(modflag);
    return;
  }
  undo_resume();
  if (v) {
    // Inserting a file; restore the original filename...
    free((void *)filename);
    filename = oldfilename;
    set_modflag(1);
  } else {
    // Loaded a file; free the old filename...
    set_modflag(0, 0);
    undo_clear();
    if (oldfilename) free((void *)oldfilename);
  }
}

/**
 Open a file from history.
 If the current design was modified, Fluid will ask for user confirmation.
 \param[in] v points to the absolute path and filename.
 */
void open_history_cb(Fl_Widget *, void *v) {
  if (modflag) {
    switch (fl_choice("Do you want to save changes to this user\n"
                      "interface before opening another one?", "Cancel",
                      "Save", "Don't Save"))
    {
      case 0 : /* Cancel */
          return;
      case 1 : /* Save */
          save_cb(NULL, NULL);
          if (modflag) return;  // Didn't save!
    }
  }
  const char *oldfilename = filename;
  filename = NULL;
  set_filename((char *)v);
  undo_suspend();
  if (!read_file(filename, 0)) {
    undo_resume();
    undo_clear();
    fl_message("Can't read %s: %s", filename, strerror(errno));
    free((void *)filename);
    filename = oldfilename;
    if (main_window) main_window->label(filename);
    return;
  }
  set_modflag(0, 0);
  undo_resume();
  undo_clear();
  if (oldfilename) {
    free((void *)oldfilename);
    oldfilename = 0L;
  }
}

/**
 Close the current design and create a new, empty one.
 If the current design was modified, Fluid will ask for user confirmation.
 \param[in] v if v is set, don't ask for confirmation
 */
void new_cb(Fl_Widget *, void *v) {
  // Check if the current file has been modified...
  if (!v && modflag) {
    // Yes, ask the user what to do...
    switch (fl_choice("Do you want to save changes to this user\n"
                      "interface before creating a new one?", "Cancel",
                      "Save", "Don't Save"))
    {
      case 0 : /* Cancel */
        return;
      case 1 : /* Save */
        save_cb(NULL, NULL);
        if (modflag) return;  // Didn't save!
    }
  }

  // Clear the current data...
  delete_all();
  set_filename(NULL);
  set_modflag(0, 0);
}

/**
 Open the template browser and load a new file from templates.
 If the current design was modified, Fluid will ask for user confirmation.
 \param[in] w widget that caused this request, unused
 \param[in] v if v is set, don't ask for confirmation
 */
void new_from_template_cb(Fl_Widget *w, void *v) {
  new_cb(w, v);

  // Setup the template panel...
  if (!template_panel) make_template_panel();

  template_clear();
  template_browser->add("Blank");
  template_load();

  template_name->hide();
  template_name->value("");

  template_instance->show();
  template_instance->deactivate();
  template_instance->value("");

  template_delete->show();

  template_submit->label("New");
  template_submit->deactivate();

  template_panel->label("New");

  //if ( template_browser->size() == 1 ) { // only one item?
    template_browser->value(1);          // select it
    template_browser->do_callback();
  //}

  // Show the panel and wait for the user to do something...
  template_panel->show();
  while (template_panel->shown()) Fl::wait();

  // See if the user chose anything...
  int item = template_browser->value();
  if (item < 1) return;

  // Load the template, if any...
  const char *tname = (const char *)template_browser->data(item);

  if (tname) {
    // Grab the instance name...
    const char *iname = template_instance->value();

    if (iname && *iname) {
      // Copy the template to a temp file, then read it in...
      char line[1024], *ptr, *next;
      FILE *infile, *outfile;

      if ((infile = fl_fopen(tname, "r")) == NULL) {
        fl_alert("Error reading template file \"%s\":\n%s", tname,
                 strerror(errno));
        set_modflag(0);
        undo_clear();
        return;
      }

      if ((outfile = fl_fopen(cutfname(1), "w")) == NULL) {
        fl_alert("Error writing buffer file \"%s\":\n%s", cutfname(1),
                 strerror(errno));
        fclose(infile);
        set_modflag(0);
        undo_clear();
        return;
      }

      while (fgets(line, sizeof(line), infile)) {
        // Replace @INSTANCE@ with the instance name...
        for (ptr = line; (next = strstr(ptr, "@INSTANCE@")) != NULL; ptr = next + 10) {
          fwrite(ptr, next - ptr, 1, outfile);
          fputs(iname, outfile);
        }

        fputs(ptr, outfile);
      }

      fclose(infile);
      fclose(outfile);

      undo_suspend();
      read_file(cutfname(1), 0);
      fl_unlink(cutfname(1));
      undo_resume();
    } else {
      // No instance name, so read the template without replacements...
      undo_suspend();
      read_file(tname, 0);
      undo_resume();
    }
  }

  set_modflag(0);
  undo_clear();
}

/**
 Generate the C++ source and header filenames and write those files.

 This function creates the source filename by setting the file
 extension to \c code_file_name and a header filename
 with the extension \c code_file_name which are both
 settable by the user.

 In batch_mode, the function will either be silent, or write an error message
 to \c stderr and exit with exit code 1.

 In interactive mode, we will pop up an error message, or, if the user
 hasn't isabled that, pop up a confirmation message.

 \return 1 if the operation failed, 0 if it succeeded
 */
int write_code_files() {
  if (!filename) {
    save_cb(0,0);
    if (!filename) return 1;
  }
  char cname[FL_PATH_MAX];
  char hname[FL_PATH_MAX];
  strlcpy(i18n_program, fl_filename_name(filename), sizeof(i18n_program));
  fl_filename_setext(i18n_program, sizeof(i18n_program), "");
  if (*code_file_name == '.' && strchr(code_file_name, '/') == NULL) {
    strlcpy(cname, fl_filename_name(filename), sizeof(cname));
    fl_filename_setext(cname, sizeof(cname), code_file_name);
  } else {
    strlcpy(cname, code_file_name, sizeof(cname));
  }
  if (*header_file_name == '.' && strchr(header_file_name, '/') == NULL) {
    strlcpy(hname, fl_filename_name(filename), sizeof(hname));
    fl_filename_setext(hname, sizeof(hname), header_file_name);
  } else {
    strlcpy(hname, header_file_name, sizeof(hname));
  }
  if (!batch_mode) goto_source_dir();
  int x = write_code(cname,hname);
  if (!batch_mode) leave_source_dir();
  strlcat(cname, " and ", sizeof(cname));
  strlcat(cname, hname, sizeof(cname));
  if (batch_mode) {
    if (!x) {fprintf(stderr,"%s : %s\n",cname,strerror(errno)); exit(1);}
  } else {
    if (!x) {
      fl_message("Can't write %s: %s", cname, strerror(errno));
    } else {
      set_modflag(-1, 0);
      if (completion_button->value()) {
        fl_message("Wrote %s", cname);
      }
    }
  }
  return 0;
}

/**
 Callback to write C++ code and header files.
 */
void write_cb(Fl_Widget *, void *) {
    write_code_files();
}

/**
 Write the strings that are used in i18n.
 */
void write_strings_cb(Fl_Widget *, void *) {
  static const char *exts[] = { ".txt", ".po", ".msg" };
  if (!filename) {
    save_cb(0,0);
    if (!filename) return;
  }
  char sname[FL_PATH_MAX];
  strlcpy(sname, fl_filename_name(filename), sizeof(sname));
  fl_filename_setext(sname, sizeof(sname), exts[i18n_type]);
  if (!batch_mode) goto_source_dir();
  int x = write_strings(sname);
  if (!batch_mode) leave_source_dir();
  if (batch_mode) {
    if (x) {fprintf(stderr,"%s : %s\n",sname,strerror(errno)); exit(1);}
  } else {
    if (x) {
      fl_message("Can't write %s: %s", sname, strerror(errno));
    } else if (completion_button->value()) {
      fl_message("Wrote %s", sname);
    }
  }
}

/**
 Show the editor for the \c current Fl_Type.
 */
void openwidget_cb(Fl_Widget *, void *) {
  if (!Fl_Type::current) {
    fl_message("Please select a widget");
    return;
  }
  Fl_Type::current->open();
}

/**
 User chose to copy the currently selected widgets.
 */
void copy_cb(Fl_Widget*, void*) {
  if (!Fl_Type::current) {
    fl_beep();
    return;
  }
  ipasteoffset = 10;
  if (!write_file(cutfname(),1)) {
    fl_message("Can't write %s: %s", cutfname(), strerror(errno));
    return;
  }
}

/**
 User chose to cut the currently selected widgets.
 */
void cut_cb(Fl_Widget *, void *) {
  if (!Fl_Type::current) {
    fl_beep();
    return;
  }
  if (!write_file(cutfname(),1)) {
    fl_message("Can't write %s: %s", cutfname(), strerror(errno));
    return;
  }
  undo_checkpoint();
  set_modflag(1);
  ipasteoffset = 0;
  Fl_Type *p = Fl_Type::current->parent;
  while (p && p->selected) p = p->parent;
  delete_all(1);
  if (p) select_only(p);
  //widget_browser->redraw_lines();
}

/**
 User chose to delete the currently selected widgets.
 */
void delete_cb(Fl_Widget *, void *) {
  if (!Fl_Type::current) {
    fl_beep();
    return;
  }
  undo_checkpoint();
  set_modflag(1);
  ipasteoffset = 0;
  Fl_Type *p = Fl_Type::current->parent;
  while (p && p->selected) p = p->parent;
  delete_all(1);
  if (p) select_only(p);
}

/**
 User chose to paste the widgets from the cut buffer.
 */
void paste_cb(Fl_Widget*, void*) {
  //if (ipasteoffset) force_parent = 1;
  pasteoffset = ipasteoffset;
  if (gridx>1) pasteoffset = ((pasteoffset-1)/gridx+1)*gridx;
  if (gridy>1) pasteoffset = ((pasteoffset-1)/gridy+1)*gridy;
  undo_checkpoint();
  undo_suspend();
  if (!read_file(cutfname(), 1)) {
    fl_message("Can't read %s: %s", cutfname(), strerror(errno));
  }
  undo_resume();
  pasteoffset = 0;
  ipasteoffset += 10;
  force_parent = 0;
}

/**
 Duplicate the selected widgets.
 */
void duplicate_cb(Fl_Widget*, void*) {
  if (!Fl_Type::current) {
    fl_beep();
    return;
  }

  if (!write_file(cutfname(1),1)) {
    fl_message("Can't write %s: %s", cutfname(1), strerror(errno));
    return;
  }

  pasteoffset  = 0;
  force_parent = 1;

  undo_checkpoint();
  undo_suspend();
  if (!read_file(cutfname(1), 1)) {
    fl_message("Can't read %s: %s", cutfname(1), strerror(errno));
  }
  fl_unlink(cutfname(1));
  undo_resume();

  force_parent = 0;
}

/**
 User wants to sort selected widgets by y coordinate.
 */
static void sort_cb(Fl_Widget *,void *) {
  sort((Fl_Type*)NULL);
}

/**
 Open the "About" dialog.
 */
void about_cb(Fl_Widget *, void *) {
  if (!about_panel) make_about_panel();
  about_panel->show();
}

/**
 Open a dialog to show the HTML help page form the FLTK documentation folder.
 \param[in] name name of the HTML help file.
 */
void show_help(const char *name) {
  const char    *docdir;
  char          helpname[FL_PATH_MAX];

  if (!help_dialog) help_dialog = new Fl_Help_Dialog();

  if ((docdir = fl_getenv("FLTK_DOCDIR")) == NULL) {
    docdir = FLTK_DOCDIR;
  }
  snprintf(helpname, sizeof(helpname), "%s/%s", docdir, name);

  // make sure that we can read the file
  FILE *f = fopen(helpname, "rb");
  if (f) {
    fclose(f);
    help_dialog->load(helpname);
  } else {
    // if we can not read the file, we display the canned version instead
    // or ask the native browser to open the page on www.fltk.org
    if (strcmp(name, "fluid.html")==0) {
      if (!Fl_Shared_Image::find("embedded:/fluid-org.png"))
        new Fl_PNG_Image("embedded:/fluid-org.png", fluid_org_png, sizeof(fluid_org_png));
      help_dialog->value
      (
       "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
       "<html><head><title>FLTK: Programming with FLUID</title></head><body>\n"
       "<h2>What is FLUID?</h2>\n"
       "The Fast Light User Interface Designer, or FLUID, is a graphical editor "
       "that is used to produce FLTK source code. FLUID edits and saves its state "
       "in <code>.fl</code> files. These files are text, and you can (with care) "
       "edit them in a text editor, perhaps to get some special effects.<p>\n"
       "FLUID can \"compile\" the <code>.fl</code> file into a <code>.cxx</code> "
       "and a <code>.h</code> file. The <code>.cxx</code> file defines all the "
       "objects from the <code>.fl</code> file and the <code>.h</code> file "
       "declares all the global ones. FLUID also supports localization "
       "(Internationalization) of label strings using message files and the GNU "
       "gettext or POSIX catgets interfaces.<p>\n"
       "A simple program can be made by putting all your code (including a <code>"
       "main()</code> function) into the <code>.fl</code> file and thus making the "
       "<code>.cxx</code> file a single source file to compile. Most programs are "
       "more complex than this, so you write other <code>.cxx</code> files that "
       "call the FLUID functions. These <code>.cxx</code> files must <code>"
       "#include</code> the <code>.h</code> file or they can <code>#include</code> "
       "the <code>.cxx</code> file so it still appears to be a single source file.<p>"
       "<img src=\"embedded:/fluid-org.png\"></p>"
       "<p>More information is available online at <a href="
       "\"https://www.fltk.org/doc-1.4/fluid.html\">https://www.fltk.org/</a>"
       "</body></html>"
       );
    } else if (strcmp(name, "license.html")==0) {
      fl_open_uri("https://www.fltk.org/doc-1.4/license.html");
      return;
    } else if (strcmp(name, "index.html")==0) {
      fl_open_uri("https://www.fltk.org/doc-1.4/index.html");
      return;
    } else {
      snprintf(helpname, sizeof(helpname), "https://www.fltk.org/%s", name);
      fl_open_uri(helpname);
      return;
    }
  }
  help_dialog->show();
}

/**
 User wants help on Fluid.
 */
void help_cb(Fl_Widget *, void *) {
  show_help("fluid.html");
}

/**
 User wants to see the Fluid manual.
 */
void manual_cb(Fl_Widget *, void *) {
  show_help("index.html");
}

// ---- Printing

/**
 Open the dialog to allow the user to print the current window.
 */
void print_menu_cb(Fl_Widget *, void *) {
  int w, h, ww, hh;
  int frompage, topage;
  Fl_Type       *t;                     // Current widget
  int           num_windows;            // Number of windows
  Fl_Window_Type *windows[1000];        // Windows to print
  int           winpage;                // Current window page
  Fl_Window *win;

  for (t = Fl_Type::first, num_windows = 0; t; t = t->next) {
    if (t->is_window()) {
      windows[num_windows] = (Fl_Window_Type *)t;
      if (!((Fl_Window*)(windows[num_windows]->o))->shown()) continue;
      num_windows ++;
    }
  }

  Fl_Printer printjob;
  if ( printjob.start_job(num_windows, &frompage, &topage) ) return;
  int pagecount = 0;
  for (winpage = 0; winpage < num_windows; winpage++) {
    float scale = 1, scale_x = 1, scale_y = 1;
    if (winpage+1 < frompage || winpage+1 > topage) continue;
    printjob.start_page();
    printjob.printable_rect(&w, &h);
    // Get the time and date...
    time_t curtime = time(NULL);
    struct tm *curdate = localtime(&curtime);
    char date[1024];
    strftime(date, sizeof(date), "%c", curdate);
    fl_font(FL_HELVETICA, 12);
    fl_color(FL_BLACK);
    fl_draw(date, (w - (int)fl_width(date))/2, fl_height());
    sprintf(date, "%d/%d", ++pagecount, topage-frompage+1);
    fl_draw(date, w - (int)fl_width(date), fl_height());
    // Get the base filename...
    const char *basename = strrchr(filename,
#ifdef _WIN32
                                   '\\'
#else
                                   '/'
#endif
                                   );
    if (basename) basename ++;
    else basename = filename;
    sprintf(date, "%s", basename);
    fl_draw(date, 0, fl_height());
// print centered and scaled to fit in the page
    win = (Fl_Window*)windows[winpage]->o;
    ww = win->decorated_w();
    if(ww > w) scale_x = float(w)/ww;
    hh = win->decorated_h();
    if(hh > h) scale_y = float(h)/hh;
    if (scale_x < scale) scale = scale_x;
    if (scale_y < scale) scale = scale_y;
    if (scale < 1) {
      printjob.scale(scale);
      printjob.printable_rect(&w, &h);
      }
    printjob.origin(w/2, h/2);
    printjob.print_window(win, -ww/2, -hh/2);
    printjob.end_page();
  }
  printjob.end_job();
}

// ---- Main menu bar

/**
 This is the main Fluid menu.

 Design history is manipulated right inside this menu structure.
 Some menuitem change or deactivate correctly, but most items just trigger
 various callbacks.

 \c New_Menu creates new widgets and is explained in detail in another location.

 \see New_Menu
 \todo This menu need some major modernisation. Menus are too long and their
    sorting is not always obvious.
 \todo Shortcuts are all over the palce (Alt, Ctrl, Command, Shift-Ctrl,
    function keys), and there should be a help page listing all shortcuts.
 */
Fl_Menu_Item Main_Menu[] = {
{"&File",0,0,0,FL_SUBMENU},
  {"&New", FL_COMMAND+'n', new_cb, 0},
  {"&Open...", FL_COMMAND+'o', open_cb, 0},
  {"&Insert...", FL_COMMAND+'i', open_cb, (void*)1, FL_MENU_DIVIDER},
  {"&Save", FL_COMMAND+'s', save_cb, 0},
  {"Save &As...", FL_COMMAND+FL_SHIFT+'s', save_cb, (void*)1},
  {"Sa&ve A Copy...", 0, save_cb, (void*)2},
  {"&Revert...", 0, revert_cb, 0, FL_MENU_DIVIDER},
  {"New &From Template...", FL_COMMAND+'N', new_from_template_cb, 0},
  {"Save As &Template...", 0, save_template_cb, 0, FL_MENU_DIVIDER},
  {"&Print...", FL_COMMAND+'p', print_menu_cb},
  {"Write &Code...", FL_COMMAND+FL_SHIFT+'c', write_cb, 0},
  {"&Write Strings...", FL_COMMAND+FL_SHIFT+'w', write_strings_cb, 0, FL_MENU_DIVIDER},
  {relative_history[0], FL_COMMAND+'0', open_history_cb, absolute_history[0]},
  {relative_history[1], FL_COMMAND+'1', open_history_cb, absolute_history[1]},
  {relative_history[2], FL_COMMAND+'2', open_history_cb, absolute_history[2]},
  {relative_history[3], FL_COMMAND+'3', open_history_cb, absolute_history[3]},
  {relative_history[4], FL_COMMAND+'4', open_history_cb, absolute_history[4]},
  {relative_history[5], FL_COMMAND+'5', open_history_cb, absolute_history[5]},
  {relative_history[6], FL_COMMAND+'6', open_history_cb, absolute_history[6]},
  {relative_history[7], FL_COMMAND+'7', open_history_cb, absolute_history[7]},
  {relative_history[8], FL_COMMAND+'8', open_history_cb, absolute_history[8]},
  {relative_history[9], FL_COMMAND+'9', open_history_cb, absolute_history[9], FL_MENU_DIVIDER},
  {"&Quit", FL_COMMAND+'q', exit_cb},
  {0},
{"&Edit",0,0,0,FL_SUBMENU},
  {"&Undo", FL_COMMAND+'z', undo_cb},
  {"&Redo", FL_COMMAND+FL_SHIFT+'z', redo_cb, 0, FL_MENU_DIVIDER},
  {"C&ut", FL_COMMAND+'x', cut_cb},
  {"&Copy", FL_COMMAND+'c', copy_cb},
  {"&Paste", FL_COMMAND+'v', paste_cb},
  {"Dup&licate", FL_COMMAND+'u', duplicate_cb},
  {"&Delete", FL_Delete, delete_cb, 0, FL_MENU_DIVIDER},
  {"Select &All", FL_COMMAND+'a', select_all_cb},
  {"Select &None", FL_COMMAND+FL_SHIFT+'a', select_none_cb, 0, FL_MENU_DIVIDER},
  {"Pr&operties...", FL_F+1, openwidget_cb},
  {"&Sort",0,sort_cb},
  {"&Earlier", FL_F+2, earlier_cb},
  {"&Later", FL_F+3, later_cb},
  {"&Group", FL_F+7, group_cb},
  {"Ung&roup", FL_F+8, ungroup_cb,0, FL_MENU_DIVIDER},
  {"Hide O&verlays",FL_COMMAND+FL_SHIFT+'o',toggle_overlays},
  {"Show Widget &Bin...",FL_ALT+'b',toggle_widgetbin_cb},
  {"Show Source Code...",FL_ALT+FL_SHIFT+'s', (Fl_Callback*)toggle_sourceview_cb, 0, FL_MENU_DIVIDER},
  {"Pro&ject Settings...",FL_ALT+'p',show_project_cb},
  {"GU&I Settings...",FL_ALT+FL_SHIFT+'p',show_settings_cb,0,FL_MENU_DIVIDER},
  {"Global &FLTK Settings...",FL_ALT+FL_SHIFT+'g',show_global_settings_cb},
  {0},
{"&New", 0, 0, (void *)New_Menu, FL_SUBMENU_POINTER},
{"&Layout",0,0,0,FL_SUBMENU},
  {"&Align",0,0,0,FL_SUBMENU},
    {"&Left",0,(Fl_Callback *)align_widget_cb,(void*)10},
    {"&Center",0,(Fl_Callback *)align_widget_cb,(void*)11},
    {"&Right",0,(Fl_Callback *)align_widget_cb,(void*)12},
    {"&Top",0,(Fl_Callback *)align_widget_cb,(void*)13},
    {"&Middle",0,(Fl_Callback *)align_widget_cb,(void*)14},
    {"&Bottom",0,(Fl_Callback *)align_widget_cb,(void*)15},
    {0},
  {"&Space Evenly",0,0,0,FL_SUBMENU},
    {"&Across",0,(Fl_Callback *)align_widget_cb,(void*)20},
    {"&Down",0,(Fl_Callback *)align_widget_cb,(void*)21},
    {0},
  {"&Make Same Size",0,0,0,FL_SUBMENU},
    {"&Width",0,(Fl_Callback *)align_widget_cb,(void*)30},
    {"&Height",0,(Fl_Callback *)align_widget_cb,(void*)31},
    {"&Both",0,(Fl_Callback *)align_widget_cb,(void*)32},
    {0},
  {"&Center In Group",0,0,0,FL_SUBMENU},
    {"&Horizontal",0,(Fl_Callback *)align_widget_cb,(void*)40},
    {"&Vertical",0,(Fl_Callback *)align_widget_cb,(void*)41},
    {0},
  {"Set &Widget Size",0,0,0,FL_SUBMENU|FL_MENU_DIVIDER},
    {"&Tiny",FL_ALT+'1',(Fl_Callback *)widget_size_cb,(void*)8,0,FL_NORMAL_LABEL,FL_HELVETICA,8},
    {"&Small",FL_ALT+'2',(Fl_Callback *)widget_size_cb,(void*)11,0,FL_NORMAL_LABEL,FL_HELVETICA,11},
    {"&Normal",FL_ALT+'3',(Fl_Callback *)widget_size_cb,(void*)14,0,FL_NORMAL_LABEL,FL_HELVETICA,14},
    {"&Medium",FL_ALT+'4',(Fl_Callback *)widget_size_cb,(void*)18,0,FL_NORMAL_LABEL,FL_HELVETICA,18},
    {"&Large",FL_ALT+'5',(Fl_Callback *)widget_size_cb,(void*)24,0,FL_NORMAL_LABEL,FL_HELVETICA,24},
    {"&Huge",FL_ALT+'6',(Fl_Callback *)widget_size_cb,(void*)32,0,FL_NORMAL_LABEL,FL_HELVETICA,32},
    {0},
  {"&Grid and Size Settings...",FL_COMMAND+'g',show_grid_cb},
  {0},
{"&Shell",0,0,0,FL_SUBMENU},
  {"Execute &Command...",FL_ALT+'x',(Fl_Callback *)show_shell_window},
  {"Execute &Again...",FL_ALT+'g',(Fl_Callback *)do_shell_command},
  {0},
{"&Help",0,0,0,FL_SUBMENU},
  {"&Rapid development with FLUID...",0,help_cb},
  {"&FLTK Programmers Manual...",0,manual_cb, 0, FL_MENU_DIVIDER},
  {"&About FLUID...",0,about_cb},
  {0},
{0}};

/**
 Change the app's and hence preview the design's scheme.
 The scheme setting is stored inthe app preferences.
 */
void scheme_cb(Fl_Choice *, void *) {
  if (batch_mode)
    return;

  switch (scheme_choice->value()) {
    case 0 : // Default
      Fl::scheme(NULL);
      break;
    case 1 : // None
      Fl::scheme("none");
      break;
    case 2 : // Plastic
      Fl::scheme("plastic");
      break;
    case 3 : // GTK+
      Fl::scheme("gtk+");
      break;
    case 4 : // Gleam
      Fl::scheme("gleam");
      break;
  }

  fluid_prefs.set("scheme", scheme_choice->value());
}

/**
 Show or hide the widget bin.
 The state is stored in the app preferences.
 */
void toggle_widgetbin_cb(Fl_Widget *, void *) {
  if (!widgetbin_panel) {
    make_widgetbin();
    if (!position_window(widgetbin_panel,"widgetbin_pos", 1, 320, 30)) return;
  }

  if (widgetbin_panel->visible()) {
    widgetbin_panel->hide();
    widgetbin_item->label("Show Widget &Bin...");
  } else {
    widgetbin_panel->show();
    widgetbin_item->label("Hide Widget &Bin");
  }
}

/**
 Show or hide the source code preview.
 The state is stored in the app preferences.
 */
void toggle_sourceview_cb(Fl_Double_Window *, void *) {
  if (!sourceview_panel) {
    make_sourceview();
    sourceview_panel->callback((Fl_Callback*)toggle_sourceview_cb);
    Fl_Preferences svp(fluid_prefs, "sourceview");
    int autorefresh;
    svp.get("autorefresh", autorefresh, 1);
    sv_autorefresh->value(autorefresh);
    int autoposition;
    svp.get("autoposition", autoposition, 1);
    sv_autoposition->value(autoposition);
    int tab;
    svp.get("tab", tab, 0);
    if (tab>=0 && tab<sv_tab->children()) sv_tab->value(sv_tab->child(tab));
    if (!position_window(sourceview_panel,"sourceview_pos", 0, 320, 120, 550, 500)) return;
  }

  if (sourceview_panel->visible()) {
    sourceview_panel->hide();
    sourceview_item->label("Show Source Code...");
  } else {
    sourceview_panel->show();
    sourceview_item->label("Hide Source Code...");
    update_sourceview_cb(0,0);
  }
}

/**
 Show or hide the source code preview, called from a button.
 The state is stored in the app preferences.
 */
void toggle_sourceview_b_cb(Fl_Button*, void *) {
  toggle_sourceview_cb(0,0);
}

/**
 Build the main app window and create a few other dialogs.
 */
void make_main_window() {
  if (!batch_mode) {
    fluid_prefs.get("snap", snap, 1);
    fluid_prefs.get("gridx", gridx, 5);
    fluid_prefs.get("gridy", gridy, 5);
    fluid_prefs.get("show_guides", show_guides, 0);
    fluid_prefs.get("widget_size", Fl_Widget_Type::default_size, 14);
    fluid_prefs.get("show_comments", show_comments, 1);
    shell_prefs_get();
    make_layout_window();
    make_shell_window();
  }

  if (!main_window) {
    Fl_Widget *o;
    loadPixmaps();
    main_window = new Fl_Double_Window(WINWIDTH,WINHEIGHT,"fluid");
    main_window->box(FL_NO_BOX);
    o = make_widget_browser(0,MENUHEIGHT,BROWSERWIDTH,BROWSERHEIGHT);
    o->box(FL_FLAT_BOX);
    o->tooltip("Double-click to view or change an item.");
    main_window->resizable(o);
    main_menubar = new Fl_Menu_Bar(0,0,BROWSERWIDTH,MENUHEIGHT);
    main_menubar->menu(Main_Menu);
    // quick access to all dynamic menu items
    save_item = (Fl_Menu_Item*)main_menubar->find_item(save_cb);
    history_item = (Fl_Menu_Item*)main_menubar->find_item(open_history_cb);
    widgetbin_item = (Fl_Menu_Item*)main_menubar->find_item(toggle_widgetbin_cb);
    sourceview_item = (Fl_Menu_Item*)main_menubar->find_item((Fl_Callback*)toggle_sourceview_cb);
    overlay_item = (Fl_Menu_Item*)main_menubar->find_item((Fl_Callback*)toggle_overlays);
    main_menubar->global();
    fill_in_New_Menu();
    main_window->end();
  }

  if (!batch_mode) {
    load_history();
    make_settings_window();
    make_global_settings_window();
  }
}

/**
 Load file history from preferences.

 This loads the absolute filepaths of the last 10 used design files.
 It also computes and stores the relative filepaths for display in
 the main menu.
 */
void load_history() {
  int   i;              // Looping var
  int   max_files;


  fluid_prefs.get("recent_files", max_files, 5);
  if (max_files > 10) max_files = 10;

  for (i = 0; i < max_files; i ++) {
    fluid_prefs.get( Fl_Preferences::Name("file%d", i), absolute_history[i], "", sizeof(absolute_history[i]));
    if (absolute_history[i][0]) {
      // Make a relative version of the filename for the menu...
      fl_filename_relative(relative_history[i], sizeof(relative_history[i]),
                           absolute_history[i]);

      if (i == 9) history_item[i].flags = FL_MENU_DIVIDER;
      else history_item[i].flags = 0;
    } else break;
  }

  for (; i < 10; i ++) {
    if (i) history_item[i-1].flags |= FL_MENU_DIVIDER;
    history_item[i].hide();
  }
}

/**
 Update file history from preferences.

 Add this new filepath to the history and update the main menu.
 Writes the new file history to the app preferences.

 \param[in] flname path or filename of .fl file, will be converted into an
    absolute file path based on the current working directory.
 */
void update_history(const char *flname) {
  int   i;              // Looping var
  char  absolute[FL_PATH_MAX];
  int   max_files;


  fluid_prefs.get("recent_files", max_files, 5);
  if (max_files > 10) max_files = 10;

  fl_filename_absolute(absolute, sizeof(absolute), flname);

  for (i = 0; i < max_files; i ++)
#if defined(_WIN32) || defined(__APPLE__)
    if (!strcasecmp(absolute, absolute_history[i])) break;
#else
    if (!strcmp(absolute, absolute_history[i])) break;
#endif // _WIN32 || __APPLE__

  if (i == 0) return;

  if (i >= max_files) i = max_files - 1;

  // Move the other flnames down in the list...
  memmove(absolute_history + 1, absolute_history,
          i * sizeof(absolute_history[0]));
  memmove(relative_history + 1, relative_history,
          i * sizeof(relative_history[0]));

  // Put the new file at the top...
  strlcpy(absolute_history[0], absolute, sizeof(absolute_history[0]));

  fl_filename_relative(relative_history[0], sizeof(relative_history[0]),
                       absolute_history[0]);

  // Update the menu items as needed...
  for (i = 0; i < max_files; i ++) {
    fluid_prefs.set( Fl_Preferences::Name("file%d", i), absolute_history[i]);
    if (absolute_history[i][0]) {
      if (i == 9) history_item[i].flags = FL_MENU_DIVIDER;
      else history_item[i].flags = 0;
    } else break;
  }

  for (; i < 10; i ++) {
    fluid_prefs.set( Fl_Preferences::Name("file%d", i), "");
    if (i) history_item[i-1].flags |= FL_MENU_DIVIDER;
    history_item[i].hide();
  }
  fluid_prefs.flush();
}

/**
 Set the filename of the current .fl design.
 \param[in] c the new absolute filename and path
 */
void set_filename(const char *c) {
  if (filename) free((void *)filename);
  filename = c ? fl_strdup(c) : NULL;

  if (filename && !batch_mode)
    update_history(filename);

  set_modflag(modflag);
}

/**
 Set the "modified" flag and update the title of the main window.

 The first argument sets the modifaction state of the current design against
 the corresponding .fl design file. Any change to the widget tree will mark
 the design 'modified'. Saving the design will mark it clean.

 The second argument is optional and set the modification state of the current
 design against the source code and header file. Any change to the tree,
 including saving the tree, will mark the code 'outdated'. Generating source
 code and header files will clear this flag until the next modification.

 \param[in] mf 0 to clear the modflag, 1 to mark the design "modified", -1 to
    ignore this parameter
 \param[in] mfc default -1 to let \c mf control \c modflag_c, 0 to mark the
    code files current, 1 to mark it out of date.
 */
void set_modflag(int mf, int mfc) {
  const char *basename;
  const char *code_ext = NULL;
  static char title[FL_PATH_MAX];

  // Update the modflag_c to the worst possible condition. We could be a bit
  // more graceful and compare modification times of the files, but C++ has
  // no API for that until C++17.
  if (mf!=-1) {
    modflag = mf;
    if (mfc==-1 && mf==1)
      mfc = mf;
  }
  if (mfc!=-1) {
    modflag_c = mfc;
  }

  if (main_window) {
    if (!filename) basename = "Untitled.fl";
    else if ((basename = strrchr(filename, '/')) != NULL) basename ++;
#if defined(_WIN32)
    else if ((basename = strrchr(filename, '\\')) != NULL) basename ++;
#endif // _WIN32
    else basename = filename;

    if (code_file_name)
      code_ext = fl_filename_ext(code_file_name);
    else
      code_ext = ".cxx";

    char mod_star = modflag ? '*' : ' ';
    char mod_c_star = modflag_c ? '*' : ' ';
    snprintf(title, sizeof(title), "%s%c  %s%c",
             basename, mod_star, code_ext, mod_c_star);
    main_window->label(title);
  }
  // if the UI was modified in any way, update the Source View panel
  if (sourceview_panel && sourceview_panel->visible() && sv_autorefresh->value())
  {
    // we will only update earliest 0.5 seconds after the last change, and only
    // if no other change was made, so dragging a widget will not generate any
    // CPU load
    Fl::remove_timeout(update_sourceview_timer, 0);
    Fl::add_timeout(0.5, update_sourceview_timer, 0);
  }

  // Enable/disable the Save menu item...
  if (modflag) save_item->activate();
  else save_item->deactivate();
}

// ---- Sourceview implementation

static char *sv_source_filename = NULL;
static char *sv_header_filename = NULL;

/**
 Update the header and source code highlighting depending on the
 currently selected object

 The Source View system offers an immediate preview of the code
 files that will be generated by FLUID. It also marks the code
 generated for the last selected item in the header and the source
 file.
 */
void update_sourceview_position()
{
  if (!sourceview_panel || !sourceview_panel->visible())
    return;
  if (sv_autoposition->value()==0)
    return;
  if (sourceview_panel && sourceview_panel->visible() && Fl_Type::current) {
    int pos0, pos1;
    if (sv_source->visible_r()) {
      pos0 = Fl_Type::current->code_position;
      pos1 = Fl_Type::current->code_position_end;
      if (pos0>=0) {
        if (pos1<pos0)
          pos1 = pos0;
        sv_source->buffer()->highlight(pos0, pos1);
        int line = sv_source->buffer()->count_lines(0, pos0);
        sv_source->scroll(line, 0);
      }
    }
    if (sv_header->visible_r()) {
      pos0 = Fl_Type::current->header_position;
      pos1 = Fl_Type::current->header_position_end;
      if (pos0>=0) {
        if (pos1<pos0)
          pos1 = pos0;
        sv_header->buffer()->highlight(pos0, pos1);
        int line = sv_header->buffer()->count_lines(0, pos0);
        sv_header->scroll(line, 0);
      }
    }
  }
}

/**
 Callback to update the sourceview position.
 */
void update_sourceview_position_cb(Fl_Tabs*, void*)
{
  update_sourceview_position();
}

/**
 Generate a header and source file in a temporary directory and
 load those into the Code Viewer widgets.
 */
void update_sourceview_cb(Fl_Button*, void*)
{
  if (!sourceview_panel || !sourceview_panel->visible())
    return;
  // generate space for the source and header file filenames
  if (!sv_source_filename) {
    sv_source_filename = (char*)malloc(FL_PATH_MAX);
    fluid_prefs.getUserdataPath(sv_source_filename, FL_PATH_MAX);
    strlcat(sv_source_filename, "source_view_tmp.cxx", FL_PATH_MAX);
  }
  if (!sv_header_filename) {
    sv_header_filename = (char*)malloc(FL_PATH_MAX);
    fluid_prefs.getUserdataPath(sv_header_filename, FL_PATH_MAX);
    strlcat(sv_header_filename, "source_view_tmp.h", FL_PATH_MAX);
  }

  strlcpy(i18n_program, fl_filename_name(sv_source_filename), sizeof(i18n_program));
  fl_filename_setext(i18n_program, sizeof(i18n_program), "");
  const char *code_file_name_bak = code_file_name;
  code_file_name = sv_source_filename;
  const char *header_file_name_bak = header_file_name;
  header_file_name = sv_header_filename;

  // generate the code and load the files
  write_sourceview = 1;
  // generate files
  if (write_code(sv_source_filename, sv_header_filename))
  {
    // load file into source editor
    int pos = sv_source->top_line();
    sv_source->buffer()->loadfile(sv_source_filename);
    sv_source->scroll(pos, 0);
    // load file into header editor
    pos = sv_header->top_line();
    sv_header->buffer()->loadfile(sv_header_filename);
    sv_header->scroll(pos, 0);
    // update the source code highlighting
    update_sourceview_position();
  }
  write_sourceview = 0;

  code_file_name = code_file_name_bak;
  header_file_name = header_file_name_bak;
}

/**
 This is called by the timer itself
 */
void update_sourceview_timer(void*)
{
  update_sourceview_cb(0,0);
}

// ---- Main program entry point

/**
 Handle command line arguments.
 \param[in] argc number of arguments in the list
 \param[in] argv pointer to an array of arguments
 \param[inout] i current argument index
 \return number of arguments used; if 0, the argument is not supported
 */
static int arg(int argc, char** argv, int& i) {
  if (argv[i][1] == 'd' && !argv[i][2]) {G_debug=1; i++; return 1;}
  if (argv[i][1] == 'u' && !argv[i][2]) {update_file++; batch_mode++; i++; return 1;}
  if (argv[i][1] == 'c' && !argv[i][2]) {compile_file++; batch_mode++; i++; return 1;}
  if (argv[i][1] == 'c' && argv[i][2] == 's' && !argv[i][3]) {compile_file++; compile_strings++; batch_mode++; i++; return 1;}
  if (argv[i][1] == 'o' && !argv[i][2] && i+1 < argc) {
    code_file_name = argv[i+1];
    code_file_set  = 1;
    i += 2;
    return 2;
  }
  if (argv[i][1] == 'h' && !argv[i][2]) {
    header_file_name = argv[i+1];
    header_file_set  = 1;
    i += 2;
    return 2;
  }
  return 0;
}

#if ! (defined(_WIN32) && !defined (__CYGWIN__))

int quit_flag = 0;
#include <signal.h>
#ifdef _sigargs
#define SIGARG _sigargs
#else
#ifdef __sigargs
#define SIGARG __sigargs
#else
#define SIGARG int // you may need to fix this for older systems
#endif
#endif

extern "C" {
static void sigint(SIGARG) {
  signal(SIGINT,sigint);
  quit_flag = 1;
}
}

#endif

/**
 Start Fluid.

 Fluid can run in interactive mode with a full user interface to design new
 user interfaces and write the C++ files to manage them,

 Fluid can run form the command line in batch mode to convert .fl design files
 into C++ source and header files. In batch mode, no diplay is needed,
 particularly no X11 connection will be attempted on Linux/Unix.

 \param[in] argc number of arguments in the list
 \param[in] argv pointer to an array of arguments
 \return in batch mode, an error code will be returned via \c exit() . This
    function return 1, if there was an error in the parameters list.
 \todo On MSWindows, Fluid can under certain conditions open a dialog box, even
    in batch mode. Is that intentional? Does it circumvent issues with Windows'
 stderr and stdout?
 */
int main(int argc,char **argv) {
  int i = 1;

  setlocale(LC_ALL, "");      // enable multilanguage errors in file chooser
  setlocale(LC_NUMERIC, "C"); // make sure numeric values are written correctly

  if (!Fl::args(argc,argv,i,arg) || i < argc-1) {
    static const char *msg =
      "usage: %s <switches> name.fl\n"
      " -u : update .fl file and exit (may be combined with '-c' or '-cs')\n"
      " -c : write .cxx and .h and exit\n"
      " -cs : write .cxx and .h and strings and exit\n"
      " -o <name> : .cxx output filename, or extension if <name> starts with '.'\n"
      " -h <name> : .h output filename, or extension if <name> starts with '.'\n"
      " -d : enable internal debugging\n";
#ifdef _MSC_VER
    fl_message("%s\n", msg);
#else
    fprintf(stderr, "%s\n", msg);
#endif
    return 1;
  }

  const char *c = argv[i];

  fl_register_images();

  make_main_window();

  if (c) set_filename(c);
  if (!batch_mode) {
#ifdef __APPLE__
    fl_open_callback(apple_open_cb);
#endif // __APPLE__
    Fl::visual((Fl_Mode)(FL_DOUBLE|FL_INDEX));
    Fl_File_Icon::load_system_icons();
    main_window->callback(exit_cb);
    position_window(main_window,"main_window_pos", 1, 10, 30, WINWIDTH, WINHEIGHT );
    main_window->show(argc,argv);
    toggle_widgetbin_cb(0,0);
    toggle_sourceview_cb(0,0);
    if (!c && openlast_button->value() && absolute_history[0][0]) {
      // Open previous file when no file specified...
      open_history_cb(0, absolute_history[0]);
    }
  }
  undo_suspend();
  if (c && !read_file(c,0)) {
    if (batch_mode) {
      fprintf(stderr,"%s : %s\n", c, strerror(errno));
      exit(1);
    }
    fl_message("Can't read %s: %s", c, strerror(errno));
  }
  undo_resume();

  if (update_file) {            // fluid -u
    write_file(c,0);
    if (!compile_file)
      exit(0);
  }

  if (compile_file) {           // fluid -c[s]
    if (compile_strings)
      write_strings_cb(0,0);
    write_cb(0,0);
    exit(0);
  }
  set_modflag(0);
  undo_clear();
#ifndef _WIN32
  signal(SIGINT,sigint);
#endif

  // Set (but do not start) timer callback for external editor updates
  ExternalCodeEditor::set_update_timer_callback(external_editor_timer);

  grid_cb(horizontal_input, 0); // Makes sure that windows get snap params...

#ifdef _WIN32
  Fl::run();
#else
  while (!quit_flag) Fl::wait();

  if (quit_flag) exit_cb(0,0);
#endif // _WIN32

  undo_clear();

  return (0);
}

/// \}

