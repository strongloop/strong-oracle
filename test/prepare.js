var settings = require('../tests-settings.json');
var oracle = require("../")(settings);
var assert = require('assert');

function initDb(connection, done) {
  connection.setPrefetchRowCount(50);
  connection.execute("DROP TABLE test_table", [], function (err) {
    // ignore error
    connection.execute("CREATE TABLE test_table (X INT)", [], function (err) {
      if (err) {
        return done(err);
      }
      return done();
    });
  });
}

function doInsert(stmt, i, max, done) {
  if (i < max) {
    stmt.execute([i], function (err, result) {
      if (err) {
        return done(err);
      }
      if (result.updateCount !== 1) {
        return done(new Error("bad count: " + result.updateCount));
      }
      doInsert(stmt, i + 1, max, done);
    });
  } else {
    return done();
  }
}

describe('prepare', function () {
  beforeEach(function (done) {
    var self = this;
    oracle.connect(settings, function (err, connection) {
      if (err) {
        return done(err);
      }
      self.connection = connection;
      initDb(self.connection, done);
    });
  });

  afterEach(function (done) {
    if (this.connection) {
      this.connection.close();
    }
    done();
  });

  it("should support prepared insert", function (done) {
    var self = this;
    var stmt = self.connection.prepare("INSERT INTO test_table (X) values (:1)");
    doInsert(stmt, 0, 100, function (err) {
      assert.equal(err, null);
      self.connection.execute("SELECT COUNT(*) from test_table", [], function (err, result) {
        assert.equal(err, null);
        assert(Array.isArray(result));
        assert.equal(result.length, 1);
        assert.equal(result[0]['COUNT(*)'], 100);
        done();
      });
    });
  });
});
