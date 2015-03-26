#ifndef _statement_baton_h_
#define _statement_baton_h_

#include "connection.h"
#include "executeBaton.h"

class StatementBaton: public ExecuteBaton {
public:
  StatementBaton(oracle::occi::Environment* m_environment,
                 oracle::occi::StatelessConnectionPool* m_connectionPool,
                 oracle::occi::Connection* m_connection,
                 bool m_autoCommit,
                 int m_prefetchRowCount,
                 const char* sql,
                 v8::Local<v8::Array> values = v8::Local<v8::Array>())
    : ExecuteBaton(m_environment, m_connectionPool, m_connection,
        m_autoCommit, m_prefetchRowCount,
        sql, values, Local<Object>()) {
    stmt = NULL;
    done = false;
    busy = false;
  }

  ~StatementBaton() {
    ResetStatement();
  }

  void ResetStatement() {
    if (stmt) {
      if (m_connection) {
        m_connection->terminateStatement(stmt);
      }
      stmt = NULL;
    }
  }

  oracle::occi::Statement* stmt;
  bool done;
  bool busy;
};

#endif
