#ifndef _commit_baton_h_
#define _commit_baton_h_

#include "connection.h"

class CommitBaton {
public:
  CommitBaton(Connection* connection, const v8::Handle<v8::Function>& callback) {
    this->connection = connection;
    this->callback = new NanCallback(callback);
  }
  ~CommitBaton() {
    delete callback;
  }

  Connection *connection;
  NanCallback *callback;
};

#endif
