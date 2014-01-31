
#include "connection.h"
#include "oracle_bindings.h"
#include "reader.h"
#include "outParam.h"
#include <iostream>

Persistent<FunctionTemplate> OracleClient::s_ct;

ConnectBaton::ConnectBaton(OracleClient* client, oracle::occi::Environment* environment, v8::Handle<v8::Function>* callback) {
  this->client = client;
  if(callback != NULL) {
    uni::Reset(this->callback, *callback);
  } else {
    // TODO: fix next line
    //this->callback = Persistent<Function>();
  }
  this->environment = environment;
  this->error = NULL;
  this->connection = NULL;

  this->hostname = "127.0.0.1";
  this->user = "";
  this->password = "";
  this->database = "";
}

ConnectBaton::~ConnectBaton() {
  callback.Dispose();
  if(error) {
    delete error;
  }
}

void OracleClient::Init(Handle<Object> target) {
  UNI_SCOPE(scope);

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  uni::Reset(s_ct, t);
  uni::Deref(s_ct)->InstanceTemplate()->SetInternalFieldCount(1);
  uni::Deref(s_ct)->SetClassName(String::NewSymbol("OracleClient"));

  NODE_SET_PROTOTYPE_METHOD(uni::Deref(s_ct), "connect", Connect);
  NODE_SET_PROTOTYPE_METHOD(uni::Deref(s_ct), "connectSync", ConnectSync);
  NODE_SET_PROTOTYPE_METHOD(uni::Deref(s_ct), "createConnectionPoolSync", CreateConnectionPoolSync);
  NODE_SET_PROTOTYPE_METHOD(uni::Deref(s_ct), "createConnectionPool", CreateConnectionPool);

  target->Set(String::NewSymbol("OracleClient"), uni::Deref(s_ct)->GetFunction());
}

uni::CallbackType OracleClient::New(const uni::FunctionCallbackInfo& args) {
  UNI_SCOPE(scope);

  /*
  REQ_OBJECT_ARG(0, settings);

  std:string hostname, user, password, database;
  unsigned int port, minConn, maxConn, incrConn;

  OBJ_GET_STRING(settings, "hostname", hostname);
  OBJ_GET_STRING(settings, "user", user);
  OBJ_GET_STRING(settings, "password", password);
  OBJ_GET_STRING(settings, "database", database);
  OBJ_GET_NUMBER(settings, "port", port, 1521);
  OBJ_GET_NUMBER(settings, "minConn", minConn, 0);
  OBJ_GET_NUMBER(settings, "maxConn", maxConn, 1);
  OBJ_GET_NUMBER(settings, "incrConn", incrConn, 1);

  std::ostringstream connectionStr;
      connectionStr << "//" << hostname << ":" << port << "/" << database;
  */

  OracleClient *client = new OracleClient();
  client->Wrap(args.This());
  UNI_RETURN(scope, args, args.This());
}

OracleClient::OracleClient() {
  m_environment = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::THREADED_MUTEXED);
  /*
  m_connectionPool = m_environment->createConnectionPool(user, password, connectString, minConn, maxConn, incrConn);
  */
}

OracleClient::~OracleClient() {
  /*
  m_environment->terminateConnectionPool (connectionPool);
  */
  oracle::occi::Environment::terminateEnvironment(m_environment);
}

uni::CallbackType OracleClient::Connect(const uni::FunctionCallbackInfo& args) {
  UNI_SCOPE(scope);

  REQ_OBJECT_ARG(0, settings);
  REQ_FUN_ARG(1, callback);

  OracleClient* client = ObjectWrap::Unwrap<OracleClient>(args.This());
  ConnectBaton* baton = new ConnectBaton(client, client->m_environment, &callback);

  OBJ_GET_STRING(settings, "hostname", baton->hostname);
  OBJ_GET_STRING(settings, "user", baton->user);
  OBJ_GET_STRING(settings, "password", baton->password);
  OBJ_GET_STRING(settings, "database", baton->database);
  OBJ_GET_NUMBER(settings, "port", baton->port, 1521);
  OBJ_GET_STRING(settings, "tns", baton->tns);

  client->Ref();

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Connect, (uv_after_work_cb)EIO_AfterConnect);

  UNI_RETURN(scope, args, Undefined());
}

