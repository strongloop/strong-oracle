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

Nan::Persistent<FunctionTemplate> ConnectionPool::s_ct;
Nan::Persistent<FunctionTemplate> Connection::s_ct;

// ConnectionPool implementation
ConnectionPool::ConnectionPool() :
    m_environment(NULL), m_connectionPool(NULL) {
}

ConnectionPool::~ConnectionPool() {
  /*
  try {
    closeConnectionPool(oracle::occi::StatelessConnectionPool::SPD_FORCE);
  } catch (std::exception &ex) {
    m_connectionPool = NULL;
    fprintf(stderr, "%s\n", ex.what());
  }
  */
}

void ConnectionPool::Init(Handle<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(ConnectionPool::New);
  ConnectionPool::s_ct.Reset( t);

  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(Nan::New<String>("ConnectionPool").ToLocalChecked());

  Nan::SetPrototypeMethod(t, "getConnectionSync", ConnectionPool::GetConnectionSync);
  Nan::SetPrototypeMethod(t, "getConnection", ConnectionPool::GetConnection);
  Nan::SetPrototypeMethod(t, "execute", ConnectionPool::Execute);
  Nan::SetPrototypeMethod(t, "close", ConnectionPool::Close);
  Nan::SetPrototypeMethod(t, "closeSync", ConnectionPool::CloseSync);
  Nan::SetPrototypeMethod(t, "getInfo", ConnectionPool::GetInfo);

  target->Set(Nan::New<String>("ConnectionPool").ToLocalChecked(), t->GetFunction());
}

