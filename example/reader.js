var settings = require('./config');

var ora = require('../lib/oracle')(settings);

var sql = 'SELECT * FROM PRODUCT';

ora.createConnectionPool(settings, function(err, pool) {
  if (err) {
    console.error(err);
    return;
  }
  console.log(pool.getInfo());
  pool.getConnection(function(err, conn) {
    conn.setPrefetchRowCount(10);
    conn.setAutoCommit(false);
    var reader = conn.reader(sql, []);
    var count = 0;
    var doRead = function() {
      reader.nextRow(function(err, row) {
        if (err) {
          throw err;
        }
        if (row) {
          console.log('%d %j', ++count, row);
          doRead();
        } else {
          console.log('Done');
        }
      });
    };
    doRead();
  });
});
