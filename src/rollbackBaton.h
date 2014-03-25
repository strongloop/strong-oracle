#ifndef _rollback_baton_h_
#define _rollback_baton_h_

#include "connection.h"

/**
 * Baton for asynchronous rollback
 */
class RollbackBaton {
public:
  RollbackBaton(Connection* connection, const v8::Handle<v8::Function>& callback) {
    this->connection = connection;
    this->callback = new NanCallback(callback);
  }

  ~RollbackBaton() {
    delete callback;
  }

  /**
   * The underlying OCCI connection
   */
  Connection *connection;
  /**
   * The callback function
   */
  NanCallback *callback;
};

#endif
