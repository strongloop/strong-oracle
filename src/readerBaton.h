#ifndef _reader_baton_h_
#define _reader_baton_h_

#include "connection.h"
#include "statementBaton.h"

class ReaderBaton: public StatementBaton {
public:
  ReaderBaton(oracle::occi::Environment* m_environment,
              oracle::occi::StatelessConnectionPool* m_connectionPool,
              oracle::occi::Connection* m_connection,
              bool m_autoCommit,
              int m_prefetchRowCount,
              const char* sql,
              v8::Local<v8::Array> values) :
      StatementBaton(m_environment, m_connectionPool, m_connection,
                     m_autoCommit, m_prefetchRowCount,
                     sql, values) {
    stmt = NULL;
    rs = NULL;
    done = false;
    busy = false;
    count = 0;
  }
  ~ReaderBaton() {
    ResetStatement();
  }

  void ResetStatement() {
    if (stmt && rs) {
      stmt->closeResultSet(rs);
      rs = NULL;
    }
    StatementBaton::ResetStatement();
  }

  oracle::occi::ResultSet* rs;
  int count;
};

#endif
