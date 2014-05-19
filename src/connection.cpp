#include <string.h>
#include "connection.h"
#include "executeBaton.h"
#include "outParam.h"
#include "statement.h"
#include "reader.h"
#include "node_buffer.h"
#include <vector>
#include <node_version.h>
#include <iostream>
using namespace std;

Persistent<FunctionTemplate> ConnectionPool::s_ct;
Persistent<FunctionTemplate> Connection::s_ct;

// ConnectionPool implementation
ConnectionPool::ConnectionPool() :
    m_environment(NULL), m_connectionPool(NULL) {
}

ConnectionPool::~ConnectionPool() {
  try {
    closeConnectionPool(oracle::occi::StatelessConnectionPool::SPD_FORCE);
  } catch (std::exception &ex) {
    m_connectionPool = NULL;
    fprintf(stderr, "%s\n", ex.what());
  }
}

void ConnectionPool::Init(Handle<Object> target) {
  NanScope();

  Local<FunctionTemplate> t = NanNew<FunctionTemplate>(ConnectionPool::New);
  NanAssignPersistent(ConnectionPool::s_ct, t);

  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(NanSymbol("ConnectionPool"));

  NODE_SET_PROTOTYPE_METHOD(t, "getConnectionSync", ConnectionPool::GetConnectionSync);
  NODE_SET_PROTOTYPE_METHOD(t, "getConnection", ConnectionPool::GetConnection);
  NODE_SET_PROTOTYPE_METHOD(t, "close", ConnectionPool::Close);
  NODE_SET_PROTOTYPE_METHOD(t, "closeSync", ConnectionPool::CloseSync);
  NODE_SET_PROTOTYPE_METHOD(t, "getInfo", ConnectionPool::GetInfo);

  target->Set(NanSymbol("ConnectionPool"), t->GetFunction());
}

NAN_METHOD(ConnectionPool::New) {
  NanScope();

  ConnectionPool *connectionPool = new ConnectionPool();
  connectionPool->Wrap(args.This());
  NanReturnValue(args.This());
}

void ConnectionPool::setConnectionPool(oracle::occi::Environment* environment,
    oracle::occi::StatelessConnectionPool* connectionPool) {
  m_environment = environment;
  m_connectionPool = connectionPool;
}

NAN_METHOD(ConnectionPool::CloseSync) {
  NanScope();
  try {
    ConnectionPool* connectionPool = ObjectWrap::Unwrap<ConnectionPool>(
        args.This());

    // Get the optional destroy mode
    oracle::occi::StatelessConnectionPool::DestroyMode mode =
        oracle::occi::StatelessConnectionPool::DEFAULT;
    if (args.Length() == 1 || args[0]->IsUint32()) {
      mode =
          static_cast<oracle::occi::StatelessConnectionPool::DestroyMode>(args[0]->Uint32Value());
    }

    connectionPool->closeConnectionPool(mode);

    NanReturnUndefined();
  } catch (const exception& ex) {
    return NanThrowError(ex.what());
  }
}

NAN_METHOD(ConnectionPool::Close) {
  NanScope();
  ConnectionPool* connectionPool = ObjectWrap::Unwrap<ConnectionPool>(
        args.This());

  // Get the optional destroy mode
  oracle::occi::StatelessConnectionPool::DestroyMode mode =
      oracle::occi::StatelessConnectionPool::DEFAULT;
  if (args.Length() >= 1 && args[0]->IsUint32()) {
    mode =
        static_cast<oracle::occi::StatelessConnectionPool::DestroyMode>(args[0]->Uint32Value());
  }

  Local<Function> callback;
  if (args.Length() == 1 && args[0]->IsFunction()) {
    callback = Local<Function>::Cast(args[0]);
  } else if (args.Length() > 1 && args[1]->IsFunction()) {
    callback = Local<Function>::Cast(args[1]);
  }

  ConnectionPoolBaton* baton =
      new ConnectionPoolBaton(connectionPool->getEnvironment(),
                              connectionPool,
                              mode,
                              callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                ConnectionPool::EIO_Close,
                (uv_after_work_cb) ConnectionPool::EIO_AfterClose);

  NanReturnUndefined();
}

void ConnectionPool::EIO_Close(uv_work_t* req) {
  ConnectionPoolBaton* baton = CONTAINER_OF(req, ConnectionPoolBaton, work_req);
  try {
    baton->connectionPool->closeConnectionPool(baton->destroyMode);
  } catch(const exception &ex) {
    baton->error = new std::string(ex.what());
    baton->connectionPool->m_connectionPool = NULL;
  }
}

void ConnectionPool::EIO_AfterClose(uv_work_t* req) {
  NanScope();
  ConnectionPoolBaton* baton = CONTAINER_OF(req, ConnectionPoolBaton, work_req);

  v8::TryCatch tryCatch;
  if (baton->callback != NULL) {
    Handle<Value> argv[2];
    if (baton->error) {
      argv[0] = NanError(baton->error->c_str());
      argv[1] = NanUndefined();
    } else {
      argv[0] = NanUndefined();
      argv[1] = NanUndefined();
    }
    baton->callback->Call(2, argv);
  }
  delete baton;

  if (tryCatch.HasCaught()) {
    node::FatalException(tryCatch);
  }
}

