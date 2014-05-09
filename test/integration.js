/*
 tests-settings.json:
 {
 "hostname": "localhost",
 "user": "test",
 "password": "test"
 }

 Database:
 CREATE TABLE person (id INTEGER PRIMARY KEY, name VARCHAR(255));
 CREATE SEQUENCE person_seq START WITH 1 INCREMENT BY 1 NOMAXVALUE;
 CREATE TRIGGER person_pk_trigger BEFORE INSERT ON person FOR EACH row
 BEGIN
 SELECT person_seq.nextval INTO :new.id FROM dual;
 END;
 /
 CREATE TABLE datatype_test (
 id INTEGER PRIMARY KEY,
 tvarchar2 VARCHAR2(255),
 tnvarchar2 NVARCHAR2(255),
 tchar CHAR(255),
 tnchar NCHAR(255),
 tnumber NUMBER(10,5),
 tdate DATE,
 ttimestamp TIMESTAMP,
 tclob CLOB,
 tnclob NCLOB,
 tblob BLOB);
 CREATE SEQUENCE datatype_test_seq START WITH 1 INCREMENT BY 1 NOMAXVALUE;
 CREATE TRIGGER datatype_test_pk_trigger BEFORE INSERT ON datatype_test FOR EACH row
 BEGIN
 SELECT datatype_test_seq.nextval INTO :new.id FROM dual;
 END;
 /
 */

var settings = require('../tests-settings.json');
var oracle = require("../")(settings);
var assert = require('assert');

