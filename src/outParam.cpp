#include "outParam.h"
#include "nodeOracleException.h"

using namespace std;

Persistent<FunctionTemplate> OutParam::constructorTemplate;

/**
 * The C++ class represents a JS constructor as follows:
 *
 * function OutParam(type, options) {
 *   this._type = type || 0;
 *   this._size = options.size;
 *   this._inOut.hasInParam = options.in;
 * }
 */
void OutParam::Init(Handle<Object> target) {
  NanScope();

  Local<FunctionTemplate> t = NanNew<FunctionTemplate>(New);
  NanAssignPersistent(constructorTemplate, t);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(NanSymbol("OutParam"));
  target->Set(NanSymbol("OutParam"),
      t->GetFunction());
}

NAN_METHOD(OutParam::New) {
  NanScope();
  OutParam *outParam = new OutParam();
  outParam->Wrap(args.This());

  if (args.Length() >= 1) {
    outParam->_type =
        args[0]->IsUndefined() ? OutParam::OCCIINT : args[0]->NumberValue();
  } else {
    outParam->_type = OutParam::OCCIINT;
  }

  if (args.Length() >= 2 && !args[1]->IsUndefined()) {
    REQ_OBJECT_ARG(1, opts);
    OBJ_GET_NUMBER(opts, "size", outParam->_size, 200);

    // check if there's an 'in' param
    if (opts->Has(NanNew<String>("in"))) {
      outParam->_inOut.hasInParam = true;
      switch (outParam->_type) {
      case OutParam::OCCIINT: {
        OBJ_GET_NUMBER(opts, "in", outParam->_inOut.intVal, 0);
        break;
      }
      case OutParam::OCCIDOUBLE: {
        OBJ_GET_NUMBER(opts, "in", outParam->_inOut.doubleVal, 0);
        break;
      }
      case OutParam::OCCIFLOAT: {
        OBJ_GET_NUMBER(opts, "in", outParam->_inOut.floatVal, 0);
        break;
      }
      case OutParam::OCCINUMBER: {
        OBJ_GET_NUMBER(opts, "in", outParam->_inOut.numberVal, 0);
        break;
      }
      case OutParam::OCCISTRING: {
        OBJ_GET_STRING(opts, "in", outParam->_inOut.stringVal);
        break;
      }
      default:
        throw NodeOracleException("Unhandled OutParam type!");
      }
    }
  }
  NanReturnValue(args.This());
}

OutParam::OutParam() {
  _type = OutParam::OCCIINT;
  _inOut.hasInParam = false;
  _size = 200;
}

OutParam::~OutParam() {
}

int OutParam::type() {
  return _type;
}

int OutParam::size() {
  return _size;
}
