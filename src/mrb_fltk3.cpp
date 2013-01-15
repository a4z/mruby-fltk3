#include <memory.h>
#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <fltk3/DoubleWindow.h>
#include <fltk3/Window.h>
#include <fltk3/Box.h>
#include <fltk3/Browser.h>
#include <fltk3/SelectBrowser.h>
#include <fltk3/Button.h>
#include <fltk3/CheckButton.h>
#include <fltk3/RadioButton.h>
#include <fltk3/Input.h>
#include <fltk3/ValueOutput.h>
#include <fltk3/TextDisplay.h>
#include <fltk3/TextEditor.h>
#include <fltk3/message.h>
#include <fltk3/ask.h>
#include <fltk3/run.h>
#include <stdio.h>

#if 1
#define ARENA_SAVE \
  int ai = mrb_gc_arena_save(mrb); \
  if (ai == MRB_ARENA_SIZE) { \
    mrb_raise(mrb, E_RUNTIME_ERROR, "arena overflow"); \
  }
#define ARENA_RESTORE \
  mrb_gc_arena_restore(mrb, ai);
#else
#define ARENA_SAVE
#define ARENA_RESTORE
#endif

typedef struct {
  fltk3::Widget* w;
  mrb_value instance;
  mrb_state* mrb;
} mrb_fltk3_Widget_context;

typedef struct {
  fltk3::TextBuffer* tb;
  mrb_value instance;
  mrb_state* mrb;
} mrb_fltk3_TextBuffer_context;

static void
fltk3_Widget_free(mrb_state *mrb, void *p) {
  free(p);
}

static void
fltk3_TextBuffer_free(mrb_state *mrb, void *p) {
  free(p);
}

static const struct mrb_data_type fltk3_Widget_type = {
  "fltk3_widget", fltk3_Widget_free,
};

static const struct mrb_data_type fltk3_TextBuffer_type = {
  "fltk3_textbuffer", fltk3_TextBuffer_free,
};

static bool
arg_check(const char* t, int argc, mrb_value* argv)
{
  int n, l = strlen(t);
  if (l != argc) return false;
  for (n = 0; n < l; n++) {
    if (n >= argc) return false;
    if (t[n] == 'i' && mrb_type(argv[n]) != MRB_TT_FIXNUM) return false;
    if (t[n] == 'd' && mrb_type(argv[n]) != MRB_TT_FLOAT) return false;
    if (t[n] == 's' && mrb_type(argv[n]) != MRB_TT_STRING) return false;
    if (t[n] == 'b' && mrb_type(argv[n]) != MRB_TT_TRUE && mrb_type(argv[n]) != MRB_TT_FALSE) return false;
  }
  return true;
}

#define CONTEXT_SETUP(t) \
    mrb_value value_context; \
    mrb_fltk3_ ## t ## _context* context = NULL; \
    value_context = mrb_iv_get(mrb, self, mrb_intern(mrb, "context")); \
    Data_Get_Struct(mrb, value_context, &fltk3_ ## t ## _type, context);

/*********************************************************
 * FLTK3::Widget
 *********************************************************/
static mrb_value
mrb_fltk3_widget_box_get(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  struct RClass* _class_fltk3 = mrb_class_get(mrb, "FLTK3");
  struct RClass* _class_fltk3_Box = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(_class_fltk3), mrb_intern(mrb, "Box")));
  mrb_value args[1];
  args[0] = mrb_obj_value(
    Data_Wrap_Struct(mrb, mrb->object_class,
    &fltk3_Widget_type, (void*) context->w->box()));
  return mrb_class_new_instance(mrb, 1, args, _class_fltk3_Box);
}

static mrb_value
mrb_fltk3_widget_box_set(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  mrb_value box;
  mrb_get_args(mrb, "o", &box);
  mrb_value box_value_context;
  mrb_fltk3_Widget_context* box_context = NULL;
  box_value_context = mrb_iv_get(mrb, box, mrb_intern(mrb, "context"));
  Data_Get_Struct(mrb, box_value_context, &fltk3_Widget_type, box_context);
  context->w->box((fltk3::Box*) box_context->w);
  return mrb_nil_value();
}

