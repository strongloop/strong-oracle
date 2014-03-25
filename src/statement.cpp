#include <string.h>
#include "connection.h"
#include "executeBaton.h"
#include "statement.h"

using namespace std;

Persistent<FunctionTemplate> Statement::s_ct;

void Statement::Init(Handle<Object> target) {
  NanScope();

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  NanAssignPersistent(FunctionTemplate, Statement::s_ct, t);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(String::NewSymbol("Statement"));

  NODE_SET_PROTOTYPE_METHOD(t, "execute", Execute);
  target->Set(NanSymbol("Statement"), t->GetFunction());
}

Statement::Statement(): ObjectWrap() {
  m_baton = NULL;
}

Statement::~Statement() {
  delete m_baton;
  m_baton = NULL;
}

NAN_METHOD(Statement::New) {
  NanScope();

  Statement* statement = new Statement();
  statement->Wrap(args.This());

  NanReturnValue(args.This());
}

void Statement::setBaton(StatementBaton* baton) {
  m_baton = baton;
}

NAN_METHOD(Statement::Execute) {
  NanScope();
  Statement* statement = ObjectWrap::Unwrap<Statement>(args.This());
  StatementBaton* baton = statement->m_baton;

  REQ_ARRAY_ARG(0, values);
  REQ_FUN_ARG(1, callback);

  baton->callback = new NanCallback(callback);

  ExecuteBaton::CopyValuesToBaton(baton, &values);
  if (baton->error) {
    Local<String> message = String::New(baton->error->c_str());
    return NanThrowError(message);
  }

  if (baton->busy) {
    return NanThrowError("invalid state: statement is busy with another execute call");
  }
  baton->busy = true;

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Execute, (uv_after_work_cb)EIO_AfterExecute);
  baton->connection->Ref();

  NanReturnUndefined();
}

void Statement::EIO_Execute(uv_work_t* req) {
  StatementBaton* baton = static_cast<StatementBaton*>(req->data);

  if (!baton->connection->getConnection()) {
    baton->error = new std::string("Connection already closed");
    return;
  }
  if (!baton->stmt) {
    baton->stmt = Connection::CreateStatement(baton);
    if (baton->error) return;
  }
  Connection::ExecuteStatement(baton, baton->stmt);
}

void Statement::EIO_AfterExecute(uv_work_t* req, int status) {
  NanScope();
  StatementBaton* baton = static_cast<StatementBaton*>(req->data);

  baton->busy = false;
  baton->connection->Unref();

  Handle<Value> argv[2];
  Connection::handleResult(baton, argv);

  baton->ResetValues();
  baton->ResetRows();
  baton->ResetOutputs();
  baton->ResetError();
  delete req;

  // invoke callback at the very end because callback may re-enter execute.
  baton->callback->Call(2, argv);
}