NAN_METHOD(ConnectionPool::New) {
  Nan::HandleScope scope;

  ConnectionPool *connectionPool = new ConnectionPool();
  connectionPool->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void ConnectionPool::setConnectionPool(oracle::occi::Environment* environment,
    oracle::occi::StatelessConnectionPool* connectionPool) {
  m_environment = environment;
  m_connectionPool = connectionPool;
}

NAN_METHOD(ConnectionPool::CloseSync) {
  Nan::HandleScope scope;
  try {
    ConnectionPool* connectionPool = Nan::ObjectWrap::Unwrap<ConnectionPool>(
        info.This());

    // Get the optional destroy mode
    oracle::occi::StatelessConnectionPool::DestroyMode mode =
        oracle::occi::StatelessConnectionPool::DEFAULT;
    if (info.Length() == 1 || info[0]->IsUint32()) {
      mode =
          static_cast<oracle::occi::StatelessConnectionPool::DestroyMode>(info[0]->Uint32Value());
    }

    connectionPool->closeConnectionPool(mode);
    connectionPool->Unref();

    return;
  } catch (const exception& ex) {
    return Nan::ThrowError(ex.what());
  }
}

NAN_METHOD(ConnectionPool::Close) {
  Nan::HandleScope scope;
  ConnectionPool* connectionPool = Nan::ObjectWrap::Unwrap<ConnectionPool>(
        info.This());

  // Get the optional destroy mode
  oracle::occi::StatelessConnectionPool::DestroyMode mode =
      oracle::occi::StatelessConnectionPool::DEFAULT;
  if (info.Length() >= 1 && info[0]->IsUint32()) {
    mode =
        static_cast<oracle::occi::StatelessConnectionPool::DestroyMode>(info[0]->Uint32Value());
  }

  Local<Function> callback;
  if (info.Length() == 1 && info[0]->IsFunction()) {
    callback = Local<Function>::Cast(info[0]);
  } else if (info.Length() > 1 && info[1]->IsFunction()) {
    callback = Local<Function>::Cast(info[1]);
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

  return;
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
  Nan::HandleScope scope;
  ConnectionPoolBaton* baton = CONTAINER_OF(req, ConnectionPoolBaton, work_req);
  baton->connectionPool->Unref();

  Nan::TryCatch tryCatch;
  if (baton->callback != NULL) {
    Local<Value> argv[2];
    if (baton->error) {
      argv[0] = Nan::Error(baton->error->c_str());
      argv[1] = Nan::Undefined();
    } else {
      argv[0] = Nan::Undefined();
      argv[1] = Nan::Undefined();
    }
    baton->callback->Call(2, argv);
  }
  delete baton;

  if (tryCatch.HasCaught()) {
    Nan::FatalException(tryCatch);
  }
}

NAN_METHOD(ConnectionPool::GetInfo) {
  Nan::HandleScope scope;
  ConnectionPool* connectionPool = Nan::ObjectWrap::Unwrap<ConnectionPool>(
      info.This());
  if (connectionPool->m_connectionPool) {
    Local<Object> obj = Nan::New<Object>();

    obj->Set(Nan::New<String>("openConnections").ToLocalChecked(),
        Nan::New<Uint32>(connectionPool->m_connectionPool->getOpenConnections()));
    obj->Set(Nan::New<String>("busyConnections").ToLocalChecked(),
        Nan::New<Uint32>(connectionPool->m_connectionPool->getBusyConnections()));
    obj->Set(Nan::New<String>("maxConnections").ToLocalChecked(),
        Nan::New<Uint32>(connectionPool->m_connectionPool->getMaxConnections()));
    obj->Set(Nan::New<String>("minConnections").ToLocalChecked(),
        Nan::New<Uint32>(connectionPool->m_connectionPool->getMinConnections()));
    obj->Set(Nan::New<String>("incrConnections").ToLocalChecked(),
        Nan::New<Uint32>(connectionPool->m_connectionPool->getIncrConnections()));
    obj->Set(Nan::New<String>("busyOption").ToLocalChecked(),
        Nan::New<Uint32>(static_cast<unsigned int>(connectionPool->m_connectionPool->getBusyOption())));
    obj->Set(Nan::New<String>("timeout").ToLocalChecked(),
        Nan::New<Uint32>(connectionPool->m_connectionPool->getTimeOut()));

    obj->Set(Nan::New<String>("poolName").ToLocalChecked(),
        Nan::New<String>(connectionPool->m_connectionPool->getPoolName().c_str()).ToLocalChecked());

    info.GetReturnValue().Set(obj);
  } else {
    return;
  }
}

NAN_METHOD(ConnectionPool::GetConnectionSync) {
  Nan::HandleScope scope;
  try {
    ConnectionPool* connectionPool = Nan::ObjectWrap::Unwrap<ConnectionPool>(
        info.This());
    // std::string tag = "strong-oracle";
    oracle::occi::Connection *conn =
        connectionPool->getConnectionPool()->getConnection("strong-oracle");
    Local<FunctionTemplate> ft = Nan::New(Connection::s_ct);
    Local<Object> connection = ft->GetFunction()->NewInstance();
    (Nan::ObjectWrap::Unwrap<Connection>(connection))->setConnection(
        connectionPool->getEnvironment(),
        connectionPool,
        conn);
    info.GetReturnValue().Set(connection);
  } catch (const exception& ex) {
    return Nan::ThrowError(ex.what());
  }
}

NAN_METHOD(ConnectionPool::GetConnection) {
  Nan::HandleScope scope;
  ConnectionPool* connectionPool = Nan::ObjectWrap::Unwrap<ConnectionPool>(
      info.This());

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

  return;
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
  Nan::HandleScope scope;
  ConnectionPoolBaton* baton = CONTAINER_OF(req, ConnectionPoolBaton, work_req);

  Local<Value> argv[2];
  if (baton->error) {
    argv[0] = Nan::Error(baton->error->c_str());
    argv[1] = Nan::Undefined();
  } else {
    argv[0] = Nan::Undefined();
    Local<FunctionTemplate> ft = Nan::New(Connection::s_ct);
    Local<Object> connection = ft->GetFunction()->NewInstance();
    (Nan::ObjectWrap::Unwrap<Connection>(connection))->setConnection(
        baton->connectionPool->getEnvironment(),
        baton->connectionPool,
        baton->connection);
    argv[1] = connection;
  }

  Nan::TryCatch tryCatch;
  baton->callback->Call(2, argv);
  delete baton;

  if (tryCatch.HasCaught()) {
    Nan::FatalException(tryCatch);
  }

}

void ConnectionPool::closeConnectionPool(
    oracle::occi::StatelessConnectionPool::DestroyMode mode) {
  if (m_environment && m_connectionPool) {
    m_environment->terminateStatelessConnectionPool(m_connectionPool, mode);
    m_connectionPool = NULL;
  }
}

NAN_METHOD(ConnectionPool::Execute) {
  Nan::HandleScope scope;
  ConnectionPool* connectionPool = Nan::ObjectWrap::Unwrap<ConnectionPool>(info.This());

  REQ_STRING_ARG(0, sql);
  REQ_ARRAY_ARG(1, values);
  Local<Object> options;
  int cbIndex = 2;
  if (info.Length() > 3 && info[2]->IsObject() && !info[2]->IsFunction())
  {
    options = Local<Object>::Cast(info[2]);
    ++cbIndex;
  }
  if (info.Length() <= cbIndex || !info[cbIndex]->IsFunction())
  {
    ostringstream oss;
    oss << "Argument " << cbIndex << " must be a function";
    std::string strMsg = std::string(oss.str().c_str());
    throw NodeOracleException(strMsg);
  }
  Local<Function> callback = Local<Function>::Cast(info[cbIndex]);

  String::Utf8Value sqlVal(sql);

  ExecuteBaton* baton = new ExecuteBaton(connectionPool->m_environment,
                                         connectionPool->m_connectionPool,
                                         NULL,
                                         true,
                                         10,
                                         *sqlVal, values, options,
                                         callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                ConnectionPool::EIO_Execute,
                (uv_after_work_cb) ConnectionPool::EIO_AfterExecute);

  return;
}

void ConnectionPool::EIO_Execute(uv_work_t* req) {
  ExecuteBaton* baton = CONTAINER_OF(req, ExecuteBaton, work_req);

  if(!baton->m_connection) {
    oracle::occi::Connection *conn =
        baton->m_connectionPool->getConnection("strong-oracle");
    baton->m_connection = conn;
  }
  oracle::occi::Statement* stmt = Connection::CreateStatement(baton);
  if (baton->error) return;

  Connection::ExecuteStatement(baton, stmt);

  if (stmt) {
    if (baton->m_connection) {
      baton->m_connection->terminateStatement(stmt);
    }
    stmt = NULL;
  }
  if (baton->m_connectionPool && baton->m_connection) {
    baton->m_connectionPool->releaseConnection(baton->m_connection,
        "strong-oracle");
    baton->m_connection = NULL;
  }
}

void ConnectionPool::EIO_AfterExecute(uv_work_t* req) {
  Nan::HandleScope scope;
  ExecuteBaton* baton = CONTAINER_OF(req, ExecuteBaton, work_req);

  try {
    Local<Value> argv[2];
    Connection::handleResult(baton, argv);
    baton->callback->Call(2, argv);
  } catch (NodeOracleException &ex) {
    Local<Value> argv[2];
    argv[0] = Nan::Error(ex.getMessage().c_str());
    argv[1] = Nan::Undefined();
    baton->callback->Call(2, argv);
  } catch (const exception &ex) {
    Local<Value> argv[2];
    argv[0] = Nan::Error(ex.what());
    argv[1] = Nan::Undefined();
    baton->callback->Call(2, argv);
  }

  delete baton;
}

void Connection::Init(Handle<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(Connection::New);
  Connection::s_ct.Reset( t);

  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(Nan::New<String>("Connection").ToLocalChecked());

  Nan::SetPrototypeMethod(t, "execute", Connection::Execute);
  Nan::SetPrototypeMethod(t, "executeSync", ExecuteSync);
  Nan::SetPrototypeMethod(t, "readerHandle", CreateReader);
  Nan::SetPrototypeMethod(t, "prepare", Prepare);
  Nan::SetPrototypeMethod(t, "close", Connection::Close);
  Nan::SetPrototypeMethod(t, "closeSync", Connection::CloseSync);
  Nan::SetPrototypeMethod(t, "isConnected", IsConnected);
  Nan::SetPrototypeMethod(t, "setAutoCommit", SetAutoCommit);
  Nan::SetPrototypeMethod(t, "setPrefetchRowCount", SetPrefetchRowCount);
  Nan::SetPrototypeMethod(t, "commit", Commit);
  Nan::SetPrototypeMethod(t, "rollback", Rollback);

  target->Set(Nan::New<String>("Connection").ToLocalChecked(), t->GetFunction());
}

NAN_METHOD(Connection::New) {
  Nan::HandleScope scope;

  Connection *connection = new Connection();
  connection->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Connection::Prepare) {
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());

  REQ_STRING_ARG(0, sql);

  String::Utf8Value sqlVal(sql);

  StatementBaton* baton = new StatementBaton(connection->m_environment, NULL, connection->m_connection,
      connection->getAutoCommit(), connection->getPrefetchRowCount(),
      *sqlVal);
  Local<FunctionTemplate> ft = Nan::New(Statement::s_ct);
  Local<Object> statementHandle = ft->GetFunction()->NewInstance();
  Statement* statement = Nan::ObjectWrap::Unwrap<Statement>(statementHandle);
  statement->setBaton(baton);

  info.GetReturnValue().Set(statementHandle);
}

NAN_METHOD(Connection::CreateReader) {
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());

  REQ_STRING_ARG(0, sql);
  REQ_ARRAY_ARG(1, values);

  String::Utf8Value sqlVal(sql);

  ReaderBaton* baton = new ReaderBaton(connection->m_environment, NULL, connection->m_connection,
      connection->getAutoCommit(), connection->getPrefetchRowCount(),
      *sqlVal, values);

  Local<FunctionTemplate> ft = Nan::New(Reader::s_ct);
  Local<Object> readerHandle = ft->GetFunction()->NewInstance();
  Reader* reader = Nan::ObjectWrap::Unwrap<Reader>(readerHandle);
  reader->setBaton(baton);

  info.GetReturnValue().Set(readerHandle);
}

