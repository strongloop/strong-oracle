2015-01-22, Version 1.3.0
=========================

 * Update deps (Raymond Feng)

 * rm test file extra_file.txt (Eric Waldheim)

 * add optional 3rd argument "options" to connection.execute.  Process "options" argument for getColumnMetaData flag.  Return result.columnMetaData (Eric Waldheim)

 * adding a temporary file (Benjamin Clark)

 * Fix bad CLA URL in CONTRIBUTING.md (Ryan Graham)


2014-11-20, Version 1.2.2
=========================

 * Bump version (Raymond Feng)

 * Update samples (Raymond Feng)

 * Fix GC issue around ConnectionPool/Connection (Raymond Feng)

 * Fix README (Raymond Feng)

 * Update contribution guidelines (Ryan Graham)


2014-09-25, Version 1.2.1
=========================

 * Bump version (Raymond Feng)

 * build: make oci version configurable (Ben Noordhuis)

 * build: fix whitespace errors in binding.gyp (Ben Noordhuis)

 * Upgrade to nan 1.3.0 (Raymond Feng)

 * Fixed compilation with VS2012 and node 0.11.13 (Vincent Schoettke)


2014-05-22, Version 1.2.0
=========================

 * doc: add CONTRIBUTING.md and LICENSE.md (Ben Noordhuis)

 * Remove the status arg (Raymond Feng)

 * Tidy up the destruction of Connection to respect the pool (Raymond Feng)

 * Fix LICENSE (Raymond Feng)

 * src: ref and unref connection in ExecuteBaton (Ben Noordhuis)

 * src: RAII-ify refcount management (Ben Noordhuis)

 * src: fix memory leak in callback-less close() methods (Ben Noordhuis)

 * src: pass values array by value (Ben Noordhuis)

 * src: deduplicate ExecuteBaton constructors (Ben Noordhuis)

 * src: embed NanCallback in ConnectBaton, don't new (Ben Noordhuis)

 * src: CONTAINER_OF-ize StatementBaton (Ben Noordhuis)

 * src: CONTAINER_OF-ize ReaderBaton (Ben Noordhuis)

 * src: CONTAINER_OF-ize ExecuteBaton (Ben Noordhuis)

 * src: CONTAINER_OF-ize ConnectionPoolBaton (Ben Noordhuis)

 * src: CONTAINER_OF-ize ConnectionBaton (Ben Noordhuis)

 * src: CONTAINER_OF-ize ConnectBaton (Ben Noordhuis)

 * tools: add valgrind suppression file for libocci (Ben Noordhuis)

 * Use NanNew to be compatible with v0.11.13 (Raymond Feng)

 * Fix the sql for tests (Raymond Feng)

 * Tidy up the code based on Ben's comments (Raymond Feng)

 * Fix test cases (Raymond Feng)

 * Bump version (Raymond Feng)

 * Restore char form setting for clobs (Raymond Feng)

 * Fix memory management related issues (Raymond Feng)

 * Move reading LOBs out of the callback thread (Raymond Feng)

 * Make ConnectionPool/Connection.close() async (Raymond Feng)

 * Add demo ldap server (Raymond Feng)

 * Enable LDAP naming method (Raymond Feng)

 * Upgrade to NAN 1.0.0 (Raymond Feng)


2014-03-31, Version 1.1.4
=========================

 * Bump version (Raymond Feng)

 * Catch exceptions during destructor (Raymond Feng)

 * Fix the compiler flag to enable C++ exception catch (Raymond Feng)


2014-03-29, Version 1.1.3
=========================

 * Bump version (Raymond Feng)

 * Fix date/timestamp handling (Raymond Feng)

 * Fix the prepare/reader wrappers (Raymond Feng)

 * Add more tests from upstream (Raymond Feng)

 * Fix the callback function name (Raymond Feng)

 * Convert to mocha test cases (Raymond Feng)


2014-03-26, Version 1.1.2
=========================

 * Bump version (Raymond Feng)

 * Add support for statement cache size (Raymond Feng)


2014-03-26, Version 1.1.1
=========================

 * Bump version (Raymond Feng)

 * Fix the CLOB reading issue (Raymond Feng)


2014-03-25, Version 1.1.0
=========================

 * Bump version (Raymond Feng)

 * Use a plain C struct to pass OutParm (Raymond Feng)

 * Copy the buffer data (Raymond Feng)

 * Remove using namespace v8 (Raymond Feng)

 * Use As<Date>() for the cast (Raymond Feng)

 * Use NAN to wrap V8 APIs (Raymond Feng)

 * Update .gitignore (Raymond Feng)

 * Reformat the code and add more comments (Raymond Feng)