static mrb_value
mrb_fltk3_widget_show(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  context->w->show();
  return mrb_nil_value();
}

static mrb_value
mrb_fltk3_widget_hide(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  context->w->hide();
  return mrb_nil_value();
}

static mrb_value
mrb_fltk3_widget_visible(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  return context->w->visible() ? mrb_true_value() : mrb_false_value();
}

static void
_mrb_fltk3_widget_callback(fltk3::Widget* w, void* v)
{
  mrb_value args[2];
  mrb_fltk3_Widget_context* context = (mrb_fltk3_Widget_context*) v;
  mrb_state* mrb = context->mrb;
  mrb_value proc = mrb_iv_get(mrb, context->instance, mrb_intern(mrb, "callback"));
  mrb_value value = mrb_iv_get(mrb, context->instance, mrb_intern(mrb, "value"));
  args[0] = context->instance;
  args[1] = value;
  mrb_yield_argv(mrb, proc, 2, args);
}

static mrb_value
mrb_fltk3_widget_callback(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  mrb_value b = mrb_nil_value();
  mrb_value v = mrb_nil_value();
  mrb_get_args(mrb, "&|o", &b, &v);
  if (!mrb_nil_p(b)) {
    mrb_iv_set(mrb, self, mrb_intern(mrb, "callback"), b);
    mrb_iv_set(mrb, self, mrb_intern(mrb, "value"), v);
    context->w->callback(_mrb_fltk3_widget_callback, context);
  }
  return mrb_nil_value();
}

static mrb_value
mrb_fltk3_window_show(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  ((fltk3::Window*) context->w)->show(0, NULL);
  return mrb_nil_value();
}

/*********************************************************
 * FLTK3::Input
 *********************************************************/
static mrb_value
mrb_fltk3_input_value_get(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  return mrb_str_new_cstr(mrb, ((fltk3::Input*) context->w)->value());
}

static mrb_value
mrb_fltk3_input_value_set(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  mrb_value text;
  mrb_get_args(mrb, "S", &text);
  ((fltk3::Input*) context->w)->value(RSTRING_PTR(text));
  return mrb_nil_value();
}

/*********************************************************
 * FLTK3::Group
 *********************************************************/
static mrb_value
mrb_fltk3_group_begin(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  mrb_value b = mrb_nil_value();
  mrb_get_args(mrb, "&", &b);
  if (!mrb_nil_p(b)) {
    mrb_value args[1];
    args[0] = self;
    ((fltk3::Group*)context->w)->begin();
    mrb_yield_argv(mrb, b, 1, args);
    ((fltk3::Group*)context->w)->end();
  } else
    ((fltk3::Group*)context->w)->begin();
  return mrb_nil_value();
}

static mrb_value
mrb_fltk3_group_end(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  ((fltk3::Group*)context->w)->end();
  return mrb_nil_value();
}

static mrb_value
mrb_fltk3_group_resizable_get(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  struct RClass* _class_fltk3 = mrb_class_get(mrb, "FLTK3");
  struct RClass* _class_fltk3_Widget = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(_class_fltk3), mrb_intern(mrb, "Widget")));
  mrb_value args[1];
  args[0] = mrb_obj_value(
    Data_Wrap_Struct(mrb, mrb->object_class,
    &fltk3_Widget_type, (void*) ((fltk3::Group*) context->w)->resizable()));
  return mrb_class_new_instance(mrb, 1, args, _class_fltk3_Widget);
}

static mrb_value
mrb_fltk3_group_resizable_set(mrb_state *mrb, mrb_value self)
{
  CONTEXT_SETUP(Widget);
  mrb_value arg;
  mrb_get_args(mrb, "o", &arg);
  mrb_value arg_value_context;
  mrb_fltk3_Widget_context* arg_context = NULL;
  arg_value_context = mrb_iv_get(mrb, arg, mrb_intern(mrb, "context"));
  Data_Get_Struct(mrb, arg_value_context, &fltk3_Widget_type, arg_context);
  ((fltk3::Group*) context->w)->resizable(arg_context->w);
  return mrb_nil_value();
}

/*********************************************************
 * FLTK3::*
 *********************************************************/
static mrb_value
mrb_fltk3_run(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(fltk3::run());
}

