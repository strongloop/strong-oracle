var settings = {
  minConn:1,
  maxConn:5,
  incrConn:1,
  timeout: 10,
  // host: 'demo.strongloop.com',
  // database: 'demo',
  tns: 'demo', // The tns can be //host:port/database, an tns name or ldap name
  user: 'demo',
  password: 'L00pBack',
  /*
  ldap: {
    adminContext: 'dc=strongloop,dc=com',
    host:'localhost',
    port: 1389,
    user:'cn=root',
    password:'secret'
  }*/
};

var path = require('path');
process.env.TNS_ADMIN= path.join(__dirname, 'admin');

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
