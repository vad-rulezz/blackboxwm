// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Window.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
//         Bradley T Hughes <bhughes at trolltech.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef   __Window_hh
#define   __Window_hh

#include "BlackboxResource.hh"
#include "Screen.hh"

#include <Netwm.hh>


enum WindowType {
  WindowTypeNormal,
  WindowTypeDialog,
  WindowTypeDesktop,
  WindowTypeDock,
  WindowTypeMenu,
  WindowTypeSplash,
  WindowTypeToolbar,
  WindowTypeUtility
};

enum WindowFunction {
  WindowFunctionResize   = 1<<0,
  WindowFunctionMove     = 1<<1,
  WindowFunctionShade    = 1<<2,
  WindowFunctionIconify  = 1<<3,
  WindowFunctionMaximize = 1<<4,
  WindowFunctionClose    = 1<<5,
  AllWindowFunctions     = (WindowFunctionResize |
                            WindowFunctionMove |
                            WindowFunctionShade |
                            WindowFunctionIconify |
                            WindowFunctionMaximize |
                            WindowFunctionClose)
};
typedef unsigned char WindowFunctionFlags;

enum WindowDecoration {
  WindowDecorationTitlebar = 1<<0,
  WindowDecorationHandle   = 1<<1,
  WindowDecorationGrip     = 1<<2,
  WindowDecorationBorder   = 1<<3,
  WindowDecorationIconify  = 1<<4,
  WindowDecorationMaximize = 1<<5,
  WindowDecorationClose    = 1<<6,
  AllWindowDecorations     = (WindowDecorationTitlebar |
                              WindowDecorationHandle |
                              WindowDecorationGrip |
                              WindowDecorationBorder |
                              WindowDecorationIconify |
                              WindowDecorationMaximize |
                              WindowDecorationClose)
};
typedef unsigned char WindowDecorationFlags;