NAN_METHOD(ConnectionPool::GetInfo) {
  NanScope();
  ConnectionPool* connectionPool = ObjectWrap::Unwrap<ConnectionPool>(
      args.This());
  if (connectionPool->m_connectionPool) {
    Local<Object> obj = NanNew<Object>();

    obj->Set(NanSymbol("openConnections"),
        NanNew<Uint32>(connectionPool->m_connectionPool->getOpenConnections()));
    obj->Set(NanSymbol("busyConnections"),
        NanNew<Uint32>(connectionPool->m_connectionPool->getBusyConnections()));
    obj->Set(NanSymbol("maxConnections"),
        NanNew<Uint32>(connectionPool->m_connectionPool->getMaxConnections()));
    obj->Set(NanSymbol("minConnections"),
        NanNew<Uint32>(connectionPool->m_connectionPool->getMinConnections()));
    obj->Set(NanSymbol("incrConnections"),
        NanNew<Uint32>(connectionPool->m_connectionPool->getIncrConnections()));
    obj->Set(NanSymbol("busyOption"),
        NanNew<Uint32>(static_cast<unsigned int>(connectionPool->m_connectionPool->getBusyOption())));
    obj->Set(NanSymbol("timeout"),
        NanNew<Uint32>(connectionPool->m_connectionPool->getTimeOut()));

    obj->Set(NanSymbol("poolName"),
        NanNew<String>(connectionPool->m_connectionPool->getPoolName().c_str()));

    NanReturnValue(obj);
  } else {
    NanReturnUndefined();
  }
}

NAN_METHOD(ConnectionPool::GetConnectionSync) {
  NanScope();
  try {
    ConnectionPool* connectionPool = ObjectWrap::Unwrap<ConnectionPool>(
        args.This());
    // std::string tag = "strong-oracle";
    oracle::occi::Connection *conn =
        connectionPool->getConnectionPool()->getConnection("strong-oracle");
    Local<FunctionTemplate> ft = NanNew(Connection::s_ct);
    Handle<Object> connection = ft->GetFunction()->NewInstance();
    (node::ObjectWrap::Unwrap<Connection>(connection))->setConnection(
        connectionPool->getEnvironment(),
        connectionPool,
        conn);
    NanReturnValue(connection);
  } catch (const exception& ex) {
    return NanThrowError(ex.what());
  }
}

NAN_METHOD(ConnectionPool::GetConnection) {
  NanScope();
  ConnectionPool* connectionPool = ObjectWrap::Unwrap<ConnectionPool>(
      args.This());

  REQ_FUN_ARG(0, callback);

  ConnectionPoolBaton* baton =
      new ConnectionPoolBaton(connectionPool->getEnvironment(),
                              connectionPool,
                              oracle::occi::StatelessConnectionPool::DEFAULT,
                              callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                ConnectionPool::EIO_GetConnection,
                (uv_after_work_cb) ConnectionPool::EIO_AfterGetConnection);

  NanReturnUndefined();
}

void ConnectionPool::EIO_GetConnection(uv_work_t* req) {
  ConnectionPoolBaton* baton = CONTAINER_OF(req, ConnectionPoolBaton, work_req);
  baton->error = NULL;
  try {
    oracle::occi::Connection *conn =
        baton->connectionPool->getConnectionPool()->getConnection(
            "strong-oracle");
    baton->connection = conn;
  } catch (oracle::occi::SQLException &ex) {
    baton->error = new std::string(ex.getMessage());
  }
}

void ConnectionPool::EIO_AfterGetConnection(uv_work_t* req) {
  NanScope();
  ConnectionPoolBaton* baton = CONTAINER_OF(req, ConnectionPoolBaton, work_req);

  Handle<Value> argv[2];
  if (baton->error) {
    argv[0] = NanError(baton->error->c_str());
    argv[1] = NanUndefined();
  } else {
    argv[0] = NanUndefined();
    Local<FunctionTemplate> ft = NanNew(Connection::s_ct);
    Handle<Object> connection = ft->GetFunction()->NewInstance();
    (node::ObjectWrap::Unwrap<Connection>(connection))->setConnection(
        baton->connectionPool->getEnvironment(),
        baton->connectionPool,
        baton->connection);
    argv[1] = connection;
  }

  v8::TryCatch tryCatch;
  baton->callback->Call(2, argv);
  delete baton;

  if (tryCatch.HasCaught()) {
    node::FatalException(tryCatch);
  }

}

void ConnectionPool::closeConnectionPool(
    oracle::occi::StatelessConnectionPool::DestroyMode mode) {
  if (m_environment && m_connectionPool) {
    m_environment->terminateStatelessConnectionPool(m_connectionPool, mode);
    m_connectionPool = NULL;
  }
}

void Connection::Init(Handle<Object> target) {
  NanScope();

  Local<FunctionTemplate> t = NanNew<FunctionTemplate>(Connection::New);
  NanAssignPersistent(Connection::s_ct, t);

  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(NanSymbol("Connection"));

  NODE_SET_PROTOTYPE_METHOD(t, "execute", Execute);
  NODE_SET_PROTOTYPE_METHOD(t, "executeSync", ExecuteSync);
  NODE_SET_PROTOTYPE_METHOD(t, "readerHandle", CreateReader);
  NODE_SET_PROTOTYPE_METHOD(t, "prepare", Prepare);
  NODE_SET_PROTOTYPE_METHOD(t, "close", Connection::Close);
  NODE_SET_PROTOTYPE_METHOD(t, "closeSync", Connection::CloseSync);
  NODE_SET_PROTOTYPE_METHOD(t, "isConnected", IsConnected);
  NODE_SET_PROTOTYPE_METHOD(t, "setAutoCommit", SetAutoCommit);
  NODE_SET_PROTOTYPE_METHOD(t, "setPrefetchRowCount", SetPrefetchRowCount);
  NODE_SET_PROTOTYPE_METHOD(t, "commit", Commit);
  NODE_SET_PROTOTYPE_METHOD(t, "rollback", Rollback);

  target->Set(NanSymbol("Connection"), t->GetFunction());
}

