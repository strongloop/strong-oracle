#ifndef _oracle_binding_h_
#define _oracle_binding_h_

#include <v8.h>
#include <node.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <occi.h>
#include "utils.h"

using namespace node;
using namespace v8;

/**
 * C++ class for OracleClient as follows:
 *
 * function OracleClient() {
 * ...
 * }
 *
 * OracleClient.prototype.connect = function(..., callback) {
 * ...
 * };
 *
 * OracleClient.prototype.connectSync(...) {
 * ...
 * }
 *
 * OracleClient.prototype.createConnectionPool = function(..., callback) {
 * ...
 * };
 *
 * OracleClient.prototype.createConnectionPoolSync(...) {
 * ...
 * }
 */
class OracleClient: public ObjectWrap {
public:
  /**
   * Define OracleClient class
   */
  static void Init(Handle<Object> target);
  /**
   * Constructor
   */
  static NAN_METHOD(New);

  /**
   * connect(..., callback)
   */
  static NAN_METHOD(Connect);
  static void EIO_Connect(uv_work_t* req);
  static void EIO_AfterConnect(uv_work_t* req);

  /**
   * connectSync(...)
   */
  static NAN_METHOD(ConnectSync);

  /**
   * createConnectionPool(..., callback)
   */
  static NAN_METHOD(CreateConnectionPool);
  static void EIO_CreateConnectionPool(uv_work_t* req);
  static void EIO_AfterCreateConnectionPool(uv_work_t* req);

  /**
   * createConnectionPoolSync(...)
   */
  static NAN_METHOD(CreateConnectionPoolSync);

  explicit OracleClient(oracle::occi::Environment::Mode mode = oracle::occi::Environment::THREADED_MUTEXED);
  ~OracleClient();

  oracle::occi::Environment* getEnvironment() {
    return m_environment;
  }

private:
  static Persistent<FunctionTemplate> s_ct;
  oracle::occi::Environment* m_environment;
  // oracle::occi::StatelessConnectionPool* m_connectionPool;
};

/**
 * The Baton for the asynchronous connect
 */
class ConnectBaton {
public:
  ConnectBaton(OracleClient* client,
               oracle::occi::Environment* environment,
               v8::Handle<v8::Function> callback = v8::Handle<v8::Function>());
  ~ConnectBaton();

  /**
   * The OracleClient instance
   */
  OracleClient* client;

  /**
   * The callback function
   */
  NanCallback callback;

  /**
   * host name or ip address for the DB server
   */
  std::string hostname;
  /**
   * user name
   */
  std::string user;
  /**
   * password
   */
  std::string password;
  /**
   * The database name
   */
  std::string database;
  /**
   * The TNS connection string
   */
  std::string tns;
  /**
   * The port number
   */
  uint32_t port;
  /**
   * The minimum number of connections for pool
   */
  uint32_t minConn;
  /**
   * The maximum number of connections for pool
   */
  uint32_t maxConn;
  /**
   * The number of connections for increment
   */
  uint32_t incrConn;
  /**
   * timeout
   */
  uint32_t timeout;
  /**
   * Statement cache size
   */
  uint32_t stmtCacheSize;
  /**
   * OCCI stateless connection pool's busy  option
   */
  oracle::occi::StatelessConnectionPool::BusyOption busyOption;

  /**
   * The OCCI environment
   */
  oracle::occi::Environment* environment;
  /**
   * OCCI stateless connection pool
   */
  oracle::occi::StatelessConnectionPool* connectionPool;
  /**
   * OCCI connection
   */
  oracle::occi::Connection* connection;

  /**
   * Error message
   */
  std::string* error;

  uv_work_t work_req;
};

#endif