describe('oracle driver', function () {
  beforeEach(function (done) {
    var self = this;
    //console.log("connecting: ", settings);
    oracle.connect(settings, function (err, connection) {
      if (err) {
        done(err);
        return;
      }
      //console.log("connected");
      self.connection = connection;
      self.connection.execute("DELETE FROM person", [], function (err, results) {
        if (err) {
          done(err);
          return;
        }
        self.connection.execute("DELETE FROM datatype_test", [], function (err, results) {
          if (err) {
            done(err);
            return;
          }
          //console.log("rows deleted: ", results);
          done();
        });
      });
    });
  });

  afterEach(function (done) {
    if (this.connection) {
      this.connection.close();
    }
    done();
  });

  it("should select with single quote", function (done) {
    var self = this;
    self.connection.execute("INSERT INTO person (name) VALUES (:1)", ["Bill O'Neil"], function (err, results) {
      if (err) {
        console.error(err);
        return;
      }
      self.connection.execute("INSERT INTO person (name) VALUES (:1)", ["Bob Johnson"], function (err, results) {
        if (err) {
          console.error(err);
          return;
        }
        self.connection.execute("SELECT * FROM person WHERE name = :1", ["Bill O'Neil"], function (err, results) {
          if (err) {
            console.error(err);
            return;
          }
          //console.log(results);
          assert.equal(results.length, 1);
          assert.equal(results[0]['NAME'], "Bill O'Neil");
          done();
        });
      });
    });
  });

  it("should insert with returning value", function (done) {
    var self = this;
    self.connection.execute("INSERT INTO person (name) VALUES (:1) RETURNING id INTO :2",
      ["Bill O'Neil", new oracle.OutParam()], function (err, results) {
        if (err) {
          console.error(err);
          return;
        }
        assert(results.returnParam > 0);
        done();
      });
  });

  it("should support datatypes with valid values", function (done) {
    var self = this;
    var date1 = new Date(2011, 10, 30, 1, 2, 3);
    var date2 = new Date(2011, 11, 1, 1, 2, 3);
    self.connection.execute(
        "INSERT INTO datatype_test "
        + "(tvarchar2, tnvarchar2, tchar, tnchar, tnumber, tdate, ttimestamp, tclob, tnclob, tblob) VALUES "
        + "(:1, :2, :3, :4, :5, :6, :7, :8, :9, :10) RETURNING id INTO :11",
      [
        "tvarchar2 value",
        "tnvarchar2 value",
        "tchar value",
        "tnchar value",
        42.5,
        date1,
        date2,
        "tclob value",
        "tnclob value",
        null, //new Buffer("tblob value"),
        new oracle.OutParam()
      ],
      function (err, results) {
        if (err) {
          console.error(err);
          return;
        }
        assert(results.returnParam > 0);

        self.connection.execute("SELECT * FROM datatype_test", [], function (err, results) {
          if (err) {
            console.error(err);
            return;
          }
          assert.equal(results.length, 1);
          assert.equal(results[0]['TVARCHAR2'], "tvarchar2 value");
          assert.equal(results[0]['TNVARCHAR2'], "tnvarchar2 value");
          assert.equal(results[0]['TCHAR'], "tchar value                                                                                                                                                                                                                                                    ");
          assert.equal(results[0]['TNCHAR'], "tnchar value                                                                                                                                                                                                                                                   ");
          assert.equal(results[0]['TNUMBER'], 42.5);
          assert.equal(results[0]['TDATE'].getTime(), date1.getTime());
          assert.equal(results[0]['TTIMESTAMP'].getTime(), date2.getTime());
          assert.equal(results[0]['TCLOB'], "tclob value");
          assert.equal(results[0]['TNCLOB'], "tnclob value");
          // todo: assert.equal(results[0]['TBLOB'], null);
          done();
        });
      });
  });

  it("should support datatypes with null values", function (done) {
    var self = this;
    self.connection.execute(
        "INSERT INTO datatype_test "
        + "(tvarchar2, tnvarchar2, tchar, tnchar, tnumber, tdate, ttimestamp, tclob, tnclob, tblob) VALUES "
        + "(:1, :2, :3, :4, :5, :6, :7, :8, :9, :10) RETURNING id INTO :11",
      [
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        new oracle.OutParam()
      ],
      function (err, results) {
        if (err) {
          console.error(err);
          return;
        }
        assert(results.returnParam > 0);

        self.connection.execute("SELECT * FROM datatype_test", [], function (err, results) {
          if (err) {
            console.error(err);
            return;
          }
          assert.equal(results.length, 1);
          assert.equal(results[0]['TVARCHAR2'], null);
          assert.equal(results[0]['TNVARCHAR2'], null);
          assert.equal(results[0]['TCHAR'], null);
          assert.equal(results[0]['TNCHAR'], null);
          assert.equal(results[0]['TNUMBER'], null);
          assert.equal(results[0]['TDATE'], null);
          assert.equal(results[0]['TTIMESTAMP'], null);
          assert.equal(results[0]['TCLOB'], null);
          assert.equal(results[0]['TNCLOB'], null);
          assert.equal(results[0]['TBLOB'], null);
          done();
        });
      });
  });

  it("should support date_types_insert_milliseconds", function (done) {
    var self = this,
      date = new Date(2013, 11, 24, 1, 2, 59, 999);

    self.connection.execute(
      "INSERT INTO datatype_test (tdate, ttimestamp) VALUES (:1, :2)",
      [date, date],
      function (err, results) {
        if (err) {
          console.error(err);
          return;
        }
        assert(results.updateCount === 1);

        self.connection.execute("SELECT tdate, ttimestamp FROM datatype_test",
          [], function (err, results) {
            if (err) {
              console.error(err);
              return;
            }
            assert.equal(results.length, 1);
            var dateWithoutMs = new Date(date);
            dateWithoutMs.setMilliseconds(0);
            assert.equal(results[0]['TDATE'].getTime(), dateWithoutMs.getTime(),
              "Milliseconds should not be stored for DATE");
            assert.equal(results[0]['TTIMESTAMP'].getTime(), date.getTime(),
              "Milliseconds should be stored for TIMESTAMP");
            done();
          });
      });
  });

  // FIXME: [rfeng] Investigate why the following test is failing
  /*
  it("should support utf8_chars_in_query", function (done) {
    var self = this,
      cyrillicString = "тест";

    self.connection.execute("SELECT '" + cyrillicString + "' as test FROM DUAL",
      [], function (err, results) {
        if (err) {
          console.error(err);
          return;
        }
        assert.equal(results.length, 1);
        assert.equal(results[0]['TEST'], cyrillicString,
          "UTF8 characters in sql query should be preserved");
        done();
      });
  });
  */
});