static mrb_value
mrb_fltk3_alert(mrb_state *mrb, mrb_value self)
{
  mrb_value s;
  mrb_get_args(mrb, "S", &s);
  fltk3::alert(RSTRING_PTR(s));
  return mrb_nil_value();
}

static mrb_value
mrb_fltk3_ask(mrb_state *mrb, mrb_value self)
{
  mrb_value s;
  mrb_get_args(mrb, "S", &s);
  return fltk3::ask(RSTRING_PTR(s)) ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_fltk3_set_fonts(mrb_state *mrb, mrb_value self)
{
  mrb_value s;
  mrb_get_args(mrb, "S", &s);
  return mrb_fixnum_value(fltk3::set_fonts(RSTRING_PTR(s)));
}

static mrb_value
mrb_fltk3_font_name(mrb_state *mrb, mrb_value self)
{
  mrb_value i;
  mrb_get_args(mrb, "i", &i);
  int font_type = 0;
  const char *name = fltk3::get_font_name((fltk3::Font) mrb_fixnum(i), &font_type);
  return name ? mrb_str_new_cstr(mrb, name) : mrb_nil_value();
}

#define DECLARE_WIDGET(x)                                                 \
static mrb_value                                                          \
mrb_fltk3_ ## x ## _init(mrb_state *mrb, mrb_value self)                  \
{                                                                         \
  mrb_value *argv;                                                        \
  int argc;                                                               \
  mrb_get_args(mrb, "*", &argv, &argc);                                   \
  mrb_fltk3_Widget_context* context =                                     \
    (mrb_fltk3_Widget_context*) malloc(sizeof(mrb_fltk3_Widget_context)); \
  if (!context) mrb_raise(mrb, E_RUNTIME_ERROR, "can't alloc memory");    \
  memset(context, 0, sizeof(mrb_fltk3_Widget_context));                   \
  context->instance = self;                                               \
  context->mrb = mrb;                                                     \
  if (arg_check("o", argc, argv)) {                                       \
    if (strcmp(mrb_obj_classname(mrb, argv[0]), "fltk3_widget"))          \
      mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");               \
    mrb_iv_set(mrb, self, mrb_intern(mrb, "context"), argv[0]);           \
    return self;                                                          \
  } else if (arg_check("iiii", argc, argv)) {                             \
    context->w = (fltk3::Widget*) new fltk3::x (                          \
      (int) mrb_fixnum(argv[0]),                                          \
      (int) mrb_fixnum(argv[1]),                                          \
      (int) mrb_fixnum(argv[2]),                                          \
      (int) mrb_fixnum(argv[3]));                                         \
  } else if (arg_check("iiiis", argc, argv)) {                            \
    context->w = (fltk3::Widget*) new fltk3::x (                          \
      (int) mrb_fixnum(argv[0]),                                          \
      (int) mrb_fixnum(argv[1]),                                          \
      (int) mrb_fixnum(argv[2]),                                          \
      (int) mrb_fixnum(argv[3]),                                          \
      RSTRING_PTR(argv[4]));                                              \
  } else {                                                                \
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");                 \
  }                                                                       \
  mrb_iv_set(mrb, self, mrb_intern(mrb, "context"), mrb_obj_value(        \
    Data_Wrap_Struct(mrb, mrb->object_class,                              \
    &fltk3_Widget_type, (void*) context)));                               \
  return self;                                                            \
}

