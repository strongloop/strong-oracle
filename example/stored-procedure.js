var settings = require('./config');

var ora = require('../lib/oracle')(settings);

var sql = 'call GETSUM(:1,:2,:3)';

ora.createConnectionPool(settings, function(err, pool) {
  if (err) {
    console.error(err);
    return;
  }
  pool.getConnection(function(err, conn) {
    console.log(sql);
    conn.execute(sql, [1, 2, new ora.OutParam(ora.OCCIINT)],
      function(err, result) {
        console.log(err, result);
        conn.close();
        pool.close();
      });
  });
});