void OracleClient::EIO_Connect(uv_work_t* req) {
  ConnectBaton* baton = static_cast<ConnectBaton*>(req->data);

  baton->error = NULL;

  try {
    std::ostringstream connectionStr;
    if (baton->tns != "") {
      connectionStr << baton->tns;
    } else {
      connectionStr << "//" << baton->hostname << ":" << baton->port << "/" << baton->database;
    }
    baton->connection = baton->environment->createConnection(baton->user, baton->password, connectionStr.str());
  } catch(oracle::occi::SQLException &ex) {
    baton->error = new std::string(ex.getMessage());
  } catch (const std::exception& ex) {
    baton->error = new std::string(ex.what());
  } catch (...) {
    baton->error = new std::string("Unknown exception is thrown");
  }
}

void OracleClient::EIO_AfterConnect(uv_work_t* req, int status) {
  UNI_SCOPE(scope);
  ConnectBaton* baton = static_cast<ConnectBaton*>(req->data);
  baton->client->Unref();

  Handle<Value> argv[2];
  if(baton->error) {
    argv[0] = Exception::Error(String::New(baton->error->c_str()));
    argv[1] = Undefined();
  } else {
    argv[0] = Undefined();
    Handle<Object> connection = uni::Deref(Connection::constructorTemplate)->GetFunction()->NewInstance();
    (node::ObjectWrap::Unwrap<Connection>(connection))->setConnection(baton->client->m_environment, NULL, baton->connection);
    argv[1] = connection;
  }

  node::MakeCallback(Context::GetCurrent()->Global(), uni::Deref(baton->callback), 2, argv);

  delete baton;
}

uni::CallbackType OracleClient::ConnectSync(const uni::FunctionCallbackInfo& args) {
  UNI_SCOPE(scope);
  REQ_OBJECT_ARG(0, settings);

  OracleClient* client = ObjectWrap::Unwrap<OracleClient>(args.This());
  ConnectBaton baton(client, client->m_environment, NULL);

  OBJ_GET_STRING(settings, "hostname", baton.hostname);
  OBJ_GET_STRING(settings, "user", baton.user);
  OBJ_GET_STRING(settings, "password", baton.password);
  OBJ_GET_STRING(settings, "database", baton.database);
  OBJ_GET_NUMBER(settings, "port", baton.port, 1521);

  try {
    std::ostringstream connectionStr;
    connectionStr << "//" << baton.hostname << ":" << baton.port << "/" << baton.database;
    baton.connection = baton.environment->createConnection(baton.user, baton.password, connectionStr.str());
  } catch(oracle::occi::SQLException &ex) {
    baton.error = new std::string(ex.getMessage());
    UNI_THROW(Exception::Error(String::New(baton.error->c_str())));
  } catch (const std::exception& ex) {
    UNI_THROW(Exception::Error(String::New(ex.what())));
  }

  Handle<Object> connection = uni::Deref(Connection::constructorTemplate)->GetFunction()->NewInstance();
    (node::ObjectWrap::Unwrap<Connection>(connection))->setConnection(baton.client->m_environment, NULL, baton.connection);

  UNI_RETURN(scope, args, connection);
}


