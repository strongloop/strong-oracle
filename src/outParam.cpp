#include "outParam.h"
#include "nodeOracleException.h"

using namespace std;

Nan::Persistent<FunctionTemplate> OutParam::constructorTemplate;

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
  Nan::HandleScope scope;

  Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(New);
  constructorTemplate.Reset( t);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(Nan::New<String>("OutParam").ToLocalChecked());
  target->Set(Nan::New<String>("OutParam").ToLocalChecked(),
      t->GetFunction());
}

NAN_METHOD(OutParam::New) {
  Nan::HandleScope scope;
  OutParam *outParam = new OutParam();
  outParam->Wrap(info.This());

  if (info.Length() >= 1) {
    outParam->_type =
        info[0]->IsUndefined() ? OutParam::OCCIINT : info[0]->NumberValue();
  } else {
    outParam->_type = OutParam::OCCIINT;
  }

  if (info.Length() >= 2 && !info[1]->IsUndefined()) {
    REQ_OBJECT_ARG(1, opts);
    OBJ_GET_NUMBER(opts, "size", outParam->_size, 200);

    // check if there's an 'in' param
    if (opts->Has(Nan::New<String>("in").ToLocalChecked())) {
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
  info.GetReturnValue().Set(info.This());
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
