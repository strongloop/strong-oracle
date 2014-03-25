#ifndef _reader_h_
#define _reader_h_

#include <v8.h>
#include <node.h>
#include <occi.h>
#include "readerBaton.h"

using namespace node;
using namespace v8;

class Reader: public ObjectWrap {
public:
  static Persistent<FunctionTemplate> s_ct;
  static void Init(Handle<Object> target);
  static NAN_METHOD(New);
  static NAN_METHOD(NextRows);
  static void EIO_NextRows(uv_work_t* req);
  static void EIO_AfterNextRows(uv_work_t* req, int status);

  Reader();
  ~Reader();

  void setBaton(ReaderBaton* baton);

private:
  ReaderBaton* m_baton;
};

#endif
