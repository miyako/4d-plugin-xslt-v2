/* Synthetic 4D engine (Call4D) and libxml2/libxslt boundary.

   Every engine-owned resource (text, picture, object, handle) and every
   stub library object is registered on allocation and unregistered on
   release. Releasing something that is not registered is reported as an
   ownership violation, and each test asserts that nothing is left live. */

#include "fake4d.h"
#include "xml_stub.h"
#include "EntryPoints.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

namespace {

std::map<const void *, FakeKind> &registry() {
    static std::map<const void *, FakeKind> r;
    return r;
}

const char *kind_name(FakeKind k) {
    static const char *names[] = {"text", "picture", "object", "handle", "xmlDoc", "stylesheet", "transformCtxt", "outputBuffer"};
    return names[k];
}

void track(const void *p, FakeKind k) {
    if (!p) fake_violation("null allocation");
    registry()[p] = k;
}

void untrack(const void *p, FakeKind k) {
    auto it = registry().find(p);
    if (it == registry().end() || it->second != k)
        fake_violation(std::string("release of unowned/freed ") + kind_name(k));
    registry().erase(it);
}

struct FakeObj {
    std::vector<std::pair<std::u16string, PA_Variable>> props;
};

struct FakeHandle {
    char *data;
    size_t size;
};

const PA_Unichar kPoison[] = {'P', 'O', 'I', 'S', 'O', 'N', 0};

PA_Unistring make_text(const PA_Unichar *src, size_t len) {
    PA_Unichar *buf = new PA_Unichar[len + 1];
    if (len) memcpy(buf, src, len * sizeof(PA_Unichar));
    buf[len] = 0;
    track(buf, kText);
    PA_Unistring u;
    u.fLength = (PA_long32)len;
    u.fString = buf;
    u.fReserved1 = 0;
    u.fReserved2 = 0;
    return u;
}

void free_text(PA_Unistring *u) {
    if (!u->fString) return;
    untrack(u->fString, kText);
    delete[] u->fString;
    u->fString = nullptr;
    u->fLength = 0;
}

PA_Picture make_picture() {
    char *p = new char[1];
    track(p, kPicture);
    return p;
}

void free_picture(PA_Picture p) {
    if (!p) return;
    untrack(p, kPicture);
    delete[] (char *)p;
}

void clear_var(PA_Variable *v);
PA_ObjectRef copy_obj(PA_ObjectRef src);

PA_Variable copy_var(const PA_Variable &src) {
    PA_Variable v;
    memset(&v, 0, sizeof(v));
    v.fType = src.fType;
    switch (src.fType) {
        case eVK_Unistring:
            v.uValue.fString = make_text(src.uValue.fString.fString, src.uValue.fString.fLength);
            break;
        case eVK_Object:
            v.uValue.fObject = copy_obj(src.uValue.fObject);
            break;
        case eVK_Picture:
            v.uValue.fPicture = src.uValue.fPicture ? make_picture() : nullptr;
            break;
        case eVK_Real:
        case eVK_Longint:
        case eVK_Boolean:
        case eVK_Undefined:
            v.uValue = src.uValue;
            break;
        default:
            fake_violation("copy of unsupported variable kind " + std::to_string((int)src.fType));
    }
    return v;
}

PA_ObjectRef copy_obj(PA_ObjectRef src) {
    if (!src) return nullptr;
    auto it = registry().find(src);
    if (it == registry().end() || it->second != kObject) fake_violation("use of freed object");
    FakeObj *o = new FakeObj();
    track(o, kObject);
    for (auto &p : ((FakeObj *)src)->props) o->props.push_back({p.first, copy_var(p.second)});
    return o;
}

void clear_var(PA_Variable *v) {
    switch (v->fType) {
        case eVK_Unistring: free_text(&v->uValue.fString); break;
        case eVK_Object: fake_release_object(v->uValue.fObject); break;
        case eVK_Picture: free_picture(v->uValue.fPicture); break;
        case eVK_Blob: fake_dispose_handle(v->uValue.fBlob.fHandle); break;
        case eVK_Real:
        case eVK_Longint:
        case eVK_Boolean:
        case eVK_Undefined:
            break;
        default:
            fake_violation("clear of unsupported variable kind " + std::to_string((int)v->fType));
    }
    v->fType = eVK_Undefined;
}

std::u16string key_of(const PA_Unistring &u) {
    return std::u16string((const char16_t *)u.fString, (size_t)u.fLength);
}

PA_Variable *find_prop(PA_ObjectRef obj, const std::u16string &key) {
    if (!obj) fake_violation("null object");
    auto it = registry().find(obj);
    if (it == registry().end() || it->second != kObject) fake_violation("use of freed object");
    for (auto &p : ((FakeObj *)obj)->props)
        if (p.first == key) return &p.second;
    return nullptr;
}

void set_prop(PA_ObjectRef obj, const std::u16string &key, const PA_Variable &value) {
    PA_Variable copy = copy_var(value);
    PA_Variable *slot = find_prop(obj, key);
    if (slot) {
        clear_var(slot);
        *slot = copy;
    } else {
        ((FakeObj *)obj)->props.push_back({key, copy});
    }
}

std::string ascii(const std::u16string &s) {
    std::string r;
    for (char16_t c : s) r.push_back((char)c);
    return r;
}

std::u16string widen(const std::string &s) {
    std::u16string r;
    for (char c : s) r.push_back((char16_t)(unsigned char)c);
    return r;
}

std::string stringify(PA_ObjectRef obj) {
    std::string out = "{";
    bool first = true;
    for (auto &p : ((FakeObj *)obj)->props) {
        std::string value;
        if (p.second.fType == eVK_Unistring) {
            value = "\"";
            for (char c : ascii(key_of(p.second.uValue.fString))) {
                if (c == '"' || c == '\\') value.push_back('\\');
                value.push_back(c);
            }
            value += "\"";
        } else if (p.second.fType == eVK_Real) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.17g", p.second.uValue.fReal);
            value = buf;
        } else {
            continue;
        }
        if (!first) out += ",";
        first = false;
        out += "\"" + ascii(p.first) + "\":" + value;
    }
    return out + "}";
}