NAN_METHOD(Connection::New) {
  NanScope();

  Connection *connection = new Connection();
  connection->Wrap(args.This());
  NanReturnValue(args.This());
}

NAN_METHOD(Connection::Prepare) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());

  REQ_STRING_ARG(0, sql);

  String::Utf8Value sqlVal(sql);

  StatementBaton* baton = new StatementBaton(connection, *sqlVal);
  Local<FunctionTemplate> ft = NanNew(Statement::s_ct);
  Handle<Object> statementHandle = ft->GetFunction()->NewInstance();
  Statement* statement = ObjectWrap::Unwrap<Statement>(statementHandle);
  statement->setBaton(baton);

  NanReturnValue(statementHandle);
}

NAN_METHOD(Connection::CreateReader) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());

  REQ_STRING_ARG(0, sql);
  REQ_ARRAY_ARG(1, values);

  String::Utf8Value sqlVal(sql);

  ReaderBaton* baton = new ReaderBaton(connection, *sqlVal, values);

  Local<FunctionTemplate> ft = NanNew(Reader::s_ct);
  Local<Object> readerHandle = ft->GetFunction()->NewInstance();
  Reader* reader = ObjectWrap::Unwrap<Reader>(readerHandle);
  reader->setBaton(baton);

  NanReturnValue(readerHandle);
}

Connection::Connection() :
    m_environment(NULL), connectionPool(NULL), m_connection(NULL),
    m_autoCommit(true), m_prefetchRowCount(0) {
}

Connection::~Connection() {
  try {
    closeConnection();
  } catch (std::exception &ex) {
    m_connection = NULL;
    fprintf(stderr, "%s\n", ex.what());
  }
}

NAN_METHOD(Connection::Execute) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());

  REQ_STRING_ARG(0, sql);
  REQ_ARRAY_ARG(1, values);
  REQ_FUN_ARG(2, callback);

  String::Utf8Value sqlVal(sql);

  ExecuteBaton* baton = new ExecuteBaton(connection, *sqlVal, values, callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_Execute,
                (uv_after_work_cb) EIO_AfterExecute);

  NanReturnUndefined();
}

NAN_METHOD(Connection::CloseSync) {
  NanScope();
  try {
    Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());
    connection->closeConnection();

    NanReturnUndefined();
  } catch (const exception& ex) {
    return NanThrowError(ex.what());
  }
}

NAN_METHOD(Connection::IsConnected) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());

  if (connection && connection->m_connection) {
    NanReturnValue(NanNew<Boolean>(true));
  } else {
    NanReturnValue(NanNew<Boolean>(false));
  }
}

NAN_METHOD(Connection::Commit) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());

  REQ_FUN_ARG(0, callback);

  ConnectionBaton* baton = new ConnectionBaton(connection, callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_Commit,
                (uv_after_work_cb) EIO_AfterCommit);

  NanReturnUndefined();
}

NAN_METHOD(Connection::Rollback) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());

  REQ_FUN_ARG(0, callback);

  ConnectionBaton* baton = new ConnectionBaton(connection, callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_Rollback,
                (uv_after_work_cb) EIO_AfterRollback);

  NanReturnUndefined();
}

NAN_METHOD(Connection::SetAutoCommit) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());
  REQ_BOOL_ARG(0, autoCommit);
  connection->m_autoCommit = autoCommit;
  NanReturnUndefined();
}

NAN_METHOD(Connection::SetPrefetchRowCount) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());
  REQ_INT_ARG(0, prefetchRowCount);
  connection->m_prefetchRowCount = prefetchRowCount;
  NanReturnUndefined();
}

void Connection::closeConnection() {
  if(!m_connection) {
    return;
  }
  if(m_environment && !connectionPool) {
    // The connection is not pooled
    m_environment->terminateConnection(m_connection);
    m_connection = NULL;
    return;
  }
  // Make sure the connection pool is still open
  if (m_environment && connectionPool && connectionPool->getConnectionPool()) {
    connectionPool->getConnectionPool()->releaseConnection(m_connection, "strong-oracle");
    m_connection = NULL;
    connectionPool = NULL;
  }
}

void RandomBytesFree(char* data, void* hint) {
  delete[] data;
}