Connection::Connection() :
    m_environment(NULL), connectionPool(NULL), m_connection(NULL),
    m_autoCommit(true), m_prefetchRowCount(0) {
}

Connection::~Connection() {
  /*
  try {
    closeConnection();
  } catch (std::exception &ex) {
    m_connection = NULL;
    fprintf(stderr, "%s\n", ex.what());
  }
  */
}

NAN_METHOD(Connection::Execute) {
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());

  REQ_STRING_ARG(0, sql);
  REQ_ARRAY_ARG(1, values);
  Local<Object> options;
  int cbIndex = 2;
  if (info.Length() > 3 && info[2]->IsObject() && !info[2]->IsFunction())
  {
    options = Local<Object>::Cast(info[2]);
    ++cbIndex;
  }
  if (info.Length() <= cbIndex || !info[cbIndex]->IsFunction())
  {
    ostringstream oss;
    oss << "Argument " << cbIndex << " must be a function";
    std::string strMsg = std::string(oss.str().c_str());
    throw NodeOracleException(strMsg);
  }
  Local<Function> callback = Local<Function>::Cast(info[cbIndex]);

  String::Utf8Value sqlVal(sql);

  ExecuteBaton* baton = new ExecuteBaton(connection->m_environment,
                                         NULL,
                                         connection->m_connection,
                                         connection->getAutoCommit(),
                                         connection->getPrefetchRowCount(),
                                         *sqlVal, values, options,
                                         callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_Execute,
                (uv_after_work_cb) EIO_AfterExecute);

  return;
}

