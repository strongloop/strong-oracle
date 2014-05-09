var bindings = null;
try {
  // Try the release version
  bindings = require("../build/Release/oracle_bindings");
} catch (err) {
  if (err.code === 'MODULE_NOT_FOUND') {
    // Fall back to the debug version
    bindings = require("../build/Debug/oracle_bindings");
  } else {
    throw err;
  }
}

function getSettings(settings) {
  settings = settings || {
    hostname: '127.0.0.1',
    database: 'XE'
  };
  settings.hostname = settings.hostname || settings.host;
  settings.user = settings.user || settings.username;
  return settings;
}

module.exports = function (settings) {
  var ldap = settings && settings.ldap;
  var oracle = null;
  if (ldap === null || (typeof ldap !== 'object')) {
    oracle = new bindings.OracleClient();
  } else {
    oracle = new bindings.OracleClient(ldap);
  }
  var result = {};
  result.connect = function (settings, callback) {
    settings = getSettings(settings);
    oracle.connect(settings, callback);
  };

  result.connectSync = function (settings) {
    settings = getSettings(settings);
    return oracle.connectSync(settings);
  };

  result.createConnectionPool = function (settings, callback) {
    settings = getSettings(settings);
    oracle.createConnectionPool(settings, callback);
  };

  result.createConnectionPoolSync = function (settings) {
    settings = getSettings(settings);
    return oracle.createConnectionPoolSync(settings);
  };

  result.OutParam = bindings.OutParam;

  result.OCCIINT = 0;
  result.OCCISTRING = 1;
  result.OCCIDOUBLE = 2;
  result.OCCIFLOAT = 3;
  result.OCCICURSOR = 4;
  result.OCCICLOB = 5;
  result.OCCIDATE = 6;
  result.OCCITIMESTAMP = 7;
  result.OCCINUMBER = 8;
  result.OCCIBLOB = 9;

  return result;
}

// Reader is implemented in JS around a C++ handle
// This is easier and also more efficient because we don't cross the JS/C++ boundary
// every time we read a record.
function Reader(handle) {
  this._handle = handle;
  this._error = null;
  this._rows = [];
}

Reader.prototype.nextRows = function () {
  this._handle.nextRows.apply(this._handle, arguments);
}

Reader.prototype.nextRow = function (cb) {
  var self = this;
  if (!self._handle || self._error || (self._rows && self._rows.length > 0)) {
    process.nextTick(function () {
      cb(self._error, self._rows && self._rows.shift());
    });
  } else {
    // nextRows willl use the prefetch row count as window size
    self._handle.nextRows(function (err, result) {
      self._error = err || self._error;
      self._rows = result;
      if (err || result.length === 0) {
        self._handle = null;
      }
      cb(self._error, self._rows && self._rows.shift());
    });
  }
};

bindings.Connection.prototype.reader = function (sql, args) {
  return new Reader(this.readerHandle(sql, args));
};