int Connection::SetValuesOnStatement(oracle::occi::Statement* stmt,
    vector<value_t*> &values) {
  uint32_t index = 1;
  int outputParam = -1;
  outparam_t * outParam = NULL;
  for (vector<value_t*>::iterator iterator = values.begin(), end = values.end();
      iterator != end; ++iterator, index++) {
    value_t* val = *iterator;
    int outParamType;

    switch (val->type) {
    case VALUE_TYPE_NULL:
      stmt->setNull(index, oracle::occi::OCCISTRING);
      break;
    case VALUE_TYPE_STRING:
      stmt->setString(index, *((string*) val->value));
      break;
    case VALUE_TYPE_NUMBER:
      stmt->setNumber(index, *((oracle::occi::Number*) val->value));
      break;
    case VALUE_TYPE_DATE:
      stmt->setDate(index, *((oracle::occi::Date*) val->value));
      break;
    case VALUE_TYPE_TIMESTAMP:
      stmt->setTimestamp(index, *((oracle::occi::Timestamp*) val->value));
      break;
    case VALUE_TYPE_OUTPUT:
      outParam = static_cast<outparam_t*>(val->value);
      outParamType = outParam->type;
      switch (outParamType) {
      case OutParam::OCCIINT:
        if (outParam->inOut.hasInParam) {
          stmt->setInt(index, outParam->inOut.intVal);
        } else {
          stmt->registerOutParam(index, oracle::occi::OCCIINT);
        }
        break;
      case OutParam::OCCISTRING:
        if (outParam->inOut.hasInParam) {
          stmt->setString(index, outParam->inOut.stringVal);
        } else {
          stmt->registerOutParam(index, oracle::occi::OCCISTRING,
              outParam->size);
        }
        break;
      case OutParam::OCCIDOUBLE:
        if (outParam->inOut.hasInParam) {
          stmt->setDouble(index, outParam->inOut.doubleVal);
        } else {
          stmt->registerOutParam(index, oracle::occi::OCCIDOUBLE);
        }
        break;
      case OutParam::OCCIFLOAT:
        if (outParam->inOut.hasInParam) {
          stmt->setFloat(index, outParam->inOut.floatVal);
        } else {
          stmt->registerOutParam(index, oracle::occi::OCCIFLOAT);
        }
        break;
      case OutParam::OCCICURSOR:
        stmt->registerOutParam(index, oracle::occi::OCCICURSOR);
        break;
      case OutParam::OCCICLOB:
        stmt->registerOutParam(index, oracle::occi::OCCICLOB);
        break;
      case OutParam::OCCIDATE:
        stmt->registerOutParam(index, oracle::occi::OCCIDATE);
        break;
      case OutParam::OCCITIMESTAMP:
        stmt->registerOutParam(index, oracle::occi::OCCITIMESTAMP);
        break;
      case OutParam::OCCINUMBER: {
        if (outParam->inOut.hasInParam) {
          stmt->setNumber(index, outParam->inOut.numberVal);
        } else {
          stmt->registerOutParam(index, oracle::occi::OCCINUMBER);
        }
        break;
      }
      case OutParam::OCCIBLOB:
        stmt->registerOutParam(index, oracle::occi::OCCIBLOB);
        break;
      default:
        delete outParam;
        char msg[128];
        snprintf(msg, sizeof(msg),
            "SetValuesOnStatement: Unknown OutParam type: %d", outParamType);
        std::string strMsg = std::string(msg);
        throw NodeOracleException(strMsg);
      }
      delete outParam;
      outputParam = index;
      break;
    default:
      throw NodeOracleException("SetValuesOnStatement: Unhandled value type");
    }
  }
  return outputParam;
}

void Connection::CreateColumnsFromResultSet(oracle::occi::ResultSet* rs,
    vector<column_t*> &columns) {
  vector<oracle::occi::MetaData> metadata = rs->getColumnListMetaData();
  for (vector<oracle::occi::MetaData>::iterator iterator = metadata.begin(),
      end = metadata.end(); iterator != end; ++iterator) {
    oracle::occi::MetaData metadata = *iterator;
    column_t* col = new column_t();
    col->name = metadata.getString(oracle::occi::MetaData::ATTR_NAME);
    int type = metadata.getInt(oracle::occi::MetaData::ATTR_DATA_TYPE);
    col->charForm = metadata.getInt(oracle::occi::MetaData::ATTR_CHARSET_FORM);
    switch (type) {
    case oracle::occi::OCCI_TYPECODE_NUMBER:
    case oracle::occi::OCCI_TYPECODE_FLOAT:
    case oracle::occi::OCCI_TYPECODE_DOUBLE:
    case oracle::occi::OCCI_TYPECODE_REAL:
    case oracle::occi::OCCI_TYPECODE_DECIMAL:
    case oracle::occi::OCCI_TYPECODE_INTEGER:
    case oracle::occi::OCCI_TYPECODE_SMALLINT:
      col->type = VALUE_TYPE_NUMBER;
      break;
    case oracle::occi::OCCI_TYPECODE_VARCHAR2:
    case oracle::occi::OCCI_TYPECODE_VARCHAR:
    case oracle::occi::OCCI_TYPECODE_CHAR:
      col->type = VALUE_TYPE_STRING;
      break;
    case oracle::occi::OCCI_TYPECODE_CLOB:
      col->type = VALUE_TYPE_CLOB;
      break;
    case oracle::occi::OCCI_TYPECODE_DATE:
      col->type = VALUE_TYPE_DATE;
      break;
      //Use OCI_TYPECODE from oro.h because occiCommon.h does not re-export these in its TypeCode enum
    case OCI_TYPECODE_TIMESTAMP:
    case OCI_TYPECODE_TIMESTAMP_TZ: //Timezone
    case OCI_TYPECODE_TIMESTAMP_LTZ: //Local Timezone
      col->type = VALUE_TYPE_TIMESTAMP;
      break;
    case oracle::occi::OCCI_TYPECODE_BLOB:
      col->type = VALUE_TYPE_BLOB;
      break;
    default:
      char msg[128];
      snprintf(msg, sizeof(msg),
          "CreateColumnsFromResultSet: Unhandled oracle data type: %d", type);
      delete col;
      std::string strMsg = std::string(msg);
      throw NodeOracleException(strMsg);
      break;
    }
    columns.push_back(col);
  }
}

