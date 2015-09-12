
#include "oracle_bindings.h"
#include "connection.h"
#include "outParam.h"
#include "reader.h"
#include "statement.h"
#include <iostream>

Nan::Persistent<FunctionTemplate> OracleClient::s_ct;

// ConnectBaton implementation
ConnectBaton::ConnectBaton(OracleClient* client,
                           oracle::occi::Environment* environment,
                           v8::Local<v8::Function> callback) :
    callback(callback) {
  this->client = client;
  this->environment = environment;
  this->error = NULL;
  this->connection = NULL;
  this->connectionPool = NULL;

  this->hostname = "127.0.0.1";
  this->user = "";
  this->password = "";
  this->database = "";

  this->port = 1521;
  this->minConn = 0;
  this->maxConn = 4;
  this->incrConn = 1;
  this->timeout =0;
  this->busyOption = oracle::occi::StatelessConnectionPool::WAIT;
  this->stmtCacheSize = 0;
}

ConnectBaton::~ConnectBaton() {
  if(error) {
    delete error;
  }
}

// OracleClient implementation
OracleClient::OracleClient(oracle::occi::Environment::Mode mode) {
  m_environment = oracle::occi::Environment::createEnvironment(mode);
}

OracleClient::~OracleClient() {
  oracle::occi::Environment::terminateEnvironment(m_environment);
}

void OracleClient::Init(Handle<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(New);
  OracleClient::s_ct.Reset( t);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(Nan::New<String>("OracleClient").ToLocalChecked());

  Nan::SetPrototypeMethod(t, "connect", Connect);
  Nan::SetPrototypeMethod(t, "connectSync", ConnectSync);
  Nan::SetPrototypeMethod(t, "createConnectionPoolSync", CreateConnectionPoolSync);
  Nan::SetPrototypeMethod(t, "createConnectionPool", CreateConnectionPool);

  target->Set(Nan::New<String>("OracleClient").ToLocalChecked(), t->GetFunction());
}

NAN_METHOD(OracleClient::New) {
  Nan::HandleScope scope;

  OracleClient *client = NULL;
  if(info.Length() == 0) {
    client = new OracleClient();
  } else {
    client = new OracleClient(oracle::occi::Environment::USE_LDAP);
  }
  client->Wrap(info.This());
  if(info.Length() > 0) {
    REQ_OBJECT_ARG(0, ldap);
    std::string adminContext, host, user, password;
    int port;
    OBJ_GET_STRING(ldap, "adminContext", adminContext);
    OBJ_GET_STRING(ldap, "host", host);
    OBJ_GET_NUMBER(ldap, "port", port, 389);
    OBJ_GET_STRING(ldap, "user", user);
    OBJ_GET_STRING(ldap, "password", password);
    client->getEnvironment()->setLDAPAdminContext(adminContext);
    client->getEnvironment()->setLDAPAuthentication(0x1);
    client->getEnvironment()->setLDAPHostAndPort(host, port);
    client->getEnvironment()->setLDAPLoginNameAndPassword(user, password);
  }
  info.GetReturnValue().Set(info.This());
}


NAN_METHOD(OracleClient::Connect) {
  Nan::HandleScope scope;

  REQ_OBJECT_ARG(0, settings);
  REQ_FUN_ARG(1, callback);

  OracleClient* client = Nan::ObjectWrap::Unwrap<OracleClient>(info.This());
  ConnectBaton* baton = new ConnectBaton(client, client->m_environment, callback);

  OBJ_GET_STRING(settings, "hostname", baton->hostname);
  OBJ_GET_STRING(settings, "user", baton->user);
  OBJ_GET_STRING(settings, "password", baton->password);
  OBJ_GET_STRING(settings, "database", baton->database);
  OBJ_GET_NUMBER(settings, "port", baton->port, 1521);
  OBJ_GET_STRING(settings, "tns", baton->tns);
  OBJ_GET_NUMBER(settings, "stmtCacheSize", baton->stmtCacheSize, 16);

  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_Connect,
                (uv_after_work_cb) EIO_AfterConnect);

  return;
}

void OracleClient::EIO_Connect(uv_work_t* req) {
  ConnectBaton* baton = CONTAINER_OF(req, ConnectBaton, work_req);

  baton->error = NULL;

  try {
    char connectionStr[512];
    if (baton->tns != "") {
      snprintf(connectionStr, sizeof(connectionStr), "%s", baton->tns.c_str());
    } else {
      snprintf(connectionStr, sizeof(connectionStr), "//%s:%d/%s", baton->hostname.c_str(), baton->port, baton->database.c_str());
    }
    std::string connStr = std::string(connectionStr);
    baton->connection = baton->environment->createConnection(baton->user, baton->password, connStr);
    baton->connection->setStmtCacheSize(baton->stmtCacheSize);
  } catch(oracle::occi::SQLException &ex) {
    baton->error = new std::string(ex.getMessage());
  } catch (const std::exception& ex) {
    baton->error = new std::string(ex.what());
  }
  catch (...) {
    baton->error = new std::string("Unknown exception thrown from OCCI");
  }
}