NAN_METHOD(Connection::CloseSync) {
  Nan::HandleScope scope;
  try {
    Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());
    connection->closeConnection();
    connection->Unref();

    return;
  } catch (const exception& ex) {
    return Nan::ThrowError(ex.what());
  }
}

NAN_METHOD(Connection::IsConnected) {
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());

  if (connection && connection->m_connection) {
    info.GetReturnValue().Set(Nan::New<Boolean>(true));
  } else {
    info.GetReturnValue().Set(Nan::New<Boolean>(false));
  }
}

NAN_METHOD(Connection::Commit) {
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());

  REQ_FUN_ARG(0, callback);

  ConnectionBaton* baton = new ConnectionBaton(connection, callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_Commit,
                (uv_after_work_cb) EIO_AfterCommit);

  return;
}

NAN_METHOD(Connection::Rollback) {
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());

  REQ_FUN_ARG(0, callback);

  ConnectionBaton* baton = new ConnectionBaton(connection, callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_Rollback,
                (uv_after_work_cb) EIO_AfterRollback);

  return;
}

NAN_METHOD(Connection::SetAutoCommit) {
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());
  REQ_BOOL_ARG(0, autoCommit);
  connection->m_autoCommit = autoCommit;
  return;
}

NAN_METHOD(Connection::SetPrefetchRowCount) {
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());
  REQ_INT_ARG(0, prefetchRowCount);
  connection->m_prefetchRowCount = prefetchRowCount;
  return;
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
  Nan::HandleScope scope;
  ConnectionBaton* baton = CONTAINER_OF(req, ConnectionBaton, work_req);

  Nan::TryCatch tryCatch;
  if (baton->callback != NULL) {
    Local<Value> argv[2];
    if (baton->error) {
      argv[0] = Nan::Error(baton->error->c_str());
      argv[1] = Nan::Undefined();
    } else {
      argv[0] = Nan::Undefined();
      argv[1] = Nan::Undefined();
    }
    baton->callback->Call(2, argv);
  }
  delete baton;

  if (tryCatch.HasCaught()) {
    Nan::FatalException(tryCatch);
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
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());

  Local<Function> callback;
  if (info.Length() > 0 && info[0]->IsFunction()) {
    callback = Local<Function>::Cast(info[0]);
  }

  ConnectionBaton* baton = new ConnectionBaton(connection, callback);
  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                Connection::EIO_Close,
                (uv_after_work_cb) Connection::EIO_AfterClose);

  return;
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
  Nan::HandleScope scope;
  ConnectionBaton* baton = CONTAINER_OF(req, ConnectionBaton, work_req);
  baton->connection->Unref();
  Nan::TryCatch tryCatch;
  if (baton->callback != NULL) {
    Local<Value> argv[2];
    if (baton->error) {
      argv[0] = Nan::Error(baton->error->c_str());
      argv[1] = Nan::Undefined();
    } else {
      argv[0] = Nan::Undefined();
      argv[1] = Nan::Undefined();
    }
    baton->callback->Call(2, argv);
  }
  delete baton;

  if (tryCatch.HasCaught()) {
    Nan::FatalException(tryCatch);
  }
}

