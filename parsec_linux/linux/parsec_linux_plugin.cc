#include "include/parsec_linux/parsec_linux_plugin.h"
//#include "ext/equations-parser/parser/mpParser.h"
#include "mpParser.h"
#include "mpDefines.h"
//#include "ext/equations-parser/parser/mpDefines.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>

#include <cstring>
#include <string>
#include <iostream>
using namespace std;
using namespace mup;

#define PARSEC_LINUX_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), parsec_linux_plugin_get_type(), \
                              ParsecLinuxPlugin))

struct _ParsecLinuxPlugin {
  GObject parent_instance;
};

G_DEFINE_TYPE(ParsecLinuxPlugin, parsec_linux_plugin, g_object_get_type())

Value Calc(string input) {
    ParserX parser(pckALL_NON_COMPLEX);

    Value ans;
    parser.DefineVar(_T("ans"), Variable(&ans));

    try
    {
        parser.SetExpr(input);
        ans = parser.Eval();

        return ans;
    }
    catch(ParserError &e)
    {
        if (e.GetPos() != -1) {
            string_type error = "Error: ";
            error.append(e.GetMsg());
            return error;
        }
    }
    catch(std::runtime_error &)
    {
        string_type error = "Error: Runtime error";
        return error;
    }
    return ans;
}

// Called when a method call is received from Flutter.
static void parsec_linux_plugin_handle_method_call(
    ParsecLinuxPlugin* self,
    FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, "nativeEval") == 0) {
    struct utsname uname_data = {};
    uname(&uname_data);
    //g_autofree gchar *version = g_strdup_printf("Linux %s", uname_data.version);
    Value ans = Calc("5 * 5");
    g_autofree gchar *version = g_strdup_printf("Linux %s", ans.AsString().c_str());
    g_autoptr(FlValue) result = fl_value_new_string(version);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void parsec_linux_plugin_dispose(GObject* object) {
  G_OBJECT_CLASS(parsec_linux_plugin_parent_class)->dispose(object);
}

static void parsec_linux_plugin_class_init(ParsecLinuxPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = parsec_linux_plugin_dispose;
}

static void parsec_linux_plugin_init(ParsecLinuxPlugin* self) {}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  ParsecLinuxPlugin* plugin = PARSEC_LINUX_PLUGIN(user_data);
  parsec_linux_plugin_handle_method_call(plugin, method_call);
}

void parsec_linux_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  ParsecLinuxPlugin* plugin = PARSEC_LINUX_PLUGIN(
      g_object_new(parsec_linux_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "parsec_linux",
                            FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(channel, method_call_cb,
                                            g_object_ref(plugin),
                                            g_object_unref);

  g_object_unref(plugin);
}