row_t* Connection::CreateRowFromCurrentResultSetRow(oracle::occi::ResultSet* rs,
    vector<column_t*> &columns) {
  row_t* row = new row_t();
  int colIndex = 1;
  for (vector<column_t*>::iterator iterator = columns.begin(), end =
      columns.end(); iterator != end; ++iterator, colIndex++) {
    column_t* col = *iterator;
    if (rs->isNull(colIndex)) {
      row->values.push_back(NULL);
    } else {
      switch (col->type) {
      case VALUE_TYPE_STRING:
        row->values.push_back(new string(rs->getString(colIndex)));
        break;
      case VALUE_TYPE_NUMBER:
        row->values.push_back(
            new oracle::occi::Number(rs->getNumber(colIndex)));
        break;
      case VALUE_TYPE_DATE:
        row->values.push_back(new oracle::occi::Date(rs->getDate(colIndex)));
        break;
      case VALUE_TYPE_TIMESTAMP:
        row->values.push_back(
            new oracle::occi::Timestamp(rs->getTimestamp(colIndex)));
        break;
      case VALUE_TYPE_CLOB: {
        oracle::occi::Clob clob = rs->getClob(colIndex);
        row->values.push_back(Connection::readClob(clob, col->charForm));
        }
        break;
      case VALUE_TYPE_BLOB: {
        oracle::occi::Blob blob = rs->getBlob(colIndex);
        row->values.push_back(Connection::readBlob(blob));
        }
        break;
      default:
        char msg[128];
        snprintf(msg, sizeof(msg),
            "CreateRowFromCurrentResultSetRow: Unhandled type: %d", col->type);
        std::string strMsg = std::string(msg);
        throw NodeOracleException(strMsg);
        break;
      }
    }
  }
  return row;
}

void Connection::EIO_AfterCall(uv_work_t* req) {
  NanScope();
  ConnectionBaton* baton = CONTAINER_OF(req, ConnectionBaton, work_req);

  v8::TryCatch tryCatch;
  if (baton->callback != NULL) {
    Handle<Value> argv[2];
    if (baton->error) {
      argv[0] = NanError(baton->error->c_str());
      argv[1] = NanUndefined();
    } else {
      argv[0] = NanUndefined();
      argv[1] = NanUndefined();
    }
    baton->callback->Call(2, argv);
  }
  delete baton;

  if (tryCatch.HasCaught()) {
    node::FatalException(tryCatch);
  }
}

void Connection::EIO_Commit(uv_work_t* req) {
  ConnectionBaton* baton = CONTAINER_OF(req, ConnectionBaton, work_req);

  baton->connection->m_connection->commit();
}

void Connection::EIO_AfterCommit(uv_work_t* req) {
  Connection::EIO_AfterCall(req);
}

void Connection::EIO_Rollback(uv_work_t* req) {
  ConnectionBaton* baton = CONTAINER_OF(req, ConnectionBaton, work_req);

  baton->connection->m_connection->rollback();
}

void Connection::EIO_AfterRollback(uv_work_t* req) {
  Connection::EIO_AfterCall(req);
}

NAN_METHOD(Connection::Close) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());

  Local<Function> callback;
  if (args.Length() > 0 && args[0]->IsFunction()) {
    callback = Local<Function>::Cast(args[0]);
  }

  ConnectionBaton* baton = new ConnectionBaton(connection, callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                Connection::EIO_Close,
                (uv_after_work_cb) Connection::EIO_AfterClose);

  NanReturnUndefined();
}

void Connection::EIO_Close(uv_work_t* req) {
  ConnectionBaton* baton = CONTAINER_OF(req, ConnectionBaton, work_req);
  try {
    baton->connection->closeConnection();
  } catch(const exception &ex) {
    baton->error = new std::string(ex.what());
  }
}

void Connection::EIO_AfterClose(uv_work_t* req) {
  Connection::EIO_AfterCall(req);
}

void Connection::EIO_Execute(uv_work_t* req) {
  ExecuteBaton* baton = CONTAINER_OF(req, ExecuteBaton, work_req);

  oracle::occi::Statement* stmt = CreateStatement(baton);
  if (baton->error) return;

  ExecuteStatement(baton, stmt);

  if (stmt) {
    if (baton->connection->m_connection) {
      baton->connection->m_connection->terminateStatement(stmt);
    }
    stmt = NULL;
  }
}

void CallDateMethod(v8::Local<v8::Date> date, const char* methodName, int val) {
  Handle<Value> args[1];
  args[0] = NanNew<Number>(val);
  Local<Function>::Cast(date->Get(NanNew<String>(methodName)))->Call(date, 1,
      args);
}

Local<Date> OracleDateToV8Date(oracle::occi::Date* d) {
  int year;
  unsigned int month, day, hour, min, sec;
  d->getDate(year, month, day, hour, min, sec);
  Local<Date> date = NanNew<Date>(0.0).As<Date>();
  CallDateMethod(date, "setUTCMilliseconds", 0);
  CallDateMethod(date, "setUTCSeconds", sec);
  CallDateMethod(date, "setUTCMinutes", min);
  CallDateMethod(date, "setUTCHours", hour);
  CallDateMethod(date, "setUTCDate", day);
  CallDateMethod(date, "setUTCMonth", month - 1);
  CallDateMethod(date, "setUTCFullYear", year);
  return date;
}

