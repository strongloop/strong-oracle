var settings = require('./config');

var ora = require('../lib/oracle')(settings);

var sql = 'SELECT * FROM PRODUCT';

ora.createConnectionPool(settings, function(err, pool) {
  if(err) {
    console.error(err);
    return;
  }
  console.log(pool.getInfo());
  pool.getConnection(function(err, conn) {
    conn.setAutoCommit(false);
    console.log(pool.getInfo());
    console.log(sql);
    console.log(conn.executeSync(sql, []));
    conn.commit(function(err, result) {
      console.log(err, result);
      conn.close();
      console.log(pool.getInfo());
      pool.close();
      console.log(pool.getInfo());
    });
  });
});