class BlackboxWindow : public StackEntity, public bt::TimeoutHandler,
                                                      public bt::EventHandler, public bt::NoCopy {
  Blackbox *blackbox;
  BScreen *screen;
  bt::Timer *timer;

  Time lastButtonPressTime;  // used for double clicks, when were we clicked

  unsigned int window_number;

  enum FocusMode { F_NoInput = 0, F_Passive,
                   F_LocallyActive, F_GloballyActive };
  enum WMSkip { SKIP_NONE, SKIP_TASKBAR, SKIP_PAGER, SKIP_BOTH };

  struct WMState {
    bool visible,            // is visible?
      modal,                 // is modal? (must be dismissed to continue)
      shaded,                // is shaded?
      iconic,                // is iconified?
      fullscreen,            // is a full screen window
      moving,                // is moving?
      resizing,              // is resizing?
      focused,               // has focus?
      send_focus_message,    // should we send focus messages to our client?
      shaped;                // does the frame use the shape extension?
    unsigned int maximized;  // maximize is special, the number corresponds
                             // with a mouse button
                             // if 0, not maximized
                             // 1 = HorizVert, 2 = Vertical, 3 = Horizontal
    WMSkip skip;             // none, taskbar, pager, both

    inline WMState(void)
      : visible(false), modal(false), shaded(false), iconic(false),
        fullscreen(false), moving(false), resizing(false), focused(false),
        send_focus_message(false), shaped(false), maximized(0), skip(SKIP_NONE)
    { }
  };

  struct _client {
    Window window,                  // the client's window
      window_group;
    Colormap colormap;
    BlackboxWindow *transient_for;  // which window are we a transient for?
    BlackboxWindowList transientList; // which windows are our transients?

    std::string title, visible_title, icon_title;

    bt::Rect rect, premax;

    int old_bw;                       // client's borderwidth

    unsigned int
    min_width, min_height,        // can not be resized smaller
      max_width, max_height,        // can not be resized larger
      width_inc, height_inc,        // increment step
      min_aspect_x, min_aspect_y,   // minimum aspect ratio
      max_aspect_x, max_aspect_y,   // maximum aspect ratio
      base_width, base_height,
      win_gravity;

    unsigned long current_state, normal_hint_flags;
    unsigned int workspace;

    bt::Netwm::Strut *strut;
    FocusMode focus_mode;
    WMState state;
    WindowType window_type;
    WindowFunctionFlags functions;
    WindowDecorationFlags decorations;
  } client;

  /*
   * client window = the application's window
   * frame window = the window drawn around the outside of the client window
   *                by the window manager which contains items like the
   *                titlebar and close button
   * title = the titlebar drawn above the client window, it displays the
   *         window's name and any buttons for interacting with the window,
   *         such as iconify, maximize, and close
   * label = the window in the titlebar where the title is drawn
   * buttons = maximize, iconify, close
   * handle = the bar drawn at the bottom of the window, which contains the
   *          left and right grips used for resizing the window
   * grips = the smaller reactangles in the handle, one of each side of it.
   *         When clicked and dragged, these resize the window interactively
   * border = the line drawn around the outside edge of the frame window,
   *          between the title, the bordered client window, and the handle.
   *          Also drawn between the grips and the handle
   */

  struct _frame {
    ScreenResource::WindowStyle* style;

    // u -> unfocused, f -> has focus
    unsigned long uborder_pixel, fborder_pixel;
    Pixmap ulabel, flabel, utitle, ftitle, uhandle, fhandle,
      ubutton, fbutton, pbutton, ugrip, fgrip;

    Window window,       // the frame
      plate,             // holds the client
      title,
      label,
      handle,
      close_button, iconify_button, maximize_button,
      right_grip, left_grip;

    /*
     * size and location of the box drawn while the window dimensions or
     * location is being changed, ie. resized or moved
     */
    bt::Rect changing;

    // frame geometry
    bt::Rect rect;

    /*
     * margins between the frame and client, this has nothing to do
     * with netwm, it is simply code reuse for similar functionality
     */
    bt::Netwm::Strut margin;
    int grab_x, grab_y;         // where was the window when it was grabbed?

    unsigned int inside_w, inside_h, // window w/h without border_w
      label_w, mwm_border_w, border_w;
  } frame;

  Window createToplevelWindow();
  Window createChildWindow(Window parent, unsigned long event_mask,
                           Cursor = None);

  std::string readWMName(void);
  std::string readWMIconName(void);

  void getNetwmHints(void);
  void getWMNormalHints(void);
  void getWMProtocols(void);
  void getWMHints(void);
  void getMWMHints(void);
  void getTransientInfo(void);

  void setNetWMAttributes(void);

  void associateClientWindow(void);

  void decorate(void);

  void positionButtons(bool redecorate_label = false);
  void positionWindows(void);

  void createTitlebar(void);
  void destroyTitlebar(void);
  void createHandle(void);
  void destroyHandle(void);
  void createGrips(void);
  void destroyGrips(void);
  void createIconifyButton(void);
  void destroyIconifyButton(void);
  void createMaximizeButton(void);
  void destroyMaximizeButton(void);
  void createCloseButton(void);
  void destroyCloseButton(void);

  void redrawWindowFrame(void) const;
  void redrawTitle(void) const;
  void redrawLabel(void) const;
  void redrawAllButtons(void) const;
  void redrawCloseButton(bool pressed) const;
  void redrawIconifyButton(bool pressed) const;
  void redrawMaximizeButton(bool pressed) const;
  void redrawHandle(void) const;
  void redrawGrips(void) const;

  void applyGravity(bt::Rect &r);
  void restoreGravity(bt::Rect &r);

  bool getState(void);
  void setState(unsigned long new_state);
  void clearState(void);

  void upsize(void);

  void showGeometry(const bt::Rect &r) const;

  enum Corner { TopLeft, TopRight, BottomLeft, BottomRight };
  void constrain(Corner anchor);

 public:
  BlackboxWindow(Blackbox *b, Window w, BScreen *s);
  virtual ~BlackboxWindow(void);

  inline bool isTransient(void) const { return client.transient_for != 0; }
  inline bool isModal(void) const { return client.state.modal; }

  inline WindowType windowType(void) const
  { return client.window_type; }
  inline bool isDesktop(void) const
  { return client.window_type == WindowTypeDesktop; }

  inline bool hasWindowFunction(WindowFunction function) const
  { return client.functions & function; }
  inline bool hasWindowDecoration(WindowDecoration decoration) const
  { return client.decorations & decoration; }

  inline const BlackboxWindowList &getTransients(void) const
  { return client.transientList; }
  BlackboxWindow *getTransientFor(void) const;

  inline BScreen *getScreen(void) const { return screen; }

  // StackEntity interface
  inline Window windowID(void) const { return frame.window; }

  inline Window getFrameWindow(void) const { return frame.window; }
  inline Window getClientWindow(void) const { return client.window; }
  inline Window getGroupWindow(void) const { return client.window_group; }

  inline const char *getTitle(void) const
  { return client.title.c_str(); }
  inline const char *getIconTitle(void) const
  { return client.icon_title.c_str(); }

  inline unsigned int workspace(void) const
  { return client.workspace; }
  void setWorkspace(unsigned int new_workspace);

  inline unsigned int windowNumber(void) const
  { return window_number; }
  inline void setWindowNumber(int n)
  { window_number = n; }

  inline const bt::Rect &frameRect(void) const { return frame.rect; }
  inline const bt::Rect &clientRect(void) const { return client.rect; }

  inline unsigned int getTitleHeight(void) const
  { return frame.style->title_height; }

  unsigned long normalHintFlags(void) const
  { return client.normal_hint_flags; }

  unsigned long currentState(void) const
  { return client.current_state; }

  inline void setModal(bool flag) { client.state.modal = flag; }

  bool validateClient(void) const;

  inline bool isFocused(void) const
  { return client.state.focused; }
  void setFocused(bool focused);
  bool setInputFocus(void);

  inline bool isVisible(void) const
  { return client.state.visible; }
  void show(void);
  void hide(void);
  void close(void);

  inline bool isShaded(void) const
  { return client.state.shaded; }
  void setShaded(bool shaded);

  inline bool isIconic(void) const
  { return client.state.iconic; }
  void iconify(void); // call show() to deiconify

  inline bool isMaximized(void) const
  { return client.state.maximized; }
  // ### change to setMaximized()
  void maximize(unsigned int button);
  void remaximize(void);

  inline bool isFullScreen(void) const
  { return client.state.fullscreen; }
  void setFullScreen(bool);

  void reconfigure(void);
  void grabButtons(void);
  void ungrabButtons(void);
  void restore(bool remap);
  void configure(int dx, int dy, unsigned int dw, unsigned int dh);
  inline void configure(const bt::Rect &r)
  { configure(r.x(), r.y(), r.width(), r.height()); }

  void clientMessageEvent(const XClientMessageEvent * const ce);
  void buttonPressEvent(const XButtonEvent * const be);
  void buttonReleaseEvent(const XButtonEvent * const re);
  void motionNotifyEvent(const XMotionEvent * const me);
  void destroyNotifyEvent(const XDestroyWindowEvent * const /*unused*/);
  void unmapNotifyEvent(const XUnmapEvent * const /*unused*/);
  void reparentNotifyEvent(const XReparentEvent * const /*unused*/);
  void propertyNotifyEvent(const XPropertyEvent * const pe);
  void exposeEvent(const XExposeEvent * const ee);
  void configureRequestEvent(const XConfigureRequestEvent * const cr);
  void enterNotifyEvent(const XCrossingEvent * const ce);
  void leaveNotifyEvent(const XCrossingEvent * const /*unused*/);

#ifdef SHAPE
  void configureShape(void);
  void shapeEvent(const XEvent * const /*unused*/);
#endif // SHAPE

  virtual void timeout(bt::Timer *);
};

#endif // __Window_hh