void Connection::EIO_Execute(uv_work_t* req) {
  ExecuteBaton* baton = CONTAINER_OF(req, ExecuteBaton, work_req);

  oracle::occi::Statement* stmt = CreateStatement(baton);
  if (baton->error) return;

  ExecuteStatement(baton, stmt);

  if (stmt) {
    if (baton->m_connection) {
      baton->m_connection->terminateStatement(stmt);
    }
    stmt = NULL;
  }
}

void CallDateMethod(v8::Local<v8::Date> date, const char* methodName, int val) {
  Local<Value> info[1];
  info[0] = Nan::New<Number>(val);
  Local<Function>::Cast(date->Get(Nan::New<String>(methodName).ToLocalChecked()))->Call(date, 1,
      info);
}

Local<Date> OracleDateToV8Date(oracle::occi::Date* d) {
  int year;
  unsigned int month, day, hour, min, sec;
  d->getDate(year, month, day, hour, min, sec);
  Local<Date> date = Nan::New<Date>(0.0).ToLocalChecked();
  CallDateMethod(date, "setUTCFullYear", year);
  CallDateMethod(date, "setUTCMonth", month - 1);
  CallDateMethod(date, "setUTCDate", day);
  CallDateMethod(date, "setUTCHours", hour);
  CallDateMethod(date, "setUTCMinutes", min);
  CallDateMethod(date, "setUTCSeconds", sec);
  CallDateMethod(date, "setUTCMilliseconds", 0);
  return date;
}

Local<Date> OracleTimestampToV8Date(oracle::occi::Timestamp* d) {
  int year;
  unsigned int month, day, hour, min, sec, fs, ms;
  d->getDate(year, month, day);
  d->getTime(hour, min, sec, fs);
  Local<Date> date = Nan::New<Date>(0.0).ToLocalChecked();
  //occi always returns nanoseconds, regardless of precision set on timestamp column
  ms = (fs / 1000000.0) + 0.5; // add 0.5 to round to nearest millisecond

  CallDateMethod(date, "setUTCFullYear", year);
  CallDateMethod(date, "setUTCMonth", month - 1);
  CallDateMethod(date, "setUTCDate", day);
  CallDateMethod(date, "setUTCHours", hour);
  CallDateMethod(date, "setUTCMinutes", min);
  CallDateMethod(date, "setUTCSeconds", sec);
  CallDateMethod(date, "setUTCMilliseconds", ms);
  return date;
}

