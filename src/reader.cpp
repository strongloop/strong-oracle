#include <string.h>
#include "connection.h"
#include "executeBaton.h"
#include "reader.h"

using namespace std;

Persistent<FunctionTemplate> Reader::s_ct;

void Reader::Init(Handle<Object> target) {
  NanScope();

  Local<FunctionTemplate> t = NanNew<FunctionTemplate>(New);
  NanAssignPersistent(Reader::s_ct, t);

  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(NanSymbol("Reader"));

  NODE_SET_PROTOTYPE_METHOD(t, "nextRows", NextRows);
  target->Set(NanSymbol("Reader"), t->GetFunction());
}

Reader::Reader(): ObjectWrap() {
  m_baton = NULL;
}

Reader::~Reader() {
  delete m_baton;
  m_baton = NULL;
}

NAN_METHOD(Reader::New) {
  NanScope();

  Reader* reader = new Reader();
  reader->Wrap(args.This());

  NanReturnValue(args.This());
}

void Reader::setBaton(ReaderBaton* baton) {
  m_baton = baton;
}

NAN_METHOD(Reader::NextRows) {
  NanScope();
  Reader* reader = ObjectWrap::Unwrap<Reader>(args.This());
  ReaderBaton* baton = reader->m_baton;
  if (baton->error) {
    Local<String> message = NanNew<String>(baton->error->c_str());
    return NanThrowError(message);
  }
  if (baton->busy) {
    return NanThrowError("invalid state: reader is busy with another nextRows call");
  }
  baton->busy = true;

  if (args.Length() > 1) {
    REQ_INT_ARG(0, count);
    REQ_FUN_ARG(1, callback);
    baton->count = count;
    baton->callback = new NanCallback(callback);
  } else {
    REQ_FUN_ARG(0, callback);
    baton->count = baton->connection->getPrefetchRowCount();
    baton->callback = new NanCallback(callback);
  }
  if (baton->count <= 0) baton->count = 1;

  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_NextRows,
                (uv_after_work_cb) EIO_AfterNextRows);

  NanReturnUndefined();
}

void Reader::EIO_NextRows(uv_work_t* req) {
  ReaderBaton* baton = CONTAINER_OF(req, ReaderBaton, work_req);

  baton->rows = new vector<row_t*>();
  if (baton->done) return;

  if (!baton->connection->getConnection()) {
    baton->error = new std::string("Connection already closed");
    return;
  }
  if (!baton->rs) {
    try {
      baton->stmt = baton->connection->getConnection()->createStatement(baton->sql);
      baton->stmt->setAutoCommit(baton->connection->getAutoCommit());
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
  NanScope();
  ReaderBaton* baton = CONTAINER_OF(req, ReaderBaton, work_req);

  baton->busy = false;

  Handle<Value> argv[2];
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
