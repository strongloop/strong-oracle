#include "executeBaton.h"
#include "outParam.h"
#include "nodeOracleException.h"
#include "connection.h"
#include <iostream>

using namespace std;

ExecuteBaton::ExecuteBaton(Connection* connection, const char* sql,
    v8::Local<v8::Array>* values, Handle<v8::Function>* callback) {
  this->connection = connection;
  this->sql = sql;
  if (callback != NULL) {
    NanCallback *cb = new NanCallback(*callback);
    this->callback = cb;
  } else {
    NanCallback *cb = new NanCallback();
    this->callback = cb;
  }
  this->outputs = new vector<output_t*>();
  this->error = NULL;
  CopyValuesToBaton(this, values);
}

ExecuteBaton::~ExecuteBaton() {
  delete callback;

  for (vector<column_t*>::iterator iterator = columns.begin(), end =
      columns.end(); iterator != end; ++iterator) {
    column_t* col = *iterator;
    delete col;
  }

  ResetValues();
  ResetRows();
  ResetOutputs();
  ResetError();

}

double CallDateMethod(v8::Local<Date> date, const char* methodName) {
  Handle<Value> args[1]; // should be zero but on windows the compiler will not allow a zero length array
  v8::Local<Value> result = v8::Local<v8::Function>::Cast(
      date->Get(String::New(methodName)))->Call(date, 0, args);
  return v8::Local<v8::Number>::Cast(result)->Value();
}

oracle::occi::Date* V8DateToOcciDate(oracle::occi::Environment* env,
    v8::Local<Date> val) {
  int year = CallDateMethod(val, "getFullYear");
  int month = CallDateMethod(val, "getMonth") + 1;
  int day = CallDateMethod(val, "getDate");
  int hours = CallDateMethod(val, "getHours");
  int minutes = CallDateMethod(val, "getMinutes");
  int seconds = CallDateMethod(val, "getSeconds");
  oracle::occi::Date* d = new oracle::occi::Date(env, year, month, day, hours,
      minutes, seconds);
  return d;
}

void ExecuteBaton::CopyValuesToBaton(ExecuteBaton* baton,
    v8::Local<v8::Array>* values) {

  if (values == NULL) {
    return;
  }

  for (uint32_t i = 0; i < (*values)->Length(); i++) {
    v8::Local<Value> val = (*values)->Get(i);
    value_t *value = new value_t();

    // null
    if (val->IsNull()) {
      value->type = VALUE_TYPE_NULL;
      value->value = NULL;
      baton->values.push_back(value);
    }

    // string
    else if (val->IsString()) {
      String::Utf8Value utf8Value(val);
      value->type = VALUE_TYPE_STRING;
      value->value = new string(*utf8Value);
      baton->values.push_back(value);
    }

    // date
    else if (val->IsDate()) {
      value->type = VALUE_TYPE_DATE;
      value->value = V8DateToOcciDate(baton->connection->getEnvironment(),
          val.As<Date>());
      baton->values.push_back(value);
    }

    // number
    else if (val->IsNumber()) {
      value->type = VALUE_TYPE_NUMBER;
      double d = v8::Number::Cast(*val)->Value();
      value->value = new oracle::occi::Number(d); // XXX not deleted in dtor
      baton->values.push_back(value);
    }

    // Buffer
    else if (node::Buffer::HasInstance(val)) {
      value->type = VALUE_TYPE_CLOB;
      v8::Local<Object> buffer = val->ToObject();
      size_t length = node::Buffer::Length(buffer);
      uint8_t* data = (uint8_t*) node::Buffer::Data(buffer);

      buffer_t* buf = new buffer_t();
      buf->length = length;

      // Copy the buffer to a new array as the original copy can be GCed
      buf->data = new uint8_t[length];
      memcpy(buf->data, data, length);

      value->value = buf;
      baton->values.push_back(value);
    }


    // output
    else if (NanHasInstance(OutParam::constructorTemplate, val)) {

      OutParam* op = node::ObjectWrap::Unwrap<OutParam>(val->ToObject());

      // [rfeng] The OutParam object will be destroyed. We need to create a new copy.
      value->type = VALUE_TYPE_OUTPUT;
      value->value = op->c_outparam();
      baton->values.push_back(value);

      output_t* output = new output_t();
      output->rows = NULL;
      output->type = op->type();
      output->index = i + 1;
      baton->outputs->push_back(output);

    }

    // unhandled type
    else {
      //XXX leaks new value on error
      baton->error = new string("CopyValuesToBaton: Unhandled value type");
      // throw NodeOracleException(message.str());
    }

  }
}

void ExecuteBaton::ResetValues() {
  for (std::vector<value_t*>::iterator iterator = values.begin(), end = values.end(); iterator != end; ++iterator) {

    value_t* val = *iterator;
    switch (val->type) {
      case VALUE_TYPE_STRING:
        delete (std::string*)val->value;
        break;
      case VALUE_TYPE_NUMBER:
        delete (oracle::occi::Number*)val->value;
        break;
      case VALUE_TYPE_TIMESTAMP:
        delete (oracle::occi::Timestamp*)val->value;
        break;
    }
    delete val;
  }
  values.clear();
}

void ExecuteBaton::ResetRows() {
  if (rows) {
    for (std::vector<row_t*>::iterator iterator = rows->begin(), end = rows->end(); iterator != end; ++iterator) {
      row_t* currentRow = *iterator;
      delete currentRow;
    }

    delete rows;
    rows = NULL;
  }
}

void ExecuteBaton::ResetOutputs() {
  if (outputs) {
    for (std::vector<output_t*>::iterator iterator = outputs->begin(), end = outputs->end(); iterator != end; ++iterator) {
      output_t* o = *iterator;
      delete o;
    }
    delete outputs;
    outputs = NULL;
  }
}

void ExecuteBaton::ResetError() {
  if (error) {
    delete error;
    error = NULL;
  }
}

