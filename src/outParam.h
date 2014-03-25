#ifndef _outparam_h_
#define _outparam_h_

#include <v8.h>
#include <node.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "utils.h"
#include <occi.h>

using namespace node;
using namespace v8;

struct inout_t {
  bool hasInParam;
  const char* stringVal;
  int intVal;
  double doubleVal;
  float floatVal;
  oracle::occi::Date dateVal;
  oracle::occi::Timestamp timestampVal;
  oracle::occi::Number numberVal;
};

struct outparam_t {
  int type;
  int size;
  inout_t inOut;
};

/**
 * Oracle out parameter
 */
class OutParam: public ObjectWrap {
public:
  static void Init(Handle<Object> target);
  static NAN_METHOD(New);
  static Persistent<FunctionTemplate> constructorTemplate;
  int _type;
  int _size;
  inout_t _inOut;
  OutParam();
  ~OutParam();

  int type();
  int size();

  /**
   * Create a copy of the outparam to C style struct
   */
  outparam_t* c_outparam() {
    outparam_t* p = new outparam_t();
    p->type = _type;
    p->size = _size;
    p->inOut.hasInParam = _inOut.hasInParam;
    p->inOut.stringVal = _inOut.stringVal;
    p->inOut.intVal = _inOut.intVal;
    p->inOut.doubleVal = _inOut.doubleVal;
    p->inOut.floatVal = _inOut.floatVal;
    p->inOut.dateVal = _inOut.dateVal;
    p->inOut.timestampVal = _inOut.timestampVal;
    p->inOut.numberVal = _inOut.numberVal;
    return p;
  }

  static const int OCCIINT = 0;
  static const int OCCISTRING = 1;
  static const int OCCIDOUBLE = 2;
  static const int OCCIFLOAT = 3;
  static const int OCCICURSOR = 4;
  static const int OCCICLOB = 5;
  static const int OCCIDATE = 6;
  static const int OCCITIMESTAMP = 7;
  static const int OCCINUMBER = 8;
  static const int OCCIBLOB = 9;

};

#endif
