/* ASan/UBSan ownership harness for XSLT Apply stylesheet and the
   support/4DPlugin-JSON.cpp helpers. Each test runs in its own process:
   harness <test-name>. A test passes when it exits 0. */

#include "4DPlugin-XSLT.h"
#include "fake4d.h"

#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
    if (!cond) {
        fprintf(stderr, "CHECK FAILED: %s\n", what.c_str());
        failures++;
    }
}

void check_nothing_live(const char *when) {
    std::string live = fake_live_report();
    check(live == "none", std::string(when) + ": still live: " + live);
}

void init_plugin() {
    PackInitBlock init;
    memset(&init, 0, sizeof(init));
    init.fCall4Dex = fake_call4d;
    FourDPackex(kInitPlugin, &init, nullptr, nullptr);
}

/* Drives the real FourDPackex -> PluginMain -> XSLT_Apply_stylesheet path. */
std::string apply(const std::string &xml, const std::string &xsl, PA_ObjectRef options) {
    PA_Handle hxml = fake_new_handle(xml);
    PA_Handle hxsl = fake_new_handle(xsl);
    PA_ObjectRef opt = options;
    void *params[3] = {&hxml, &hxsl, &opt};
    PA_Handle result = nullptr;
    FourDPackex(1, params, nullptr, &result);
    std::string out = fake_handle_bytes(result);
    fake_dispose_handle(result);
    fake_dispose_handle(hxml);
    fake_dispose_handle(hxsl);
    return out;
}

/* >= 1024 bytes, so is_file_path() never consults 4D */
std::string big(const char *tag) { return std::string(tag) + std::string(1100, ' '); }

/* ---- LIVE defects ---- */

void defect_is_file_path_text_owner_leak() {
    fake_test_path_calls = 0;
    std::string out = apply("<doc/>", "<xsl/>", nullptr);
    check(fake_test_path_calls == 2, "Test path name consulted for both small blobs");
    check(out == "xml=16398;xsl=16398;params=", "transform output: " + out);
    check_nothing_live("after small-blob apply");
}

void defect_ob_get_n_text_option_leak() {
    PA_ObjectRef options = fake_new_object();
    fake_obj_set_text(options, u"xmlParserOption", u"4");
    fake_obj_set_text(options, u"xslParserOption", u"8");
    std::string out = apply(big("<doc/>"), big("<xsl/>"), options);
    check(out == "xml=16398;xsl=16398;params=xmlParserOption,4,xslParserOption,8", "transform output: " + out);
    fake_release_object(options);
    check_nothing_live("after text parser options");
}

/* ---- LIVE controls ---- */

void control_numeric_options_and_params() {
    PA_ObjectRef options = fake_new_object();
    fake_obj_set_real(options, u"xmlParserOption", 1);
    fake_obj_set_real(options, u"xslParserOption", 1024);
    fake_obj_set_text(options, u"str1", u"'stroke'");
    std::string out = apply(big("<doc/>"), big("<xsl/>"), options);
    check(out == "xml=16399;xsl=17422;params=str1,'stroke'", "transform output: " + out);
    /* the caller's object is borrowed: still alive and intact */
    std::u16string v;
    check(fake_obj_get_text(options, u"str1", &v) && v == u"'stroke'", "caller options intact");
    check(fake_live(kObject) == 1, "only the caller's options object is live");
    fake_release_object(options);
    check_nothing_live("after numeric options");
}

void control_parse_failure_returns_empty_blob() {
    PA_ObjectRef options = fake_new_object();
    std::string out = apply(big("BAD"), big("<xsl/>"), options);
    check(out.empty(), "empty result on parse failure");
    out = apply(big("<doc/>"), big("BAD"), options);
    check(out.empty(), "empty result on stylesheet failure");
    fake_release_object(options);
    check_nothing_live("after parse failures");
}

void control_ob_get_n_failed_get_not_cleared() {
    PA_ObjectRef obj = fake_new_object();
    fake_obj_set_real(obj, u"ERR", 3);
    check(ob_get_n(obj, L"ERR") == 0.0, "failed get yields 0");
    check(ob_get_n(obj, L"missing") == 0.0, "missing key yields 0");
    fake_release_object(obj);
    check_nothing_live("after failed get");
}