Local<Date> OracleTimestampToV8Date(oracle::occi::Timestamp* d) {
  int year;
  unsigned int month, day, hour, min, sec, fs, ms;
  d->getDate(year, month, day);
  d->getTime(hour, min, sec, fs);
  Local<Date> date = NanNew<Date>(0.0).As<Date>();
  //occi always returns nanoseconds, regardless of precision set on timestamp column
  ms = (fs / 1000000.0) + 0.5; // add 0.5 to round to nearest millisecond

  CallDateMethod(date, "setUTCMilliseconds", ms);
  CallDateMethod(date, "setUTCSeconds", sec);
  CallDateMethod(date, "setUTCMinutes", min);
  CallDateMethod(date, "setUTCHours", hour);
  CallDateMethod(date, "setUTCDate", day);
  CallDateMethod(date, "setUTCMonth", month - 1);
  CallDateMethod(date, "setUTCFullYear", year);
  return date;
}

Local<Object> Connection::CreateV8ObjectFromRow(vector<column_t*> columns,
    row_t* currentRow) {
  Local<Object> obj = NanNew<Object>();
  uint32_t colIndex = 0;
  for (vector<column_t*>::iterator iterator = columns.begin(), end =
      columns.end(); iterator != end; ++iterator, colIndex++) {
    column_t* col = *iterator;
    void* val = currentRow->values[colIndex];
    if (val == NULL) {
      obj->Set(NanNew<String>(col->name.c_str()), NanNull());
    } else {
      switch (col->type) {
      case VALUE_TYPE_STRING: {
        string* v = (string*) val;
        obj->Set(NanNew<String>(col->name.c_str()), NanNew<String>(v->c_str()));
        delete v;
      }
        break;
      case VALUE_TYPE_NUMBER: {
        oracle::occi::Number* v = (oracle::occi::Number*) val;
        obj->Set(NanNew<String>(col->name.c_str()), NanNew<Number>((double) (*v)));
        delete v;
      }
        break;
      case VALUE_TYPE_DATE: {
        oracle::occi::Date* v = (oracle::occi::Date*) val;
        obj->Set(NanNew<String>(col->name.c_str()), OracleDateToV8Date(v));
        delete v;
      }
        break;
      case VALUE_TYPE_TIMESTAMP: {
        oracle::occi::Timestamp* v = (oracle::occi::Timestamp*) val;
        obj->Set(NanNew<String>(col->name.c_str()), OracleTimestampToV8Date(v));
        delete v;
      }
        break;
      case VALUE_TYPE_CLOB: {
        buffer_t *v = (buffer_t *) val;
        obj->Set(NanNew<String>(col->name.c_str()), NanNew<String>((const char*)v->data, v->length));
        delete[] v->data;
        delete v;
      }
        break;
      case VALUE_TYPE_BLOB: {
        buffer_t *v = (buffer_t *) val;
        // convert to V8 buffer
        v8::Local<v8::Object> v8Buffer = NanBufferUse((char *)v->data, v->length);
        obj->Set(NanNew<String>(col->name.c_str()), v8Buffer);
        delete[] v->data;
        delete v;
        break;
      }
        break;
      default:
        char msg[128];
        snprintf(msg, sizeof(msg), "reateV8ObjectFromRow: Unhandled type: %d",
            col->type);
        std::string strMsg = std::string(msg);
        throw NodeOracleException(strMsg);
        break;
      }
    }
  }
  return obj;
}

Local<Array> Connection::CreateV8ArrayFromRows(vector<column_t*> columns,
    vector<row_t*>* rows) {
  size_t totalRows = rows->size();
  Local<Array> retRows = NanNew<Array>(totalRows);
  uint32_t index = 0;
  for (vector<row_t*>::iterator iterator = rows->begin(), end = rows->end();
      iterator != end; ++iterator, index++) {
    row_t* currentRow = *iterator;
    Local<Object> obj = CreateV8ObjectFromRow(columns, currentRow);
    retRows->Set(index, obj);
  }
  return retRows;
}

void Connection::EIO_AfterExecute(uv_work_t* req) {
  NanScope();
  ExecuteBaton* baton = CONTAINER_OF(req, ExecuteBaton, work_req);

  try {
    Handle<Value> argv[2];
    handleResult(baton, argv);
    baton->callback->Call(2, argv);
  } catch (NodeOracleException &ex) {
    Handle<Value> argv[2];
    argv[0] = NanError(ex.getMessage().c_str());
    argv[1] = NanUndefined();
    baton->callback->Call(2, argv);
  } catch (const exception &ex) {
    Handle<Value> argv[2];
    argv[0] = NanError(ex.what());
    argv[1] = NanUndefined();
    baton->callback->Call(2, argv);
  }

  delete baton;
}