Local<Object> Connection::CreateV8ObjectFromRow(vector<column_t*> columns,
    row_t* currentRow) {
  Local<Object> obj = Nan::New<Object>();
  uint32_t colIndex = 0;
  for (vector<column_t*>::iterator iterator = columns.begin(), end =
      columns.end(); iterator != end; ++iterator, colIndex++) {
    column_t* col = *iterator;
    void* val = currentRow->values[colIndex];
    Local<String> colName = Nan::New<String>(col->name.c_str()).ToLocalChecked();
    if (val == NULL) {
      obj->Set(colName, Nan::Null());
    } else {
      switch (col->type) {
      case VALUE_TYPE_STRING: {
        string* v = (string*) val;
        obj->Set(colName, Nan::New<String>(v->c_str()).ToLocalChecked());
        delete v;
      }
        break;
      case VALUE_TYPE_NUMBER: {
        oracle::occi::Number* v = (oracle::occi::Number*) val;
        obj->Set(colName, Nan::New<Number>((double) (*v)));
        delete v;
      }
        break;
      case VALUE_TYPE_DATE: {
        oracle::occi::Date* v = (oracle::occi::Date*) val;
        obj->Set(colName, OracleDateToV8Date(v));
        delete v;
      }
        break;
      case VALUE_TYPE_TIMESTAMP: {
        oracle::occi::Timestamp* v = (oracle::occi::Timestamp*) val;
        obj->Set(colName, OracleTimestampToV8Date(v));
        delete v;
      }
        break;
      case VALUE_TYPE_CLOB: {
        buffer_t *v = (buffer_t *) val;
        obj->Set(colName, Nan::New<String>((const char*)v->data, v->length).ToLocalChecked());
        delete[] v->data;
        delete v;
      }
        break;
      case VALUE_TYPE_BLOB: {
        buffer_t *v = (buffer_t *) val;
        // convert to V8 buffer
        v8::Local<v8::Object> v8Buffer = Nan::NewBuffer((char *)v->data, (uint32_t)v->length).ToLocalChecked();
        obj->Set(colName, v8Buffer);
        // Nan will free the memory of the buffer
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
  Local<Array> retRows = Nan::New<Array>(totalRows);
  uint32_t index = 0;
  for (vector<row_t*>::iterator iterator = rows->begin(), end = rows->end();
      iterator != end; ++iterator, index++) {
    row_t* currentRow = *iterator;
    Local<Object> obj = CreateV8ObjectFromRow(columns, currentRow);
    retRows->Set(index, obj);
  }
  return retRows;
}

Local<Array> Connection::CreateV8ArrayFromCols(std::vector<column_t*> columns)
{
  Local<Array> v8cols = Nan::New<Array>(columns.size());
  uint32_t index = 0;
  for (std::vector<column_t*>::iterator iterator = columns.begin(), end =columns.end(); iterator != end; ++iterator, ++index)
  {
     column_t* col = *iterator;
     Local<Object> v8col = Nan::New<Object>();
     v8col->Set(Nan::New<String>("name").ToLocalChecked(), Nan::New<String>(col->name.c_str()).ToLocalChecked());
     v8col->Set(Nan::New<String>("type").ToLocalChecked(), Nan::New<Number>((double)(col->type)));
     v8cols->Set(index, v8col);
  }
  return v8cols;
}

void Connection::EIO_AfterExecute(uv_work_t* req) {
  Nan::HandleScope scope;
  ExecuteBaton* baton = CONTAINER_OF(req, ExecuteBaton, work_req);

  try {
    Local<Value> argv[2];
    handleResult(baton, argv);
    baton->callback->Call(2, argv);
  } catch (NodeOracleException &ex) {
    Local<Value> argv[2];
    argv[0] = Nan::Error(ex.getMessage().c_str());
    argv[1] = Nan::Undefined();
    baton->callback->Call(2, argv);
  } catch (const exception &ex) {
    Local<Value> argv[2];
    argv[0] = Nan::Error(ex.what());
    argv[1] = Nan::Undefined();
    baton->callback->Call(2, argv);
  }

  delete baton;
}

void Connection::handleResult(ExecuteBaton* baton, Local<Value> (&argv)[2]) {
  try {
    if (baton->error) {
      argv[0] = Nan::Error(baton->error->c_str());
      argv[1] = Nan::Undefined();
    } else {
      argv[0] = Nan::Undefined();
      if (baton->rows) {
        Local<Object> obj = CreateV8ArrayFromRows(baton->columns, baton->rows);
        if (baton->getColumnMetaData) {
          obj->Set(Nan::New<String>("columnMetaData").ToLocalChecked(),
                   CreateV8ArrayFromCols(baton->columns));
        }
        argv[1] = obj;
      } else {
        Local<Object> obj = Nan::New<Object>();
        obj->Set(Nan::New<String>("updateCount").ToLocalChecked(), Nan::New<Integer>(baton->updateCount));

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
          Local<String> prop = Nan::New<String>(returnParam.c_str()).ToLocalChecked();
          switch (output->type) {
          case OutParam::OCCIINT:
            obj->Set(prop, Nan::New<Integer>(output->intVal));
            break;
          case OutParam::OCCISTRING:
            obj->Set(prop, Nan::New<String>(output->strVal.c_str()).ToLocalChecked());
            break;
          case OutParam::OCCIDOUBLE:
            obj->Set(prop, Nan::New<Number>(output->doubleVal));
            break;
          case OutParam::OCCIFLOAT:
            obj->Set(prop, Nan::New<Number>(output->floatVal));
            break;
          case OutParam::OCCICURSOR:
            obj->Set(prop,
                CreateV8ArrayFromRows(output->columns, output->rows));
            break;
          case OutParam::OCCICLOB: {
            obj->Set(prop, Nan::New<String>((const char *)output->bufVal, output->bufLength).ToLocalChecked());
            delete[] output->bufVal;
            break;
          }
          case OutParam::OCCIBLOB: {
            // convert to V8 buffer
            v8::Local<v8::Object> v8Buffer = Nan::NewBuffer((char *)output->bufVal, output->bufLength).ToLocalChecked();
            obj->Set(prop, v8Buffer);
            // Memory will be freed by NewBuffer GC
            break;
          }
          case OutParam::OCCIDATE:
            obj->Set(prop, OracleDateToV8Date(&output->dateVal));
            break;
          case OutParam::OCCITIMESTAMP:
            obj->Set(prop, OracleTimestampToV8Date(&output->timestampVal));
            break;
          case OutParam::OCCINUMBER:
            obj->Set(prop, Nan::New<Number>((double) output->numberVal));
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
    Local<Value> argv[2];
    argv[0] = Nan::Error(ex.getMessage().c_str());
    argv[1] = Nan::Undefined();
    baton->callback->Call(2, argv);
  } catch (const std::exception &ex) {
    Local<Value> argv[2];
    argv[0] = Nan::Error(ex.what());
    argv[1] = Nan::Undefined();
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
  Nan::HandleScope scope;
  Connection* connection = Nan::ObjectWrap::Unwrap<Connection>(info.This());

  REQ_STRING_ARG(0, sql);
  REQ_ARRAY_ARG(1, values);
  Local<Object> options;
  if (info.Length() > 2 && info[2]->IsObject() && !info[2]->IsFunction())
    options = Local<Object>::Cast(info[2]);

  String::Utf8Value sqlVal(sql);

  ExecuteBaton* baton = new ExecuteBaton(connection->m_environment, NULL, connection->m_connection,
      connection->getAutoCommit(), connection->getPrefetchRowCount(),
      *sqlVal, values, options);
  EIO_Execute(&baton->work_req);
  Local<Value> argv[2];
  handleResult(baton, argv);

  if (baton->error) {
    delete baton;
    return Nan::ThrowError(argv[0]);
  }

  delete baton;
  info.GetReturnValue().Set(argv[1]);
}


oracle::occi::Statement* Connection::CreateStatement(ExecuteBaton* baton) {
  baton->rows = NULL;
  baton->error = NULL;

  if (! baton->m_connection) {
    baton->error = new std::string("Connection already closed");
    return NULL;
  }
  try {
    oracle::occi::Statement* stmt = baton->m_connection->createStatement(baton->sql);
    stmt->setAutoCommit(baton->m_autoCommit);
    if (baton->m_prefetchRowCount > 0) {
      stmt->setPrefetchRowCount(baton->m_prefetchRowCount);
    }
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

  unsigned int clobCharCount = clobVal.length();
  // maximum 4 bytes in a encoded character so the buffer is 4 times as large
  unsigned char* lob = new unsigned char[clobCharCount * 4];
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
              if (baton->m_prefetchRowCount > 0) {
                rs->setPrefetchRowCount(baton->m_prefetchRowCount);
              }
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
      if (baton->m_prefetchRowCount > 0) {
        rs->setPrefetchRowCount(baton->m_prefetchRowCount);
      }
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

