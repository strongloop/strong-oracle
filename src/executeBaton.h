#ifndef _excute_baton_h_
#define _excute_baton_h_

class Connection;

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <occi.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

enum {
  VALUE_TYPE_NULL = 1,
  VALUE_TYPE_OUTPUT = 2,
  VALUE_TYPE_STRING = 3,
  VALUE_TYPE_NUMBER = 4,
  VALUE_TYPE_DATE = 5,
  VALUE_TYPE_TIMESTAMP = 6,
  VALUE_TYPE_CLOB = 7,
  VALUE_TYPE_BLOB = 8
};

struct column_t {
  int type;
  int charForm;
  std::string name;
};

struct row_t {
  std::vector<void*> values;
};

struct value_t {
  int type;
  void* value;
};

struct buffer_t {
  size_t length;
  uint8_t* data;
};

struct output_t {
  int type;
  int index;
  std::string strVal;
  int intVal;
  double doubleVal;
  float floatVal;
  std::vector<row_t*>* rows;
  std::vector<column_t*> columns;
  oracle::occi::Clob clobVal;
  oracle::occi::Date dateVal;
  oracle::occi::Timestamp timestampVal;
  oracle::occi::Number numberVal;
  oracle::occi::Blob blobVal;
  size_t bufLength;
  uint8_t *bufVal;
};

/**
 * Baton for execute function
 */
class ExecuteBaton {
public:
  ExecuteBaton(Connection* connection,
               const char* sql,
               v8::Local<v8::Array> values,
               v8::Local<v8::Function> callback = v8::Local<v8::Function>());

  ~ExecuteBaton();

  Connection *connection; // The JS connection object
  NanCallback *callback; // The JS callback function
  std::vector<value_t*> values; // The array of parameter values
  std::string sql; // The sql statement string
  std::vector<column_t*> columns; // The list of columns
  std::vector<row_t*>* rows; // The list of rows
  std::vector<output_t*>* outputs; // The output values
  std::string* error; // The error message
  int updateCount; // The update count
  uv_work_t work_req;

  void ResetValues();
  void ResetRows();
  void ResetOutputs();
  void ResetError();

  static void CopyValuesToBaton(ExecuteBaton* baton,
                                v8::Local<v8::Array> values);
};

#endif