2014-03-21, Version 1.0.0
=========================

 * Add an option to close connection pool (Raymond Feng)

 * Make require more flexible (Raymond Feng)

 * Add license (Raymond Feng)

 * Bump version (Raymond Feng)

 * Add emulated snprintf() for Windows (Raymond Feng)

 * Replace the insertion operator with snprintf (Raymond Feng)

 * Return the pool (Raymond Feng)

 * Remove the exception to favor the error attr (Raymond Feng)

 * Cherry-pick setPrefetchRowCount from nearinfinity/node-oracle (Raymond Feng)

 * Catch std exceptions (Raymond Feng)

 * Tidy up package.json (Raymond Feng)

 * Extract the sql (Raymond Feng)

 * Add a sample app (Raymond Feng)

 * Clean up code (Raymond Feng)

 * Add async version of getConnection (Raymond Feng)

 * Add getInfo to connection pool (Raymond Feng)

 * Start to add connection pool support (Raymond Feng)

 * Fix the merge problem (Raymond Feng)

 * Use different version for linux build (Raymond Feng)

 * Update README to add instructions to install libaio (Raymond Feng)

 * Update README (Raymond Feng)

 * Update package.json (Raymond Feng)

 * Update binding.gyp to use Oracle instance client for Windows 12_1 version (Raymond Feng)

 * Catch unknown exceptions (Raymond Feng)

 * More HandleScope close (Raymond Feng)

 * Convert connect_baton_t to a class (Raymond Feng)

 * Add HandleScope scope (Raymond Feng)

 * Use delete[] with new[] (Sam Roberts)

 * Introduced HandleScope to cleanup memory properly, fixed other minor memory leaks (Andrei Karpushonak)

 * Documentation for usage of "tns" connection parameter (Victor Volle)

 * Make sure the vars are initialized (Raymond Feng)

 * Initialize the properties to provide default values (Raymond Feng)

 * Throw exception if the job cannot be queued (Raymond Feng)

 * Clean up the handle scopes (Raymond Feng)

 * Fix OutParam destruction issue (Raymond Feng)

 * Check the args length (Raymond Feng)

 * Start to explore occi connection pool (Raymond Feng)

 * Remove std prefix (Raymond Feng)

 * Fix the message concatation (Raymond Feng)

 * Throw exception (Raymond Feng)

 * Add sync methods for connect and execute (Raymond Feng)

 * Find a way to get windows working (Raymond Feng)

 * Assign floating pointers to NULL, for safety (Sam Roberts)


2014-02-11, Version 0.3.6
=========================

 * Add 'Prerequsites' section (python, gcc) to basic install notes (John Coulter)

 * Added generic exception catcher to baton->createConnection to handle TNS not found errors (Giles Williams)

 * small edits to README (Bruno Jouhier)

 * added prepare API (Bruno Jouhier)

 * fixed memory leak on input parameters (Bruno Jouhier)

 * clear rows before invoking nextRows callback (Bruno Jouhier)

 * node 0.10.x was broken (Bruno Jouhier)

 * fixed req memory leak in all async calls (Bruno Jouhier)

 * cleanup pass on reader implementation (Bruno Jouhier)

 * fixed reader JS bindings so that reader handle gets destructed (Bruno Jouhier)

 * fixed cursor memory leaks - free rows data and don't wait for gc to release occi cursor (Bruno Jouhier)

 * fixed memory management of reader callback (Bruno Jouhier)

 * fixed memory/cursor leak in reader (Bruno Jouhier)

 * Update connection.cpp (mirozv)

 * added reader (Bruno Jouhier)

 * reviewed naming of uni macros and functions (Bruno Jouhier)

 * issue #112 - code compiles both in 0.10.x and 0.11.10 (Bruno Jouhier)

 * fixed incorrect reset of baton->error (Bruno Jouhier)

 * removed NodeOracleException (Bruno Jouhier)

 * use baton->error rather than throw to signal errors (Bruno Jouhier)

 * fixed typo on the Basic Example (ramsesrm)

 * Use utf8 V8 string for sql query string (see commit 73264550) (John Coulter)

 * Fix: Preserve milliseconds when inserting into TIMESTAMP column, by always using occi::Timestamp for JS Dates. An occi::Timestamp can still be inserted into a DATE column. Add tests for this issue. (John Coulter)

 * Fix JS Date --> Oracle Date conversion to use UTC methods. Update unit test, since TIMESTAMPs do store time info. Update README to explain how DATEs and TIMESTAMPs are handled (John Coulter)

 * Cleanup README examples and notes, and separate install instructions (John Coulter)

 * added license to project and package.json (Joe Ferner)


2013-12-06, Version 0.3.5
=========================

 * update git urls (Joe Ferner)

 * add nls_lang (tangyao)

 * added bash script to check correct environment before installing (Victor Volle)

 * added setPrefetchRowCount (Bruno Jouhier)