void control_ob_stringify() {
    PA_ObjectRef obj = fake_new_object();
    fake_obj_set_text(obj, u"a", u"b");
    CUTF8String json;
    ob_stringify(obj, &json);
    check(std::string((const char *)json.c_str()) == "{\"a\":\"b\"}", "stringify output");
    fake_release_object(obj);
    check_nothing_live("after stringify");
}

/* ---- DORMANT defects (helpers not called by the manifest command) ---- */

void dormant_ob_set_text_owner_leak() {
    PA_ObjectRef obj = fake_new_object();
    ob_set_s(obj, L"s1", "v1");
    ob_set_s(obj, "s2", "v2");
    ob_set_a(obj, L"s3", L"v3");
    CUTF16String u16 = (const PA_Unichar *)u"v4";
    ob_set_a(obj, L"s4", &u16);
    std::u16string v;
    check(fake_obj_get_text(obj, u"s1", &v) && v == u"v1", "s1");
    check(fake_obj_get_text(obj, u"s2", &v) && v == u"v2", "s2");
    check(fake_obj_get_text(obj, u"s3", &v) && v == u"v3", "s3");
    check(fake_obj_get_text(obj, u"s4", &v) && v == u"v4", "s4");
    fake_release_object(obj);
    check_nothing_live("after ob_set_s/ob_set_a");
}

void dormant_ob_set_p_picture_owner_leak() {
    PA_ObjectRef obj = fake_new_object();
    PA_Picture pict = PA_CreatePicture(nullptr, 0);
    ob_set_p(obj, L"p", pict); /* consumes pict, like ob_set_o/ob_set_c */
    check(fake_obj_has(obj, u"p"), "picture property set");
    fake_release_object(obj);
    check_nothing_live("after ob_set_p");
}

void dormant_ob_get_a_b_property_leak() {
    PA_ObjectRef obj = fake_new_object();
    fake_obj_set_text(obj, u"s", u"hello");
    CUTF8String s;
    check(ob_get_a(obj, L"s", &s) && std::string((const char *)s.c_str()) == "hello", "ob_get_a value");
    check(ob_get_b(obj, L"s") == false, "ob_get_b on text is false");
    fake_release_object(obj);
    check_nothing_live("after ob_get_a/ob_get_b");
}

void control_dormant_failed_get_not_cleared() {
    PA_ObjectRef obj = fake_new_object();
    fake_obj_set_real(obj, u"ERR", 1);
    CUTF8String s;
    ob_get_a(obj, L"ERR", &s);
    check(ob_get_b(obj, L"ERR") == false, "failed ob_get_b yields false");
    fake_release_object(obj);
    check_nothing_live("after failed dormant gets");
}

const std::vector<std::pair<const char *, std::function<void()>>> tests = {
    {"defect_is_file_path_text_owner_leak", defect_is_file_path_text_owner_leak},
    {"defect_ob_get_n_text_option_leak", defect_ob_get_n_text_option_leak},
    {"control_numeric_options_and_params", control_numeric_options_and_params},
    {"control_parse_failure_returns_empty_blob", control_parse_failure_returns_empty_blob},
    {"control_ob_get_n_failed_get_not_cleared", control_ob_get_n_failed_get_not_cleared},
    {"control_ob_stringify", control_ob_stringify},
    {"dormant_ob_set_text_owner_leak", dormant_ob_set_text_owner_leak},
    {"dormant_ob_set_p_picture_owner_leak", dormant_ob_set_p_picture_owner_leak},
    {"dormant_ob_get_a_b_property_leak", dormant_ob_get_a_b_property_leak},
    {"control_dormant_failed_get_not_cleared", control_dormant_failed_get_not_cleared},
};

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <test>|--list\n", argv[0]);
        return 2;
    }
    if (!strcmp(argv[1], "--list")) {
        for (auto &t : tests) printf("%s\n", t.first);
        return 0;
    }
    for (auto &t : tests) {
        if (!strcmp(argv[1], t.first)) {
            init_plugin();
            t.second();
            return failures ? 1 : 0;
        }
    }
    fprintf(stderr, "unknown test %s\n", argv[1]);
    return 2;
}