void execute_command(EngineBlock *eb) {
    PA_Variable **args = (PA_Variable **)eb->fPtr1;
    PA_Variable *result = (PA_Variable *)eb->fPtr2;
    sLONG_PTR id = eb->fParam1;
    switch (id) {
        case 476: /* Test path name */
            fake_test_path_calls++;
            if (args[0]->fType != eVK_Unistring) fake_violation("Test path name: argument is not text");
            result->fType = eVK_Longint;
            result->uValue.fLongint = -43;
            break;
        case 1217: /* JSON Stringify */ {
            if (args[0]->fType != eVK_Object) fake_violation("JSON Stringify: argument is not an object");
            std::u16string json = widen(stringify(args[0]->uValue.fObject));
            result->fType = eVK_Unistring;
            result->uValue.fString = make_text((const PA_Unichar *)json.c_str(), json.size());
            break;
        }
        case 1225: /* OB Copy */
            result->fType = eVK_Object;
            result->uValue.fObject = copy_obj(args[0]->uValue.fObject);
            break;
        case 1231: /* OB Is defined */
            result->fType = eVK_Boolean;
            result->uValue.fBoolean = find_prop(args[0]->uValue.fObject, key_of(args[1]->uValue.fString)) != nullptr;
            break;
        default:
            fake_violation("unexpected command id " + std::to_string((long)id));
    }
}

void handle_manager(EngineBlock *eb) {
    switch (eb->fParam1) {
        case 1: {
            FakeHandle *h = new FakeHandle();
            h->size = (size_t)eb->fParam2;
            h->data = (char *)calloc(h->size ? h->size : 1, 1);
            track(h, kHandle);
            eb->fHandle = (PA_Handle)h;
            break;
        }
        case 2: fake_dispose_handle(eb->fHandle); break;
        case 4: eb->fParam2 = (sLONG_PTR)((FakeHandle *)eb->fHandle)->size; break;
        case 5: eb->fParam3 = (sLONG_PTR)((FakeHandle *)eb->fHandle)->data; break;
        case 6: break;
        default: fake_violation("unexpected handle manager op " + std::to_string((long)eb->fParam1));
    }
}

