/* Synthetic jsoncpp boundary for the ownership harness: just enough of the
   Json::Value / CharReader surface used by 4DPlugin-XSLT.cpp to read a flat
   object of string and number members. */
#ifndef XSLT_OWNERSHIP_JSON_STUB_H
#define XSLT_OWNERSHIP_JSON_STUB_H

#include <string>
#include <vector>
#include <cstdlib>

#define JSONCPP_STRING std::string

namespace Json {

class Value {
public:
    enum Kind { kNull, kString, kNumber, kObject };
    Kind kind = kNull;
    std::string str;
    double num = 0;
    std::vector<std::string> keys;
    std::vector<Value> values;

    Value() {}
    explicit Value(const std::string &s) : kind(kString), str(s) {}

    bool isObject() const { return kind == kObject; }
    bool isString() const { return kind == kString; }
    std::string asString() const { return str; }

    class const_iterator {
        const Value *parent_;
        size_t i_;
    public:
        const_iterator(const Value *p, size_t i) : parent_(p), i_(i) {}
        Value key() const { return Value(parent_->keys[i_]); }
        std::string name() const { return parent_->keys[i_]; }
        const Value *operator->() const { return &parent_->values[i_]; }
        const_iterator operator++(int) { const_iterator t = *this; ++i_; return t; }
        bool operator!=(const const_iterator &o) const { return i_ != o.i_; }
    };

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, keys.size()); }
};

class CharReader {
    static void ws(const char *&p, const char *e) { while (p < e && (*p == ' ' || *p == '\n')) ++p; }
    static bool str(const char *&p, const char *e, std::string *out) {
        if (p >= e || *p != '"') return false;
        ++p;
        while (p < e && *p != '"') {
            if (*p == '\\' && p + 1 < e) ++p;
            out->push_back(*p++);
        }
        if (p >= e) return false;
        ++p;
        return true;
    }
public:
    virtual ~CharReader() {}
    bool parse(const char *b, const char *e, Value *root, std::string *errs) {
        const char *p = b;
        ws(p, e);
        if (p >= e || *p != '{') { *errs = "not an object"; return false; }
        ++p;
        root->kind = Value::kObject;
        ws(p, e);
        if (p < e && *p == '}') return true;
        while (p < e) {
            ws(p, e);
            std::string k;
            if (!str(p, e, &k)) { *errs = "key"; return false; }
            ws(p, e);
            if (p >= e || *p != ':') { *errs = "colon"; return false; }
            ++p;
            ws(p, e);
            Value v;
            if (p < e && *p == '"') {
                v.kind = Value::kString;
                if (!str(p, e, &v.str)) { *errs = "string"; return false; }
            } else {
                char *end = nullptr;
                std::string rest(p, e);
                v.kind = Value::kNumber;
                v.num = strtod(rest.c_str(), &end);
                p += (end - rest.c_str());
            }
            root->keys.push_back(k);
            root->values.push_back(v);
            ws(p, e);
            if (p < e && *p == ',') { ++p; continue; }
            if (p < e && *p == '}') return true;
            *errs = "separator";
            return false;
        }
        return false;
    }
};

class CharReaderBuilder {
public:
    CharReader *newCharReader() const { return new CharReader(); }
};

}

#endif
