module.exports = {
  hostname: process.env.ORACLE_HOST || 'localhost',
  port: process.env.ORACLE_PORT || undefined,
  database: process.env.ORACLE_DB || undefined,
  username: process.env.ORACLE_USER || 'test',
  password: process.env.ORACLE_PASSWORD || 'test',
};
