#include <string.h>
#include "connection.h"
#include "executeBaton.h"
#include "reader.h"

using namespace std;

Nan::Persistent<FunctionTemplate> Reader::s_ct;

void Reader::Init(Handle<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(New);
  Reader::s_ct.Reset( t);

  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(Nan::New<String>("Reader").ToLocalChecked());

  Nan::SetPrototypeMethod(t, "nextRows", NextRows);
  target->Set(Nan::New<String>("Reader").ToLocalChecked(), t->GetFunction());
}

Reader::Reader(): Nan::ObjectWrap() {
  m_baton = NULL;
}

Reader::~Reader() {
  delete m_baton;
  m_baton = NULL;
}

NAN_METHOD(Reader::New) {
  Nan::HandleScope scope;

  Reader* reader = new Reader();
  reader->Wrap(info.This());

  info.GetReturnValue().Set(info.This());
}

void Reader::setBaton(ReaderBaton* baton) {
  m_baton = baton;
}

NAN_METHOD(Reader::NextRows) {
  Nan::HandleScope scope;
  Reader* reader = Nan::ObjectWrap::Unwrap<Reader>(info.This());
  ReaderBaton* baton = reader->m_baton;
  if (baton->error) {
    Local<String> message = Nan::New<String>(baton->error->c_str()).ToLocalChecked();
    return Nan::ThrowError(message);
  }
  if (baton->busy) {
    return Nan::ThrowError("invalid state: reader is busy with another nextRows call");
  }
  baton->busy = true;

  if (info.Length() > 1) {
    REQ_INT_ARG(0, count);
    REQ_FUN_ARG(1, callback);
    baton->count = count;
    baton->callback = new Nan::Callback(callback);
  } else {
    REQ_FUN_ARG(0, callback);
    baton->count = baton->m_prefetchRowCount;
    baton->callback = new Nan::Callback(callback);
  }
  if (baton->count <= 0) baton->count = 1;

  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_NextRows,
                (uv_after_work_cb) EIO_AfterNextRows);

  return;
}

void Reader::EIO_NextRows(uv_work_t* req) {
  ReaderBaton* baton = CONTAINER_OF(req, ReaderBaton, work_req);

  baton->rows = new vector<row_t*>();
  if (baton->done) return;

  if (!baton->m_connection) {
    baton->error = new std::string("Connection already closed");
    return;
  }
  if (!baton->rs) {
    try {
      baton->stmt = baton->m_connection->createStatement(baton->sql);
      baton->stmt->setAutoCommit(baton->m_autoCommit);
      baton->stmt->setPrefetchRowCount(baton->count);
      Connection::SetValuesOnStatement(baton->stmt, baton->values);
      if (baton->error) return;

      int status = baton->stmt->execute();
      if (status != oracle::occi::Statement::RESULT_SET_AVAILABLE) {
         baton->error = new std::string("Connection already closed");
        return;
      }     
      baton->rs = baton->stmt->getResultSet();
    } catch (oracle::occi::SQLException &ex) {
      baton->error = new string(ex.getMessage());
      return;
    }
    Connection::CreateColumnsFromResultSet(baton->rs, baton->columns);
    if (baton->error) return;
  }
  for (int i = 0; i < baton->count && baton->rs->next(); i++) {
    row_t* row = Connection::CreateRowFromCurrentResultSetRow(baton->rs, baton->columns);
    if (baton->error) return;
    baton->rows->push_back(row);
  }
  if (baton->rows->size() < (size_t)baton->count) baton->done = true;
}

void Reader::EIO_AfterNextRows(uv_work_t* req, int status) {
  Nan::HandleScope scope;
  ReaderBaton* baton = CONTAINER_OF(req, ReaderBaton, work_req);

  baton->busy = false;

  Local<Value> argv[2];
  Connection::handleResult(baton, argv);

  baton->ResetRows();
  if (baton->done || baton->error) {
    // free occi resources so that we don't run out of cursors if gc is not fast enough
    // reader destructor will delete the baton and everything else.
    baton->ResetStatement();
  }

  // invoke callback at the very end because callback may re-enter nextRows.
  baton->callback->Call(2, argv);
}