void OracleClient::EIO_AfterConnect(uv_work_t* req) {
  Nan::HandleScope scope;
  ConnectBaton* baton = CONTAINER_OF(req, ConnectBaton, work_req);

  Local<Value> argv[2];
  if(baton->error) {
    argv[0] = Exception::Error(Nan::New<String>(baton->error->c_str()).ToLocalChecked());
    argv[1] = Nan::Undefined();
  } else {
    argv[0] = Nan::Undefined();
    Local<FunctionTemplate> ft = Nan::New(Connection::s_ct);
    Local<Object> connection = ft->GetFunction()->NewInstance();
    (Nan::ObjectWrap::Unwrap<Connection>(connection))->setConnection(baton->environment, NULL, baton->connection);
    argv[1] = connection;
  }

  Nan::TryCatch tryCatch;

  baton->callback.Call(2, argv);
  delete baton;

  if (tryCatch.HasCaught()) {
    Nan::FatalException(tryCatch);
  }
}

NAN_METHOD(OracleClient::ConnectSync) {
  Nan::HandleScope scope;

  REQ_OBJECT_ARG(0, settings);

  OracleClient* client = Nan::ObjectWrap::Unwrap<OracleClient>(info.This());
  ConnectBaton baton(client, client->m_environment);

  OBJ_GET_STRING(settings, "hostname", baton.hostname);
  OBJ_GET_STRING(settings, "user", baton.user);
  OBJ_GET_STRING(settings, "password", baton.password);
  OBJ_GET_STRING(settings, "database", baton.database);
  OBJ_GET_NUMBER(settings, "port", baton.port, 1521);
  OBJ_GET_NUMBER(settings, "stmtCacheSize", baton.stmtCacheSize, 16);

  try {
    char connectionStr[512];
    snprintf(connectionStr, sizeof(connectionStr), "//%s:%d/%s", baton.hostname.c_str(), baton.port, baton.database.c_str());
    std::string connStr = std::string(connectionStr);
    baton.connection = baton.environment->createConnection(baton.user, baton.password, connStr);
    baton.connection->setStmtCacheSize(baton.stmtCacheSize);
  } catch(oracle::occi::SQLException &ex) {
    return Nan::ThrowError(ex.getMessage().c_str());
  } catch (const std::exception& ex) {
    return Nan::ThrowError(ex.what());
  }

  Local<FunctionTemplate> ft = Nan::New(Connection::s_ct);
  Local<Object> connection = ft->GetFunction()->NewInstance();

  (Nan::ObjectWrap::Unwrap<Connection>(connection))->setConnection(baton.environment, NULL, baton.connection);

  info.GetReturnValue().Set(connection);

}


NAN_METHOD(OracleClient::CreateConnectionPoolSync) {
  Nan::HandleScope scope;
  REQ_OBJECT_ARG(0, settings);

  OracleClient* client = Nan::ObjectWrap::Unwrap<OracleClient>(info.This());

  std::string hostname, user, password, database, tns;
  unsigned int port, minConn, maxConn, incrConn, timeout, busyOption, stmtCacheSize;

  OBJ_GET_STRING(settings, "hostname", hostname);
  OBJ_GET_STRING(settings, "user", user);
  OBJ_GET_STRING(settings, "password", password);
  OBJ_GET_STRING(settings, "database", database);
  OBJ_GET_NUMBER(settings, "port", port, 1521);
  OBJ_GET_NUMBER(settings, "minConn", minConn, 0);
  OBJ_GET_NUMBER(settings, "maxConn", maxConn, 4);
  OBJ_GET_NUMBER(settings, "incrConn", incrConn, 1);
  OBJ_GET_NUMBER(settings, "timeout", timeout, 0);
  OBJ_GET_NUMBER(settings, "busyOption", busyOption, 0);
  OBJ_GET_STRING(settings, "tns", tns);
  OBJ_GET_NUMBER(settings, "stmtCacheSize", stmtCacheSize, 16);

  try {
    char connectionStr[512];
    if (tns != "") {
      snprintf(connectionStr, sizeof(connectionStr), "%s", tns.c_str());
    } else {
      snprintf(connectionStr, sizeof(connectionStr), "//%s:%d/%s", hostname.c_str(), port, database.c_str());
    }
    std::string connStr = std::string(connectionStr);
    oracle::occi::StatelessConnectionPool *scp = client->m_environment->createStatelessConnectionPool(
                                    user, password, connStr, maxConn,
                                    minConn, incrConn,
                                    oracle::occi::StatelessConnectionPool::HOMOGENEOUS);
    scp->setTimeOut(timeout);
    scp->setBusyOption(static_cast<oracle::occi::StatelessConnectionPool::BusyOption>(busyOption));
    scp->setStmtCacheSize(stmtCacheSize);

    Local<FunctionTemplate> ft = Nan::New(ConnectionPool::s_ct);
    Local<Object> connectionPool = ft->GetFunction()->NewInstance();
    (Nan::ObjectWrap::Unwrap<ConnectionPool>(connectionPool))->setConnectionPool(client->m_environment, scp);

    info.GetReturnValue().Set(connectionPool);
  } catch(oracle::occi::SQLException &ex) {
    return Nan::ThrowError(ex.getMessage().c_str());
  } catch (const std::exception& ex) {
    return Nan::ThrowError(ex.what());
  }
}