Handle<Value> OracleClient::CreateConnectionPoolSync(const uni::FunctionCallbackInfo& args) {
  UNI_SCOPE(scope);
  REQ_OBJECT_ARG(0, settings);

  OracleClient* client = ObjectWrap::Unwrap<OracleClient>(args.This());

  std::string hostname, user, password, database, tns;
  unsigned int port, minConn, maxConn, incrConn, timeout, busyOption;

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

  try {
    std::ostringstream connectionStr;
    if (tns != "") {
      connectionStr << tns;
    } else {
      connectionStr << "//" << hostname << ":" << port << "/" << database;
    }

    oracle::occi::StatelessConnectionPool *scp = client->m_environment->createStatelessConnectionPool(
                                    user, password, connectionStr.str(), maxConn,
                                    minConn, incrConn,
                                    oracle::occi::StatelessConnectionPool::HOMOGENEOUS);
    scp->setTimeOut(timeout);
    scp->setBusyOption(static_cast<oracle::occi::StatelessConnectionPool::BusyOption>(busyOption));

    Handle<Object> connectionPool = ConnectionPool::connectionPoolConstructorTemplate->GetFunction()->NewInstance();
    (node::ObjectWrap::Unwrap<ConnectionPool>(connectionPool))->setConnectionPool(client->m_environment, scp);

    UNI_RETURN(scope, args, connectionPool);
  } catch(oracle::occi::SQLException &ex) {
    UNI_THROW(Exception::Error(String::New(ex.getMessage().c_str())));
  } catch (const std::exception& ex) {
    UNI_THROW(Exception::Error(String::New(ex.what())));
  }
}

Handle<Value> OracleClient::CreateConnectionPool(const uni::FunctionCallbackInfo& args) {
  UNI_SCOPE(scope);

  REQ_OBJECT_ARG(0, settings);
  REQ_FUN_ARG(1, callback);

  OracleClient* client = ObjectWrap::Unwrap<OracleClient>(args.This());
  ConnectBaton* baton = new ConnectBaton(client, client->m_environment, &callback);

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


  client->Ref();

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_CreateConnectionPool, (uv_after_work_cb)EIO_AfterCreateConnectionPool);

  UNI_RETURN(scope, args, Undefined());
}

void OracleClient::EIO_CreateConnectionPool(uv_work_t* req) {
  ConnectBaton* baton = static_cast<ConnectBaton*>(req->data);

  baton->error = NULL;

  try {
    std::ostringstream connectionStr;
    if (baton->tns != "") {
      connectionStr << baton->tns;
    } else {
      connectionStr << "//" << baton->hostname << ":" << baton->port << "/" << baton->database;
    }

    oracle::occi::StatelessConnectionPool *scp = baton->environment->createStatelessConnectionPool(
                                    baton->user,
                                    baton->password,
                                    connectionStr.str(),
                                    baton->maxConn,
                                    baton->minConn,
                                    baton->incrConn,
                                    oracle::occi::StatelessConnectionPool::HOMOGENEOUS);
    scp->setTimeOut(baton->timeout);
    scp->setBusyOption(baton->busyOption);
    baton->connectionPool = scp;

  } catch(oracle::occi::SQLException &ex) {
    baton->error = new std::string(ex.getMessage());
  } catch (const std::exception& ex) {
    baton->error = new std::string(ex.what());
  } catch (...) {
    baton->error = new std::string("Unknown exception is thrown");
  }
}

void OracleClient::EIO_AfterCreateConnectionPool(uv_work_t* req, int status) {
  UNI_SCOPE(scope);

  ConnectBaton* baton = static_cast<ConnectBaton*>(req->data);
  baton->client->Unref();

  Handle<Value> argv[2];
  if(baton->error) {
    argv[0] = Exception::Error(String::New(baton->error->c_str()));
    argv[1] = Undefined();
  } else {
    argv[0] = Undefined();
    Handle<Object> connectionPool =  uni::Deref(ConnectionPool::connectionPoolConstructorTemplate)->GetFunction()->NewInstance();
    (node::ObjectWrap::Unwrap<ConnectionPool>(connectionPool))->setConnectionPool(baton->client->m_environment, baton->connectionPool);
    argv[1] = connectionPool;
  }

  node::MakeCallback(Context::GetCurrent()->Global(), uni::Deref(baton->callback), 2, argv);

  delete baton;

}

extern "C" {
  static void init(Handle<Object> target) {
    OracleClient::Init(target);
    ConnectionPool::Init(target);
    Connection::Init(target);
    Reader::Init(target);
    OutParam::Init(target);
  }

  NODE_MODULE(oracle_bindings, init);
}