void Connection::handleResult(ExecuteBaton* baton, Handle<Value> (&argv)[2]) {
  try {
    if (baton->error) {
      argv[0] = NanError(baton->error->c_str());
      argv[1] = NanUndefined();
    } else {
      argv[0] = NanUndefined();
      if (baton->rows) {
        argv[1] = CreateV8ArrayFromRows(baton->columns, baton->rows);
      } else {
        Local<Object> obj = NanNew<Object>();
        obj->Set(NanNew<String>("updateCount"), NanNew<Integer>(baton->updateCount));

        /* Note: attempt to keep backward compatability here: existing users of this library will have code that expects a single out param
         called 'returnParam'. For multiple out params, the first output will continue to be called 'returnParam' and subsequent outputs
         will be called 'returnParamX'.
         */
        uint32_t index = 0;
        for (vector<output_t*>::iterator iterator = baton->outputs->begin(),
            end = baton->outputs->end(); iterator != end; ++iterator, index++) {
          output_t* output = *iterator;
          char msg[256];
          if (index > 0) {
            snprintf(msg, sizeof(msg), "returnParam%d", index);
          } else {
            snprintf(msg, sizeof(msg), "returnParam");
          }
          std::string returnParam(msg);
          Local<String> prop = NanNew<String>(returnParam.c_str());
          switch (output->type) {
          case OutParam::OCCIINT:
            obj->Set(prop, NanNew<Integer>(output->intVal));
            break;
          case OutParam::OCCISTRING:
            obj->Set(prop, NanNew<String>(output->strVal.c_str()));
            break;
          case OutParam::OCCIDOUBLE:
            obj->Set(prop, NanNew<Number>(output->doubleVal));
            break;
          case OutParam::OCCIFLOAT:
            obj->Set(prop, NanNew<Number>(output->floatVal));
            break;
          case OutParam::OCCICURSOR:
            obj->Set(prop,
                CreateV8ArrayFromRows(output->columns, output->rows));
            break;
          case OutParam::OCCICLOB: {
            obj->Set(prop, NanNew<String>((const char *)output->bufVal, output->bufLength));
            delete[] output->bufVal;
            break;
          }
          case OutParam::OCCIBLOB: {
            // convert to V8 buffer
            v8::Local<v8::Object> v8Buffer = NanBufferUse((char *)output->bufVal, output->bufLength);
            obj->Set(prop, v8Buffer);
            delete[] output->bufVal;
            break;
          }
          case OutParam::OCCIDATE:
            obj->Set(prop, OracleDateToV8Date(&output->dateVal));
            break;
          case OutParam::OCCITIMESTAMP:
            obj->Set(prop, OracleTimestampToV8Date(&output->timestampVal));
            break;
          case OutParam::OCCINUMBER:
            obj->Set(prop, NanNew<Number>((double) output->numberVal));
            break;
          default:
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown OutParam type: %d",
                output->type);
            std::string strMsg = std::string(msg);
            throw NodeOracleException(strMsg);
            break;
          }
        }
        argv[1] = obj;
      }
    }
  } catch (NodeOracleException &ex) {
    Handle<Value> argv[2];
    argv[0] = NanError(ex.getMessage().c_str());
    argv[1] = NanUndefined();
    baton->callback->Call(2, argv);
  } catch (const std::exception &ex) {
    Handle<Value> argv[2];
    argv[0] = NanError(ex.what());
    argv[1] = NanUndefined();
    baton->callback->Call(2, argv);
  }

}

void Connection::setConnection(oracle::occi::Environment* environment,
    ConnectionPool* _connectionPool,
    oracle::occi::Connection* connection) {
  m_environment = environment;
  connectionPool = _connectionPool;
  if (connectionPool) {
    m_environment = connectionPool->getEnvironment();
  }
  m_connection = connection;
}

NAN_METHOD(Connection::ExecuteSync) {
  NanScope();
  Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());

  REQ_STRING_ARG(0, sql);
  REQ_ARRAY_ARG(1, values);

  String::Utf8Value sqlVal(sql);

  ExecuteBaton* baton = new ExecuteBaton(connection, *sqlVal, values);
  EIO_Execute(&baton->work_req);
  Handle<Value> argv[2];
  handleResult(baton, argv);

  if (baton->error) {
    delete baton;
    return NanThrowError(argv[0]);
  }

  delete baton;
  NanReturnValue(argv[1]);
}


oracle::occi::Statement* Connection::CreateStatement(ExecuteBaton* baton) {
  baton->rows = NULL;
  baton->error = NULL;

  if (! baton->connection->m_connection) {
    baton->error = new std::string("Connection already closed");
    return NULL;
  }
  try {
    oracle::occi::Statement* stmt = baton->connection->m_connection->createStatement(baton->sql);
    stmt->setAutoCommit(baton->connection->m_autoCommit);
    if (baton->connection->m_prefetchRowCount > 0) stmt->setPrefetchRowCount(baton->connection->m_prefetchRowCount);
    return stmt;
  } catch(oracle::occi::SQLException &ex) {
    baton->error = new string(ex.getMessage());
    return NULL;
  }
}

buffer_t* Connection::readClob(oracle::occi::Clob& clobVal, int charForm) {
  clobVal.open(oracle::occi::OCCI_LOB_READONLY);
  switch (charForm) {
  case SQLCS_IMPLICIT:
    clobVal.setCharSetForm(oracle::occi::OCCI_SQLCS_IMPLICIT);
    break;
  case SQLCS_NCHAR:
    clobVal.setCharSetForm(oracle::occi::OCCI_SQLCS_NCHAR);
    break;
  case SQLCS_EXPLICIT:
    clobVal.setCharSetForm(oracle::occi::OCCI_SQLCS_EXPLICIT);
    break;
  case SQLCS_FLEXIBLE:
    clobVal.setCharSetForm(oracle::occi::OCCI_SQLCS_FLEXIBLE);
    break;
  }

  unsigned int lobLength = clobVal.length();
  unsigned char* lob = new unsigned char[lobLength];
  unsigned int totalBytesRead = 0;
  oracle::occi::Stream* instream = clobVal.getStream(1, 0);
  // chunk size is set when the table is created
  size_t chunkSize = clobVal.getChunkSize();
  char* buffer = new char[chunkSize];
  memset(buffer, 0, chunkSize);
  int numBytesRead = 0;
  while (numBytesRead != -1) {
    numBytesRead = instream->readBuffer(buffer, chunkSize);
    if (numBytesRead > 0) {
      memcpy(lob + totalBytesRead, buffer, numBytesRead);
      totalBytesRead += numBytesRead;
    }
  }
  clobVal.closeStream(instream);
  clobVal.close();
  delete[] buffer;

  buffer_t* b = new buffer_t();
  b->data = lob;
  b->length = totalBytesRead;

  return b;
}