void convert_string(EngineBlock *eb) {
    if (eb->fParam1 == 5) { /* FromUserData */
        eb->fHandle = fake_new_handle(std::string((const char *)eb->fParam3, (size_t)eb->fParam4));
    } else if (eb->fParam1 == 4) { /* ToUserData */
        FakeHandle *h = (FakeHandle *)eb->fHandle;
        if (eb->fParam3) memcpy((void *)eb->fParam3, h->data, h->size);
        eb->fParam4 = (sLONG_PTR)h->size;
    } else {
        fake_violation("unexpected convert string op");
    }
}

} // namespace

int fake_test_path_calls = 0;

void fake_violation(const std::string &what) {
    fprintf(stderr, "OWNERSHIP VIOLATION: %s\n", what.c_str());
    fflush(stderr);
    abort();
}

size_t fake_live(FakeKind kind) {
    size_t n = 0;
    for (auto &e : registry())
        if (e.second == kind) n++;
    return n;
}

std::string fake_live_report() {
    std::string r;
    for (int k = 0; k < kKindCount; k++) {
        size_t n = fake_live((FakeKind)k);
        if (n) r += std::string(r.empty() ? "" : ", ") + std::to_string(n) + " " + kind_name((FakeKind)k);
    }
    return r.empty() ? "none" : r;
}

PA_ObjectRef fake_new_object() {
    FakeObj *o = new FakeObj();
    track(o, kObject);
    return o;
}

void fake_release_object(PA_ObjectRef obj) {
    if (!obj) return;
    untrack(obj, kObject);
    FakeObj *o = (FakeObj *)obj;
    for (auto &p : o->props) clear_var(&p.second);
    delete o;
}

void fake_obj_set_text(PA_ObjectRef obj, const std::u16string &key, const std::u16string &value) {
    PA_Variable v;
    memset(&v, 0, sizeof(v));
    v.fType = eVK_Unistring;
    v.uValue.fString.fString = (PA_Unichar *)value.c_str();
    v.uValue.fString.fLength = (PA_long32)value.size();
    set_prop(obj, key, v);
}

void fake_obj_set_real(PA_ObjectRef obj, const std::u16string &key, double value) {
    PA_Variable v;
    memset(&v, 0, sizeof(v));
    v.fType = eVK_Real;
    v.uValue.fReal = value;
    set_prop(obj, key, v);
}

bool fake_obj_get_text(PA_ObjectRef obj, const std::u16string &key, std::u16string *value) {
    PA_Variable *v = find_prop(obj, key);
    if (!v || v->fType != eVK_Unistring) return false;
    *value = key_of(v->uValue.fString);
    return true;
}

bool fake_obj_has(PA_ObjectRef obj, const std::u16string &key) {
    return find_prop(obj, key) != nullptr;
}

PA_Handle fake_new_handle(const std::string &bytes) {
    FakeHandle *h = new FakeHandle();
    h->size = bytes.size();
    h->data = (char *)malloc(h->size ? h->size : 1);
    if (h->size) memcpy(h->data, bytes.data(), h->size);
    track(h, kHandle);
    return (PA_Handle)h;
}

std::string fake_handle_bytes(PA_Handle h) {
    if (!h) return std::string();
    FakeHandle *fh = (FakeHandle *)h;
    return std::string(fh->data, fh->size);
}

void fake_dispose_handle(PA_Handle h) {
    if (!h) return;
    untrack(h, kHandle);
    FakeHandle *fh = (FakeHandle *)h;
    free(fh->data);
    delete fh;
}