#define DECLARE_WINDOW(x)                                                 \
static mrb_value                                                          \
mrb_fltk3_ ## x ## _init(mrb_state *mrb, mrb_value self)                  \
{                                                                         \
  mrb_value *argv;                                                        \
  int argc;                                                               \
  mrb_get_args(mrb, "*", &argv, &argc);                                   \
  mrb_fltk3_Widget_context* context =                                     \
    (mrb_fltk3_Widget_context*) malloc(sizeof(mrb_fltk3_Widget_context)); \
  if (!context) mrb_raise(mrb, E_RUNTIME_ERROR, "can't alloc memory");    \
  memset(context, 0, sizeof(mrb_fltk3_Widget_context));                   \
  context->instance = self;                                               \
  context->mrb = mrb;                                                     \
  if (arg_check("iis", argc, argv)) {                                     \
    context->w = (fltk3::Widget*) new fltk3::x (                          \
      (int) mrb_fixnum(argv[0]),                                          \
      (int) mrb_fixnum(argv[1]),                                          \
      RSTRING_PTR(argv[2]));                                              \
  } else if (arg_check("iiiis", argc, argv)) {                            \
    context->w = (fltk3::Widget*) new fltk3::x (                          \
      (int) mrb_fixnum(argv[0]),                                          \
      (int) mrb_fixnum(argv[1]),                                          \
      (int) mrb_fixnum(argv[2]),                                          \
      (int) mrb_fixnum(argv[3]),                                          \
      RSTRING_PTR(argv[4]));                                              \
  } else {                                                                \
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");                 \
  }                                                                       \
  mrb_iv_set(mrb, self, mrb_intern(mrb, "context"), mrb_obj_value(        \
    Data_Wrap_Struct(mrb, mrb->object_class,                              \
    &fltk3_Widget_type, (void*) context)));                               \
  return self;                                                            \
}

#define DECLARE_HIDDEN_WIDGET(x)                                          \
static mrb_value                                                          \
mrb_fltk3_ ## x ## _init(mrb_state *mrb, mrb_value self)                  \
{                                                                         \
  mrb_value arg = mrb_nil_value();                                        \
  mrb_get_args(mrb, "|o", &arg);                                          \
  if (!mrb_nil_p(arg)) {                                                  \
    if (strcmp(mrb_obj_classname(mrb, arg), "fltk3_widget"))              \
      mrb_raise(mrb, E_RUNTIME_ERROR, "can't alloc fltk3::" # x);         \
    mrb_iv_set(mrb, self, mrb_intern(mrb, "context"), arg);               \
  } else mrb_raise(mrb, E_RUNTIME_ERROR, "can't alloc fltk3::" # x);      \
  return self;                                                            \
}

#define DECLARE_BOX(x)                                                    \
static mrb_value                                                          \
mrb_fltk3_ ## x ## _init(mrb_state *mrb, mrb_value self)                  \
{                                                                         \
  mrb_value arg = mrb_nil_value();                                        \
  mrb_get_args(mrb, "|S", &arg);                                          \
  mrb_fltk3_Widget_context* context =                                     \
    (mrb_fltk3_Widget_context*) malloc(sizeof(mrb_fltk3_Widget_context)); \
  if (!context) mrb_raise(mrb, E_RUNTIME_ERROR, "can't alloc memory");    \
  memset(context, 0, sizeof(mrb_fltk3_Widget_context));                   \
  context->instance = self;                                               \
  context->mrb = mrb;                                                     \
  context->w = (fltk3::Widget*) new fltk3::x (                            \
    mrb_nil_p(arg) ? NULL : RSTRING_PTR(arg));                            \
  mrb_iv_set(mrb, self, mrb_intern(mrb, "context"), mrb_obj_value(        \
    Data_Wrap_Struct(mrb, mrb->object_class,                              \
    &fltk3_Widget_type, (void*) context)));                               \
  return self;                                                            \
}

DECLARE_WIDGET(Widget)
DECLARE_WINDOW(DoubleWindow)
DECLARE_WINDOW(Window)
DECLARE_WIDGET(Browser)
DECLARE_WIDGET(SelectBrowser)
DECLARE_WIDGET(Button)
DECLARE_WIDGET(ValueOutput)
DECLARE_WIDGET(Input)
DECLARE_WIDGET(CheckButton)
DECLARE_WIDGET(RadioButton)

DECLARE_HIDDEN_WIDGET(Group)
DECLARE_WIDGET(TextDisplay)
DECLARE_WIDGET(TextEditor)

DECLARE_HIDDEN_WIDGET(Box)
DECLARE_BOX(NoBox)
DECLARE_BOX(FlatBox)
DECLARE_BOX(UpBox)
DECLARE_BOX(DownBox)
DECLARE_BOX(ThinUpBox)
DECLARE_BOX(ThinDownBox)
DECLARE_BOX(EngravedBox)
DECLARE_BOX(EmbossedBox)
DECLARE_BOX(BorderBox)
DECLARE_BOX(ShadowBox)
DECLARE_BOX(RoundedBox)
DECLARE_BOX(RShadowBox)
DECLARE_BOX(RFlatBox)
DECLARE_BOX(RoundUpBox)
DECLARE_BOX(RoundDownBox)
DECLARE_BOX(DiamondUpBox)
DECLARE_BOX(DiamondDownBox)
DECLARE_BOX(OvalBox)
DECLARE_BOX(OShadowBox)
DECLARE_BOX(OFlatBox)
DECLARE_BOX(PlasticUpBox)
DECLARE_BOX(PlasticDownBox)
DECLARE_BOX(PlasticThinUpBox)
DECLARE_BOX(PlasticThinDownBox)
DECLARE_BOX(PlasticRoundUpBox)
DECLARE_BOX(PlasticRoundDownBox)
DECLARE_BOX(ClassicUpBox)
DECLARE_BOX(ClassicDownBox)
DECLARE_BOX(ClassicThinUpBox)
DECLARE_BOX(ClassicThinDownBox)
DECLARE_BOX(ClassicRoundUpBox)
DECLARE_BOX(ClassicRoundDownBox)
DECLARE_BOX(BorderFrame)
DECLARE_BOX(UpFrame)
DECLARE_BOX(DownFrame)
DECLARE_BOX(ThinUpFrame)
DECLARE_BOX(ThinDownFrame)
DECLARE_BOX(EngravedFrame)
DECLARE_BOX(EmbossedFrame)
DECLARE_BOX(ShadowFrame)
DECLARE_BOX(RoundedFrame)
DECLARE_BOX(OvalFrame)
DECLARE_BOX(PlasticUpFrame)
DECLARE_BOX(PlasticDownFrame)
DECLARE_BOX(ClassicUpFrame)
DECLARE_BOX(ClassicDownFrame)
DECLARE_BOX(ClassicThinUpFrame)
DECLARE_BOX(ClassicThinDownFrame)

extern "C"
{
#define INHERIT_GROUP(x) \
  mrb_define_method(mrb, _class_fltk3_ ## x, "begin", mrb_fltk3_group_begin, ARGS_NONE()); \
  mrb_define_method(mrb, _class_fltk3_ ## x, "end", mrb_fltk3_group_end, ARGS_NONE()); \
  mrb_define_method(mrb, _class_fltk3_ ## x, "resizable", mrb_fltk3_group_resizable_get, ARGS_NONE()); \
  mrb_define_method(mrb, _class_fltk3_ ## x, "resizable=", mrb_fltk3_group_resizable_set, ARGS_REQ(1)); \
  ARENA_RESTORE;

#define DEFINE_FIXNUM_PROP(x, y) \
  mrb_define_method(mrb, _class_fltk3_ ## x, # y, [] (mrb_state* mrb, mrb_value self) -> mrb_value { \
    CONTEXT_SETUP(Widget); \
    return mrb_fixnum_value(((fltk3::x*) context->w)->y()); \
  }, ARGS_NONE()); \
  mrb_define_method(mrb, _class_fltk3_ ## x, # y "=", [] (mrb_state* mrb, mrb_value self) -> mrb_value { \
    CONTEXT_SETUP(Widget); \
    mrb_value vv; \
    mrb_get_args(mrb, "i", &vv); \
    ((fltk3::x*) context->w)->y(mrb_fixnum(vv)); \
    return mrb_nil_value(); \
  }, ARGS_NONE()); \
  ARENA_RESTORE;

#define DEFINE_STR_PROP(x, y) \
  mrb_define_method(mrb, _class_fltk3_ ## x, # y, [] (mrb_state* mrb, mrb_value self) -> mrb_value { \
    CONTEXT_SETUP(Widget); \
    const char* vv = ((fltk3::x*) context->w)->y(); \
    return vv ? mrb_str_new_cstr(mrb, vv) : mrb_nil_value(); \
  }, ARGS_NONE()); \
  mrb_define_method(mrb, _class_fltk3_ ## x, # y "=", [] (mrb_state* mrb, mrb_value self) -> mrb_value { \
    CONTEXT_SETUP(Widget); \
    mrb_value vv; \
    mrb_get_args(mrb, "S", &vv); \
    ((fltk3::x*) context->w)->y(RSTRING_PTR(vv)); \
    return mrb_nil_value(); \
  }, ARGS_NONE()); \
  ARENA_RESTORE;

#define INHERIT_INPUT_VALUE(x) \
  mrb_define_method(mrb, _class_fltk3_ ## x, "value", mrb_fltk3_input_value_get, ARGS_NONE()); \
  mrb_define_method(mrb, _class_fltk3_ ## x, "value=", mrb_fltk3_input_value_set, ARGS_NONE()); \
  ARENA_RESTORE;

#define DEFINE_WIDGET(x, y) \
  struct RClass* _class_fltk3_ ## x = mrb_define_class_under(mrb, _class_fltk3, # x, _class_fltk3_ ## y); \
  mrb_define_method(mrb, _class_fltk3_ ## x, "initialize", mrb_fltk3_ ## x ## _init, ARGS_ANY()); \
  ARENA_RESTORE;

void
mrb_mruby_fltk3_gem_init(mrb_state* mrb)
{
  ARENA_SAVE;
  struct RClass* _class_fltk3 = mrb_define_module(mrb, "FLTK3");
  mrb_define_module_function(mrb, _class_fltk3, "run", mrb_fltk3_run, ARGS_NONE());
  mrb_define_module_function(mrb, _class_fltk3, "alert", mrb_fltk3_alert, ARGS_REQ(1));
  mrb_define_module_function(mrb, _class_fltk3, "ask", mrb_fltk3_ask, ARGS_REQ(1));
  mrb_define_module_function(mrb, _class_fltk3, "set_fonts", mrb_fltk3_set_fonts, ARGS_REQ(1));
  mrb_define_module_function(mrb, _class_fltk3, "font_name", mrb_fltk3_font_name, ARGS_REQ(1));
  ARENA_RESTORE;

  struct RClass* _class_fltk3_Widget = mrb_define_class_under(mrb, _class_fltk3, "Widget", mrb->object_class);
  mrb_define_method(mrb, _class_fltk3_Widget, "initialize", mrb_fltk3_Widget_init, ARGS_ANY());
  mrb_define_method(mrb, _class_fltk3_Widget, "show", mrb_fltk3_widget_show, ARGS_NONE());
  mrb_define_method(mrb, _class_fltk3_Widget, "hide", mrb_fltk3_widget_hide, ARGS_NONE());
  DEFINE_FIXNUM_PROP(Widget, x);
  DEFINE_FIXNUM_PROP(Widget, y);
  DEFINE_FIXNUM_PROP(Widget, w);
  DEFINE_FIXNUM_PROP(Widget, h);
  DEFINE_FIXNUM_PROP(Widget, labelfont);
  DEFINE_FIXNUM_PROP(Widget, labelsize);
  DEFINE_STR_PROP(Widget, label);
  mrb_define_method(mrb, _class_fltk3_Widget, "box", mrb_fltk3_widget_box_get, ARGS_NONE());
  mrb_define_method(mrb, _class_fltk3_Widget, "box=", mrb_fltk3_widget_box_set, ARGS_REQ(1));
  mrb_define_method(mrb, _class_fltk3_Widget, "visible", mrb_fltk3_widget_visible, ARGS_NONE());
  mrb_define_method(mrb, _class_fltk3_Widget, "callback", mrb_fltk3_widget_callback, ARGS_OPT(1));
  ARENA_RESTORE;

  DEFINE_WIDGET(Button, Widget);
  DEFINE_WIDGET(ValueOutput, Widget);
  DEFINE_WIDGET(CheckButton, Widget);
  DEFINE_WIDGET(RadioButton, Widget);

  DEFINE_WIDGET(Input, Widget);
  INHERIT_INPUT_VALUE(Input);

  DEFINE_WIDGET(Browser, Widget);
  INHERIT_INPUT_VALUE(Input);

  mrb_define_method(mrb, _class_fltk3_Browser, "load", [] (mrb_state* mrb, mrb_value self) -> mrb_value {
    CONTEXT_SETUP(Widget);
    mrb_value filename;
    mrb_get_args(mrb, "S", &filename);
    return mrb_fixnum_value(((fltk3::Browser*) context->w)->load(RSTRING_PTR(filename)));
  }, ARGS_REQ(1));
  DEFINE_FIXNUM_PROP(Browser, value);
  mrb_define_method(mrb, _class_fltk3_Browser, "text", [] (mrb_state* mrb, mrb_value self) -> mrb_value {
    CONTEXT_SETUP(Widget);
    mrb_value line = mrb_nil_value(), text = mrb_nil_value();
    mrb_get_args(mrb, "|i|S", &line, &text);
    if (mrb_nil_p(text)) {
      int value = ((fltk3::Browser*) context->w)->value();
      const char* text = ((fltk3::Browser*) context->w)->text(value);
      return text ? mrb_str_new_cstr(mrb, text) : mrb_nil_value();
    } else if (mrb_nil_p(text)) {
      const char* text = ((fltk3::Browser*) context->w)->text(mrb_fixnum(line));
      return text ? mrb_str_new_cstr(mrb, text) : mrb_nil_value();
    }
    ((fltk3::Browser*) context->w)->text(mrb_fixnum(line), RSTRING_PTR(text));
    return mrb_nil_value();
  }, ARGS_REQ(1) | ARGS_OPT(1));
  DEFINE_WIDGET(SelectBrowser, Browser);

  struct RClass* _class_fltk3_Group = mrb_define_class_under(mrb, _class_fltk3, "Group", _class_fltk3_Widget);
  mrb_define_method(mrb, _class_fltk3_Group, "initialize", mrb_fltk3_Group_init, ARGS_ANY());
  INHERIT_GROUP(Group);

  DEFINE_WIDGET(TextDisplay, Group);
  DEFINE_WIDGET(TextEditor, TextDisplay);

  struct RClass* _class_fltk3_Window = mrb_define_class_under(mrb, _class_fltk3, "Window", _class_fltk3_Widget);
  mrb_define_method(mrb, _class_fltk3_Window, "initialize", mrb_fltk3_Window_init, ARGS_ANY());
  mrb_define_method(mrb, _class_fltk3_Window, "show", mrb_fltk3_window_show, ARGS_OPT(1));
  INHERIT_GROUP(Window);

  DEFINE_WIDGET(DoubleWindow, Window);

  DEFINE_WIDGET(Box, Widget);
  DEFINE_WIDGET(NoBox, Box);
  DEFINE_WIDGET(FlatBox, Box);
  DEFINE_WIDGET(UpBox, Box);
  DEFINE_WIDGET(DownBox, Box);
  DEFINE_WIDGET(ThinUpBox, Box);
  DEFINE_WIDGET(ThinDownBox, Box);
  DEFINE_WIDGET(EngravedBox, Box);
  DEFINE_WIDGET(EmbossedBox, Box);
  DEFINE_WIDGET(BorderBox, Box);
  DEFINE_WIDGET(ShadowBox, Box);
  DEFINE_WIDGET(RoundedBox, Box);
  DEFINE_WIDGET(RShadowBox, Box);
  DEFINE_WIDGET(RFlatBox, Box);
  DEFINE_WIDGET(RoundUpBox, Box);
  DEFINE_WIDGET(RoundDownBox, Box);
  DEFINE_WIDGET(DiamondUpBox, Box);
  DEFINE_WIDGET(DiamondDownBox, Box);
  DEFINE_WIDGET(OvalBox, Box);
  DEFINE_WIDGET(OShadowBox, Box);
  DEFINE_WIDGET(OFlatBox, Box);
  DEFINE_WIDGET(PlasticUpBox, Box);
  DEFINE_WIDGET(PlasticDownBox, Box);
  DEFINE_WIDGET(PlasticThinUpBox, Box);
  DEFINE_WIDGET(PlasticThinDownBox, Box);
  DEFINE_WIDGET(PlasticRoundUpBox, Box);
  DEFINE_WIDGET(PlasticRoundDownBox, Box);
  DEFINE_WIDGET(ClassicUpBox, Box);
  DEFINE_WIDGET(ClassicDownBox, Box);
  DEFINE_WIDGET(ClassicThinUpBox, Box);
  DEFINE_WIDGET(ClassicThinDownBox, Box);
  DEFINE_WIDGET(ClassicRoundUpBox, Box);
  DEFINE_WIDGET(ClassicRoundDownBox, Box);
  DEFINE_WIDGET(BorderFrame, Box);
  DEFINE_WIDGET(UpFrame, Box);
  DEFINE_WIDGET(DownFrame, Box);
  DEFINE_WIDGET(ThinUpFrame, Box);
  DEFINE_WIDGET(ThinDownFrame, Box);
  DEFINE_WIDGET(EngravedFrame, Box);
  DEFINE_WIDGET(EmbossedFrame, Box);
  DEFINE_WIDGET(ShadowFrame, Box);
  DEFINE_WIDGET(RoundedFrame, Box);
  DEFINE_WIDGET(OvalFrame, Box);
  DEFINE_WIDGET(PlasticUpFrame, Box);
  DEFINE_WIDGET(PlasticDownFrame, Box);
  DEFINE_WIDGET(ClassicUpFrame, Box);
  DEFINE_WIDGET(ClassicDownFrame, Box);
  DEFINE_WIDGET(ClassicThinUpFrame, Box);
  DEFINE_WIDGET(ClassicThinDownFrame, Box);

  struct RClass* _class_fltk3_TextBuffer = mrb_define_class_under(mrb, _class_fltk3, "TextBuffer", mrb->object_class);
  mrb_define_method(mrb, _class_fltk3_TextBuffer, "initialize", [] (mrb_state* mrb, mrb_value self) -> mrb_value {
    mrb_fltk3_TextBuffer_context* context =
      (mrb_fltk3_TextBuffer_context*) malloc(sizeof(mrb_fltk3_TextBuffer_context));
    if (!context) mrb_raise(mrb, E_RUNTIME_ERROR, "can't alloc memory");
    memset(context, 0, sizeof(mrb_fltk3_TextBuffer_context));
    context->instance = self;
    context->tb = new fltk3::TextBuffer;
    mrb_iv_set(mrb, self, mrb_intern(mrb, "context"), mrb_obj_value(
      Data_Wrap_Struct(mrb, mrb->object_class,
      &fltk3_TextBuffer_type, (void*) context)));
  }, ARGS_ANY());

  mrb_define_method(mrb, _class_fltk3_TextDisplay, "buffer", [] (mrb_state* mrb, mrb_value self) -> mrb_value {
    CONTEXT_SETUP(Widget);
    struct RClass* _class_fltk3 = mrb_class_get(mrb, "FLTK3");
    struct RClass* _class_fltk3_TextBuffer = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(_class_fltk3), mrb_intern(mrb, "TextBuffer")));
    mrb_value args[1];
    args[0] = mrb_obj_value(
      Data_Wrap_Struct(mrb, mrb->object_class,
      &fltk3_TextBuffer_type, (void*) ((fltk3::TextDisplay*) context->w)->buffer()));
    return mrb_class_new_instance(mrb, 1, args, _class_fltk3_TextBuffer);
  }, ARGS_NONE());
  mrb_define_method(mrb, _class_fltk3_TextDisplay, "buffer=", [] (mrb_state* mrb, mrb_value self) -> mrb_value {
    CONTEXT_SETUP(Widget);
    struct RClass* _class_fltk3 = mrb_class_get(mrb, "FLTK3");
    struct RClass* _class_fltk3_TextBuffer = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(_class_fltk3), mrb_intern(mrb, "TextBuffer")));
    mrb_value textbuffer;
    mrb_get_args(mrb, "o", &textbuffer);
    mrb_value textbuffer_value_context;
    mrb_fltk3_TextBuffer_context* textbuffer_context = NULL;
    textbuffer_value_context = mrb_iv_get(mrb, textbuffer, mrb_intern(mrb, "context"));
    Data_Get_Struct(mrb, textbuffer_value_context, &fltk3_TextBuffer_type, textbuffer_context);
    ((fltk3::TextDisplay*) context->w)->buffer(textbuffer_context->tb);
    return mrb_nil_value();
  }, ARGS_REQ(1));
  ARENA_RESTORE;
}

}

/* vim:set et ts=2 sts=2 sw=2 tw=0: */
