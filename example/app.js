var ora = require('../lib/oracle');
var settings = {
  host:'demo.strongloop.com',
  database:'XE',
  username:'demo',
  password:'L00pBack',
  minConn:1,
  maxConn:5,
  incrConn:1,
  timeout: 10
};

var sql = 'SELECT * FROM PRODUCT';
// var pool = ora.createConnectionPoolSync(settings);

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
