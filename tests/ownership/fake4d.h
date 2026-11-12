/* Synthetic 4D engine (Call4D) for the ownership harness. */
#ifndef XSLT_OWNERSHIP_FAKE4D_H
#define XSLT_OWNERSHIP_FAKE4D_H

#include "4DPluginAPI.h"
#include "PrivateTypes.h"
#include <string>

enum FakeKind { kText, kPicture, kObject, kHandle, kDoc, kStyle, kCtxt, kOutBuf, kKindCount };

void fake_call4d(short selector, EngineBlock *eb);
size_t fake_live(FakeKind kind);
std::string fake_live_report();
[[noreturn]] void fake_violation(const std::string &what);

PA_ObjectRef fake_new_object();
void fake_release_object(PA_ObjectRef obj);
void fake_obj_set_text(PA_ObjectRef obj, const std::u16string &key, const std::u16string &value);
void fake_obj_set_real(PA_ObjectRef obj, const std::u16string &key, double value);
bool fake_obj_get_text(PA_ObjectRef obj, const std::u16string &key, std::u16string *value);
bool fake_obj_has(PA_ObjectRef obj, const std::u16string &key);

PA_Handle fake_new_handle(const std::string &bytes);
std::string fake_handle_bytes(PA_Handle h);
void fake_dispose_handle(PA_Handle h);

extern int fake_test_path_calls;

#endif
