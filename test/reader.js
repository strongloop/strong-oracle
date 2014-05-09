var settings = require('../tests-settings.json');
var oracle = require("../")(settings);
var assert = require('assert');

function initDb(connection, max, cb) {
  connection.setPrefetchRowCount(50);
  connection.execute("DROP TABLE test_table", [], function (err) {
    // ignore error
    connection.execute("CREATE TABLE test_table (X INT)", [], function (err) {
      if (err) {
        return cb(err);
      }

      function insert(i, cb) {
        connection.execute("INSERT INTO test_table (X) VALUES (:1)", [i], function (err) {
          if (err) {
            return cb(err);
          }
          if (i < max) {
            insert(i + 1, cb);
          }
          else {
            cb();
          }
        });
      }

      insert(1, cb);
    });
  });
}

function doRead(reader, fn, count, cb) {
  if (count === 0) {
    return cb();
  }
  reader.nextRow(function (err, row) {
    if (err) {
      return cb(err);
    }
    if (row) {
      fn(row);
      return doRead(reader, fn, count - 1, cb)
    } else {
      return cb();
    }
  });
}

function testNextRow(done, connection, prefetch, requested, expected) {
  connection.setPrefetchRowCount(prefetch);
  var reader = connection.reader("SELECT X FROM test_table ORDER BY X", []);
  var total = 0;
  doRead(reader, function (row) {
    total += row.X;
  }, requested, function (err) {
    if (err) {
      return done(err);
    }
    assert.equal(total, expected * (expected + 1) / 2);
    done();
  });
}

function testNextRows(done, connection, requested, len1, len2) {
  var reader = connection.reader("SELECT X FROM test_table ORDER BY X", []);
  reader.nextRows(requested, function (err, rows) {
    if (err) {
      return done(err);
    }
    assert.equal(rows.length, len1);
    reader.nextRows(requested, function (err, rows) {
      if (err) {
        return done(err);
      }
      assert.equal(rows.length, len2);
      done();
    });
  });
}

describe('reader', function () {
  before(function (done) {
    var self = this;
    oracle.connect(settings, function (err, connection) {
      if (err) {
        return done(err);
      }
      self.connection = connection;
      initDb(self.connection, 100, done);
    });
  });

  after(function (done) {
    if (this.connection) {
      this.connection.close();
    }
    done();
  });

  it("should support nextRow - request 20 with prefetch of 5", function (done) {
    testNextRow(done, this.connection, 5, 20, 20);
  });

  it("should support nextRow - request 20 with prefetch of 200", function (done) {
    testNextRow(done, this.connection, 200, 20, 20);
  });

  it("should support nextRow - request 200 with prefetch of 5", function (done) {
    testNextRow(done, this.connection, 5, 200, 100);
  });

  it("should support nextRow - request 200 with prefetch of 200", function (done) {
    testNextRow(done, this.connection, 200, 200, 100);
  });

  it("should support nextRows - request 20", function (done) {
    testNextRows(done, this.connection, 20, 20, 20);
  });

  it("should support nextRows - request 70", function (done) {
    testNextRows(done, this.connection, 70, 70, 30);
  });

  it("should support nextRows - request 200", function (done) {
    testNextRows(done, this.connection, 200, 100, 0);
  });

});