NAN_METHOD(OracleClient::CreateConnectionPool) {
  Nan::HandleScope scope;

  REQ_OBJECT_ARG(0, settings);
  REQ_FUN_ARG(1, callback);

  OracleClient* client = Nan::ObjectWrap::Unwrap<OracleClient>(info.This());
  ConnectBaton* baton = new ConnectBaton(client, client->m_environment, callback);

  uint32_t busyOption;

  OBJ_GET_STRING(settings, "hostname", baton->hostname);
  OBJ_GET_STRING(settings, "user", baton->user);
  OBJ_GET_STRING(settings, "password", baton->password);
  OBJ_GET_STRING(settings, "database", baton->database);
  OBJ_GET_NUMBER(settings, "port", baton->port, 1521);
  OBJ_GET_NUMBER(settings, "minConn", baton->minConn, 0);
  OBJ_GET_NUMBER(settings, "maxConn", baton->maxConn, 4);
  OBJ_GET_NUMBER(settings, "incrConn", baton->incrConn, 1);
  OBJ_GET_NUMBER(settings, "timeout", baton->timeout, 0);
  OBJ_GET_NUMBER(settings, "busyOption", busyOption, 0);
  baton->busyOption = static_cast<oracle::occi::StatelessConnectionPool::BusyOption>(busyOption);
  OBJ_GET_STRING(settings, "tns", baton->tns);
  OBJ_GET_NUMBER(settings, "stmtCacheSize", baton->stmtCacheSize, 16);


  uv_queue_work(uv_default_loop(),
                &baton->work_req,
                EIO_CreateConnectionPool,
                (uv_after_work_cb) EIO_AfterCreateConnectionPool);

  return;
}

void OracleClient::EIO_CreateConnectionPool(uv_work_t* req) {
  ConnectBaton* baton = CONTAINER_OF(req, ConnectBaton, work_req);
  baton->error = NULL;

  try {
    char connectionStr[512];
    if (baton->environment->getLDAPHost() != "") {
      // http://docs.oracle.com/cd/E16655_01/network.121/e17610/config_concepts.htm#NETAG1054
      snprintf(connectionStr, sizeof(connectionStr), "%s", baton->database.c_str());
    } else if (baton->tns != "") {
      snprintf(connectionStr, sizeof(connectionStr), "%s", baton->tns.c_str());
    } else {
      snprintf(connectionStr, sizeof(connectionStr), "//%s:%d/%s", baton->hostname.c_str(), baton->port, baton->database.c_str());
    }
    std::string connStr = std::string(connectionStr);
    oracle::occi::StatelessConnectionPool *scp = baton->environment->createStatelessConnectionPool(
                                    baton->user,
                                    baton->password,
                                    connStr,
                                    baton->maxConn,
                                    baton->minConn,
                                    baton->incrConn,
                                    oracle::occi::StatelessConnectionPool::HOMOGENEOUS);
    scp->setTimeOut(baton->timeout);
    scp->setBusyOption(baton->busyOption);
    scp->setStmtCacheSize(baton->stmtCacheSize);
    baton->connectionPool = scp;

  } catch(oracle::occi::SQLException &ex) {
    baton->error = new std::string(ex.getMessage());
  } catch (const std::exception& ex) {
    baton->error = new std::string(ex.what());
  } catch (...) {
    baton->error = new std::string("Unknown exception is thrown");
  }
}

void OracleClient::EIO_AfterCreateConnectionPool(uv_work_t* req) {
  Nan::HandleScope scope;
  ConnectBaton* baton = CONTAINER_OF(req, ConnectBaton, work_req);

  Local<Value> argv[2];
  if(baton->error) {
    argv[0] = Exception::Error(Nan::New<String>(baton->error->c_str()).ToLocalChecked());
    argv[1] = Nan::Undefined();
  } else {
    argv[0] = Nan::Undefined();
    Local<FunctionTemplate> ft = Nan::New(ConnectionPool::s_ct);
    Local<Object> connectionPool = ft->GetFunction()->NewInstance();
    (Nan::ObjectWrap::Unwrap<ConnectionPool>(connectionPool))->setConnectionPool(baton->client->m_environment, baton->connectionPool);
    argv[1] = connectionPool;
  }

  Nan::TryCatch tryCatch;

  baton->callback.Call(2, argv);
  delete baton;

  if (tryCatch.HasCaught()) {
    Nan::FatalException(tryCatch);
  }
}

extern "C" {
  static void init(Handle<Object> target) {
    OracleClient::Init(target);
    ConnectionPool::Init(target);
    Connection::Init(target);
    OutParam::Init(target);
    Reader::Init(target);
    Statement::Init(target);
  }

  NODE_MODULE(oracle_bindings, init);
}