2013-09-09, Version 0.3.4
=========================

 * Fix whitespace and comment wording (John Coulter)

 * Cleanup whitespace (2 spaces) (John Coulter)

 * Use node::MakeCallback instead of v8::Function::Call (John Coulter)

 * Remove duplicate try/catch block which resulted in multiple callbacks (John Coulter)

 * Update binding.gyp (maxdm07)

 * Rethrow SQLException caught in closeConnection. This could be bad if an SQLException is thrown when the deconstructor calls closeConnection, as terminate will likely be called. (John Coulter)

 * Prevent closeConnection() from throwing an occi::SQLException. This might happen if the connection was already invalidated on the server, e.g., after a restart. (John Coulter)


2013-08-12, Version 0.3.3
=========================

 * Allow user to specify Instant Client version with OCI_VERSION (John Coulter)

 * Closes pull request 54 (Raymond Feng)

 * Convert oracle fractional seconds to milliseconds and fix typo in setUTCMilliseconds. (John Coulter)

 * Fix memory leak on throw (mscdex)

 * Handle timestamp with timezone data in the result set. Done by using additional OCI_TYPECODE constants from oro.h.  See the occi programmer's guide ch. 5 on datatypes: http://docs.oracle.com/cd/B19306_01/appdev.102/b14294/types.htm#sthref434 (John Coulter)

 * Fixed CLOB UTF8 bug where value would be truncated and padded with null characters when multi-byte charset is set. (John Coulter)


2013-07-22, Version 0.3.2
=========================

 * Add v8::TryCatch calls to handle exceptions in function callbacks (John Coulter)

 * Implement connection.isConnected(), don't segfault when doing work on closed connection (Michael Olson)

 * Documentation for usage of "tns" connection parameter (Victor Volle)

 * Support for TNS strings (Michael Friese)


2013-05-30, Version 0.3.1
=========================

 * Introduced HandleScope to cleanup memory properly, fixed other minor memory leaks (Andrei Karpushonak)

 * Add detailed instructions for installing the oracle instant client, including notes about the default directories that the binding.gyp looking for. (John Coulter)

 * Provide somewhat sane defaults for OCI_INCLUDE_DIR and OCI_LIB_DIR (oracle instant client directories) on windows. (John Coulter)

 * Properly resolve merge conflict between 5e9ed4a4665751becc865dcd800fcd533875a0e1 and f60745e3e5a464539ba184082c1dcd8fc20d1270 (the variables have now been moved to within OS specific conditions so that they can be resolved correctly on each platform.) (John Coulter)


2013-05-03, Version 0.3.0
=========================

 * fix merge conflict (Joe Ferner)

 * Added default values for oci_include_dir and oci_lib_dir variables in binding.gyp (so if you don't have OCCI_INCLUDE_DIR and OCCI_LIB_DIR set in your environment, the default oracle locations will be used.. (Damian Beresford)

 * Fix for setting default string size, 'outputs' destructed properly, & misc minor tidyup (Damian Beresford)

 * Minor update to README (Damian Beresford)

 * Support for INOUT types in stored procedures. Split tests into two files. Minor refactoring (Damian Beresford)

 * Support for unstreamed blobs in node-oracle (both in coulmns and as OUT params) (Damian Beresford)

 * Added out param support for OCCIDATE, OCCITIMESTAMP, OCCINUMBER (Damian Beresford)

 * Documentation updated in README.md. (Damian Beresford)

 * Minor refactoring: renamed variables in output_t (Damian Beresford)

 * Support for CLOB as out param (Damian Beresford)

 * Support for OCCICURSOR return type (Damian Beresford)

 * Support for multiple out params in stored procedures (Damian Beresford)

 * Support for node 0.8.x (and 0.10.x) (Damian Beresford)

 * Ticket 3818 Support for more OCCI types in node-oracle: doubles and floats now supported (Damian Beresford)

 * Support for both OCCIINT and OCCISTRING out params in stored procedures (Damian Beresford)

 * Get the result set after execute() (Raymond Feng)

 * Fix the gyp file so that it works on Windows (Raymond Feng)

 * Fix the date/timestamp conversion (Raymond Feng)

 * Enable the gyp build Upgrade to support node v0.10.x Fix the clob char form set issue to avoid addon 'abort' (Raymond Feng)

 * Added CLOB support (Danny Brain)

 * switch to utf8 in insert and select (Victor Volle)


2012-12-03, Version 0.2.0
=========================

 * version 0.2.0 (Joe Ferner)

 * now compiles and runs with node 0.8.8 (Romain Bossart)

 * [fix] path.existsSync was moved to fs.existsSync (Farrin Reid)


2012-06-22, Version 0.1.3
=========================

 * First release!
