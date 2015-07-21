var async = require('async');
var fs = require('fs');
var oracle = require('./')(settings);
var path = require('path');
var settings = require('./tests-settings');

var raw = fs.readFileSync(path.join(__dirname, 'test', 'test.sql'), 'utf8');
var statements = [];
var stmt = '';
// split test/test.sql file into statements, but strip any trailing ;s unless
// they are at the end of an indented line, wich indicates a substatement.
raw.split('\n').forEach(function(line) {
  // test/test.sql uses "/" as dividers between related statements, so ignore
  // them and any blank lines
  if (/\//.test(line) || /^\s+$/.test(line)) {
    return;
  }
  if (/^\S+.*;$/.test(line)) {
    if (!/^end;$/i.test(line)) {
      stmt += line.slice(0, -1);
    } else {
      stmt += line;
    }
    statements.push(stmt.trim());
    stmt = '';
    return;
  }
  stmt += line + '\n';
});

oracle.connect(settings, function(err, connection) {
  if (err) {
    console.error('Error connecting to %j:', settings, err);
    throw err;
  }
  async.eachSeries(statements, runStmt, shutdown);

  function runStmt(stmt, next) {
    connection.execute(stmt, [], function(err) {
      if (err) {
        // ignore the errors, but mention them. The SQL script doesn't make
        // consistent use of exception handling for things like dropping tables
        // that don't exist.. (yet?)
        console.error('%s => %s', stmt, err);
      }
      next();
    });
  }
  function shutdown(err, res) {
    connection.close();
  }
});