buffer_t* Connection::readBlob(oracle::occi::Blob& blobVal) {
  blobVal.open(oracle::occi::OCCI_LOB_READONLY);
  unsigned int lobLength = blobVal.length();
  unsigned char* lob = new unsigned char[lobLength];
  unsigned int totalBytesRead = 0;
  oracle::occi::Stream* instream = blobVal.getStream(1, 0);
  // chunk size is set when the table is created
  size_t chunkSize = blobVal.getChunkSize();
  char* buffer = new char[chunkSize];
  memset(buffer, 0, chunkSize);
  int numBytesRead = 0;
  while (numBytesRead != -1) {
    numBytesRead = instream->readBuffer(buffer, chunkSize);
    if (numBytesRead > 0) {
      memcpy(lob + totalBytesRead, buffer, numBytesRead);
      totalBytesRead += numBytesRead;
    }
  }
  blobVal.closeStream(instream);
  blobVal.close();
  delete[] buffer;

  buffer_t* b = new buffer_t();
  b->data = lob;
  b->length = totalBytesRead;
  return b;
}

void Connection::ExecuteStatement(ExecuteBaton* baton, oracle::occi::Statement* stmt) {
  oracle::occi::ResultSet* rs = NULL;

  int outputParam = SetValuesOnStatement(stmt, baton->values);
  if (baton->error) goto cleanup;

  if (!baton->outputs) baton->outputs = new std::vector<output_t*>();

  try {
    int status = stmt->execute();
    if (status == oracle::occi::Statement::UPDATE_COUNT_AVAILABLE) {
      baton->updateCount = stmt->getUpdateCount();
      if (outputParam >= 0) {
        for (vector<output_t*>::iterator iterator = baton->outputs->begin(), end = baton->outputs->end(); iterator != end; ++iterator) {
          output_t* output = *iterator;
          oracle::occi::ResultSet* rs;
          switch(output->type) {
            case OutParam::OCCIINT:
              output->intVal = stmt->getInt(output->index);
              break;
            case OutParam::OCCISTRING:
              output->strVal = string(stmt->getString(output->index));
              break;
            case OutParam::OCCIDOUBLE:
              output->doubleVal = stmt->getDouble(output->index);
              break;
            case OutParam::OCCIFLOAT:
              output->floatVal = stmt->getFloat(output->index);
              break;
            case OutParam::OCCICURSOR:
              rs = stmt->getCursor(output->index);
              CreateColumnsFromResultSet(rs, output->columns);
              if (baton->error) goto cleanup;
              output->rows = new vector<row_t*>();
              while(rs->next()) {
                row_t* row = CreateRowFromCurrentResultSetRow(rs, output->columns);
                if (baton->error) goto cleanup;
                output->rows->push_back(row);
              }
              break;
            case OutParam::OCCICLOB: {
              output->clobVal = stmt->getClob(output->index);
              buffer_t *buf = Connection::readClob(output->clobVal);
              output->bufVal = buf->data;
              output->bufLength = buf->length;
              delete buf;
              }
              break;
            case OutParam::OCCIBLOB: {
              output->blobVal = stmt->getBlob(output->index);
              buffer_t *buf = Connection::readBlob(output->blobVal);
              output->bufVal = buf->data;
              output->bufLength = buf->length;
              delete buf;
              }
              break;
            case OutParam::OCCIDATE:
              output->dateVal = stmt->getDate(output->index);
              break;
            case OutParam::OCCITIMESTAMP:
              output->timestampVal = stmt->getTimestamp(output->index);
              break;
            case OutParam::OCCINUMBER:
              output->numberVal = stmt->getNumber(output->index);
              break;
            default:
              {
                ostringstream oss;
                oss << "Unknown OutParam type: " << output->type;
                baton->error = new std::string(oss.str());
                goto cleanup;
              }
          }
        }
      }
    } else if (status == oracle::occi::Statement::RESULT_SET_AVAILABLE) {
      rs = stmt->getResultSet();
      CreateColumnsFromResultSet(rs, baton->columns);
      if (baton->error) goto cleanup;
      baton->rows = new vector<row_t*>();

      while(rs->next()) {
        row_t* row = CreateRowFromCurrentResultSetRow(rs, baton->columns);
        if (baton->error) goto cleanup;
        baton->rows->push_back(row);
      }
    }
  } catch (oracle::occi::SQLException &ex) {
    baton->error = new string(ex.getMessage());
  } catch (NodeOracleException &ex) {
    baton->error = new string(ex.getMessage());
  } catch (const exception& ex) {
    baton->error = new string(ex.what());
  } catch (...) {
    baton->error = new string("Unknown exception thrown from OCCI");
  }
cleanup:
  if (rs) {
    stmt->closeResultSet(rs);
    rs = NULL;
  }
}

