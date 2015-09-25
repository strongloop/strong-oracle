#include <string.h>
#include "connection.h"
#include "executeBaton.h"
#include "statement.h"

using namespace std;

Nan::Persistent<FunctionTemplate> Statement::s_ct;

void Statement::Init(Handle<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(New);
  Statement::s_ct.Reset( t);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(Nan::New<String>("Statement").ToLocalChecked());

  Nan::SetPrototypeMethod(t, "execute", Execute);
  target->Set(Nan::New<String>("Statement").ToLocalChecked(), t->GetFunction());
}

Statement::Statement(): Nan::ObjectWrap() {
  m_baton = NULL;
}

Statement::~Statement() {
  delete m_baton;
  m_baton = NULL;
}

NAN_METHOD(Statement::New) {
  Nan::HandleScope scope;

  Statement* statement = new Statement();
  statement->Wrap(info.This());

  info.GetReturnValue().Set(info.This());
}

void Statement::setBaton(StatementBaton* baton) {
  m_baton = baton;
}

NAN_METHOD(Statement::Execute) {
  Nan::HandleScope scope;
  Statement* statement = Nan::ObjectWrap::Unwrap<Statement>(info.This());
  StatementBaton* baton = statement->m_baton;

  REQ_ARRAY_ARG(0, values);
  REQ_FUN_ARG(1, callback);

  baton->callback = new Nan::Callback(callback);

  ExecuteBaton::CopyValuesToBaton(baton, values);
  if (baton->error) {
    Local<String> message = Nan::New<String>(baton->error->c_str()).ToLocalChecked();
    return Nan::ThrowError(message);
  }

  if (baton->busy) {
    return Nan::ThrowError("invalid state: statement is busy with another execute call");
  }
  baton->busy = true;

  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_Execute,
                (uv_after_work_cb) EIO_AfterExecute);

  return;
}

void Statement::EIO_Execute(uv_work_t* req) {
  StatementBaton* baton = CONTAINER_OF(req, StatementBaton, work_req);

  if (!baton->m_connection) {
    baton->error = new std::string("Connection already closed");
    return;
  }
  if (!baton->stmt) {
    baton->stmt = Connection::CreateStatement(baton);
    if (baton->error) return;
  }
  Connection::ExecuteStatement(baton, baton->stmt);
}

void Statement::EIO_AfterExecute(uv_work_t* req) {
  Nan::HandleScope scope;
  StatementBaton* baton = CONTAINER_OF(req, StatementBaton, work_req);

  baton->busy = false;

  Local<Value> argv[2];
  Connection::handleResult(baton, argv);

  baton->ResetValues();
  baton->ResetRows();
  baton->ResetOutputs();
  baton->ResetError();

  // invoke callback at the very end because callback may re-enter execute.
  baton->callback->Call(2, argv);
}

