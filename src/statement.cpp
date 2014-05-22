#include <string.h>
#include "connection.h"
#include "executeBaton.h"
#include "statement.h"

using namespace std;

Persistent<FunctionTemplate> Statement::s_ct;

void Statement::Init(Handle<Object> target) {
  NanScope();

  Local<FunctionTemplate> t = NanNew<FunctionTemplate>(New);
  NanAssignPersistent(Statement::s_ct, t);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(NanSymbol("Statement"));

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

  ExecuteBaton::CopyValuesToBaton(baton, values);
  if (baton->error) {
    Local<String> message = NanNew<String>(baton->error->c_str());
    return NanThrowError(message);
  }

  if (baton->busy) {
    return NanThrowError("invalid state: statement is busy with another execute call");
  }
  baton->busy = true;

  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_Execute,
                (uv_after_work_cb) EIO_AfterExecute);

  NanReturnUndefined();
}

void Statement::EIO_Execute(uv_work_t* req) {
  StatementBaton* baton = CONTAINER_OF(req, StatementBaton, work_req);

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

void Statement::EIO_AfterExecute(uv_work_t* req) {
  NanScope();
  StatementBaton* baton = CONTAINER_OF(req, StatementBaton, work_req);

  baton->busy = false;

  Handle<Value> argv[2];
  Connection::handleResult(baton, argv);

  baton->ResetValues();
  baton->ResetRows();
  baton->ResetOutputs();
  baton->ResetError();

  // invoke callback at the very end because callback may re-enter execute.
  baton->callback->Call(2, argv);
}