void fake_call4d(short selector, EngineBlock *eb) {
    switch (selector) {
        case EX_CREATE_UNISTRING: {
            const PA_Unichar *src = (const PA_Unichar *)eb->fHandle;
            size_t len = 0;
            while (src[len]) len++;
            eb->fUniString1 = make_text(src, len);
            break;
        }
        case EX_DISPOSE_UNISTRING: free_text(&eb->fUniString1); break;
        case EX_VARIABLE_TO_STRING: {
            PA_Variable *v = (PA_Variable *)eb->fHandle;
            if (v->fType == eVK_Unistring) eb->fUniString1 = v->uValue.fString; /* borrowed */
            break;
        }
        case EX_CLEAR_VARIABLE: clear_var((PA_Variable *)eb->fPtr1); break;
        case EX_CREATE_PICTURE: eb->fPicture = make_picture(); break;
        case EX_DISPOSE_PICTURE: free_picture(eb->fPicture); break;
        case EX_HANDLE_MANAGER: handle_manager(eb); break;
        case EX_CONVERT_STRING: convert_string(eb); break;
        case EX_EXECUTE_COMMAND_BY_ID: execute_command(eb); break;
        case EX_GET_OBJ_VALUE: {
            PA_Variable *out = (PA_Variable *)eb->fPtr2;
            std::u16string key = key_of(eb->fUniString1);
            if (key == u"ERR") {
                /* failed call: the out variable is garbage and must not be cleared */
                eb->fError = -1;
                out->fType = eVK_Unistring;
                out->uValue.fString.fString = (PA_Unichar *)kPoison;
                out->uValue.fString.fLength = 6;
                break;
            }
            PA_Variable *v = find_prop(eb->fPtr1, key);
            if (v) {
                *out = copy_var(*v); /* owned by the caller */
            } else {
                out->fType = eVK_Undefined;
            }
            break;
        }
        case EX_SET_OBJ_VALUE:
            set_prop(eb->fPtr1, key_of(eb->fUniString1), *(PA_Variable *)eb->fPtr2); /* deep copy */
            break;
        default:
            fake_violation("unexpected Call4D selector " + std::to_string(selector));
    }
}

/* ---- synthetic libxml2 / libxslt ---- */

struct StubDoc { std::string content; int flags; };
struct StubStylesheet { xmlDocPtr doc; };
struct StubTransformContext { int unused; };
struct StubOutputBuffer { std::string content; };

void xmlInitParser(void) {}
void xmlCleanupParser(void) {}
void exsltRegisterAll(void) {}
void xsltCleanupGlobals(void) {}

xmlDocPtr xmlParseFile(const char *) { return nullptr; }

xmlDocPtr xmlParseMemory(const char *buffer, int size) {
    std::string s(buffer ? buffer : "", buffer ? (size_t)size : 0);
    if (s.compare(0, 3, "BAD") == 0) return nullptr;
    StubDoc *d = new StubDoc{s, 0};
    track(d, kDoc);
    return d;
}

int xmlXIncludeProcessFlags(xmlDocPtr doc, int flags) { doc->flags = flags; return 0; }

void xmlFreeDoc(xmlDocPtr doc) {
    if (!doc) return;
    untrack(doc, kDoc);
    delete doc;
}

xsltStylesheetPtr xsltParseStylesheetDoc(xmlDocPtr doc) {
    StubStylesheet *s = new StubStylesheet{doc};
    track(s, kStyle);
    return s;
}

void xsltFreeStylesheet(xsltStylesheetPtr style) {
    untrack(style, kStyle);
    xmlFreeDoc(style->doc);
    delete style;
}

xsltTransformContextPtr xsltNewTransformContext(xsltStylesheetPtr, xmlDocPtr) {
    StubTransformContext *c = new StubTransformContext{0};
    track(c, kCtxt);
    return c;
}

void xsltFreeTransformContext(xsltTransformContextPtr ctxt) {
    untrack(ctxt, kCtxt);
    delete ctxt;
}

xmlDocPtr xsltApplyStylesheetUser(xsltStylesheetPtr style, xmlDocPtr doc, const char **params,
                                  const char *, FILE *, xsltTransformContextPtr) {
    std::string out = "xml=" + std::to_string(doc->flags) + ";xsl=" + std::to_string(style->doc->flags) + ";params=";
    for (size_t i = 0; params && params[i]; i++) out += std::string(i ? "," : "") + params[i];
    StubDoc *d = new StubDoc{out, 0};
    track(d, kDoc);
    return d;
}

xmlOutputBufferPtr xmlAllocOutputBuffer(xmlCharEncodingHandlerPtr) {
    StubOutputBuffer *b = new StubOutputBuffer();
    track(b, kOutBuf);
    return b;
}

int xsltSaveResultTo(xmlOutputBufferPtr buf, xmlDocPtr result, xsltStylesheetPtr) {
    buf->content = result->content;
    return 0;
}

size_t xmlOutputBufferGetSize(xmlOutputBufferPtr out) { return out->content.size(); }
const xmlChar *xmlOutputBufferGetContent(xmlOutputBufferPtr out) { return (const xmlChar *)out->content.data(); }

int xmlOutputBufferClose(xmlOutputBufferPtr out) {
    untrack(out, kOutBuf);
    delete out;
    return 0;
}
