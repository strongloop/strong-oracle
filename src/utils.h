#ifndef _util_h_
#define _util_h_

#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include <v8.h>
#include "nan.h"

using namespace v8;

#ifdef _WIN32
// emulate snprintf() on windows, _snprintf() doesn't zero-terminate the buffer
// on overflow...
#include <stdarg.h>
inline static int snprintf(char* buf, unsigned int len, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = _vsprintf_p(buf, len, fmt, ap);
  if (len)
  buf[len - 1] = '\0';
  va_end(ap);
  return n;
}
#endif

/**
 * Requires a JS boolean argument
 */
#define REQ_BOOL_ARG(I, VAR)                                                                         \
  if (args.Length() <= (I) || !args[I]->IsBoolean())                                                 \
    return NanThrowTypeError("Argument " #I " must be a bool");                                      \
  bool VAR = args[I]->IsTrue();

/**
 * Requires a JS number argument
 */
#define REQ_INT_ARG(I, VAR)                                                                          \
  if (args.Length() <= (I) || !args[I]->IsNumber())                                                  \
    return NanThrowTypeError("Argument " #I " must be an integer");                                  \
  int VAR = args[I]->NumberValue();

/**
 * Requires a JS string argument
 */
#define REQ_STRING_ARG(I, VAR)                                                                       \
  if (args.Length() <= (I) || !args[I]->IsString())                                                  \
    return NanThrowTypeError("Argument " #I " must be a string");                                    \
  Local<String> VAR = Local<String>::Cast(args[I]);

/**
 * Requires an JS array argument
 */
#define REQ_ARRAY_ARG(I, VAR)                                                                        \
  if (args.Length() <= (I) || !args[I]->IsArray())                                                   \
    return NanThrowTypeError("Argument " #I " must be an array");                                    \
  Local<Array> VAR = Local<Array>::Cast(args[I]);

/**
 * Requires an JS function argument
 */
#define REQ_FUN_ARG(I, VAR)                                                                          \
  if (args.Length() <= (I) || !args[I]->IsFunction())                                                \
    return NanThrowTypeError("Argument " #I " must be a function");                                  \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

/**
 * Requires an JS object argument
 */
#define REQ_OBJECT_ARG(I, VAR)                                                                       \
  if (args.Length() <= (I) || !args[I]->IsObject())                                                  \
    return NanThrowTypeError("Argument " #I " must be an object");                                   \
  Local<Object> VAR = Local<Object>::Cast(args[I]);

/**
 * Get the value of a string property from the object
 */
#define OBJ_GET_STRING(OBJ, KEY, VAR)                                                                \
  {                                                                                                  \
    Local<Value> __val = OBJ->Get(NanNew<String>(KEY));                                                 \
    if(__val->IsString()) {                                                                          \
      String::Utf8Value __utf8Val(__val);                                                            \
      VAR = *__utf8Val;                                                                              \
    }                                                                                                \
  }

/**
 * Get the number value of a property from the object
 *
 */
#define OBJ_GET_NUMBER(OBJ, KEY, VAR, DEFAULT)                                                       \
  {                                                                                                  \
    Local<Value> __val = OBJ->Get(NanNew<String>(KEY));                                                 \
    if(__val->IsNumber()) {                                                                          \
      VAR = __val->ToNumber()->Value();                                                              \
    }                                                                                                \
    else if(__val->IsString()) {                                                                     \
      String::Utf8Value __utf8Value(__val);                                                          \
      VAR = atoi(*__utf8Value);                                                                      \
    } else {                                                                                         \
      VAR = DEFAULT;                                                                                 \
    }                                                                                                \
  }

#endif

