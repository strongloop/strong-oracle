// Configure the TNS_ADMIN home directory
var path = require('path');
process.env.TNS_ADMIN = path.join(__dirname, 'admin');

module.exports = {
  minConn: 1,
  maxConn: 10,
  incrConn: 1,
  timeout: 10,
  host: 'demo.strongloop.com',
  database: 'XE',
  user: 'demo',
  password: 'L00pBack',
  // tns: 'demo', // The tns can be //host:port/database, an tns name or ldap name
  /*
   ldap: {
   adminContext: 'dc=strongloop,dc=com',
   host:'localhost',
   port: 1389,
   user:'cn=root',
   password:'secret'
   }*/
};

