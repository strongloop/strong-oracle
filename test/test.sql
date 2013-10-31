DROP TABLE person;
CREATE TABLE person
  (id INTEGER PRIMARY KEY, name VARCHAR(255)
  );

DROP SEQUENCE person_seq;
CREATE SEQUENCE person_seq START WITH 1 INCREMENT BY 1 NOMAXVALUE;
CREATE OR REPLACE TRIGGER person_pk_trigger BEFORE
  INSERT ON person FOR EACH row BEGIN
  SELECT person_seq.nextval INTO :new.id FROM dual;
END;
/
DROP TABLE datatype_test;
CREATE TABLE datatype_test
  (
    id        INTEGER PRIMARY KEY,
    tvarchar2 VARCHAR2(255),
    tnvarchar2 NVARCHAR2(255),
    tchar CHAR(255),
    tnchar NCHAR(255),
    tnumber    NUMBER(10,5),
    tdate      DATE,
    ttimestamp TIMESTAMP,
    tclob CLOB,
    tnclob NCLOB,
    tblob BLOB
  );
DROP SEQUENCE datatype_test_seq;
CREATE SEQUENCE datatype_test_seq START WITH 1 INCREMENT BY 1 NOMAXVALUE;
CREATE OR REPLACE TRIGGER datatype_test_pk_trigger BEFORE
  INSERT ON datatype_test FOR EACH row BEGIN
  SELECT datatype_test_seq.nextval INTO :new.id FROM dual;
END;
/

  CREATE OR REPLACE PROCEDURE procNumericOutParam(param1 IN VARCHAR2, outParam1 OUT NUMBER)
  IS
  BEGIN
    DBMS_OUTPUT.PUT_LINE('Hello '|| param1);
    outParam1 := 42;
  END;
  /
  CREATE OR REPLACE PROCEDURE procStringOutParam(param1 IN VARCHAR2, outParam1 OUT STRING)
  IS
  BEGIN
    DBMS_OUTPUT.PUT_LINE('Hello '|| param1);
    outParam1 := 'Hello ' || param1;
  END;
  /
  CREATE OR REPLACE PROCEDURE procVarChar2OutParam(param1 IN VARCHAR2, outParam1 OUT VARCHAR2)
  IS
  BEGIN
    DBMS_OUTPUT.PUT_LINE('Hello '|| param1);
    outParam1 := 'Hello ' || param1;
  END;
  /
  CREATE OR REPLACE PROCEDURE procDoubleOutParam(param1 IN VARCHAR2, outParam1 OUT DOUBLE PRECISION)
  IS
  BEGIN
    outParam1 := -43.123456789012;
  END;
  /
  CREATE OR REPLACE PROCEDURE procFloatOutParam(param1 IN VARCHAR2, outParam1 OUT FLOAT)
  IS
  BEGIN
    outParam1 := 43;
  END;
  /

  CREATE OR REPLACE PROCEDURE procTwoOutParams(param1 IN VARCHAR2, outParam1 OUT NUMBER, outParam2 OUT STRING)
  IS
  BEGIN
    outParam1 := 42;
    outParam2 := 'Hello ' || param1;
  END;
  /
  CREATE OR REPLACE PROCEDURE procCursorOutParam(outParam OUT SYS_REFCURSOR)
  IS
  BEGIN
    open outParam for
    select * from person;
  END;
  /
  CREATE OR REPLACE PROCEDURE procCLOBOutParam(outParam OUT CLOB)
  IS
  BEGIN
    outParam := 'IAMCLOB';
  END;
  /

  BEGIN
     EXECUTE IMMEDIATE 'DROP TABLE basic_lob_table';
  EXCEPTION
     WHEN OTHERS THEN
        IF SQLCODE != -942 THEN
           RAISE;
        END IF;
  END;
  /

  create table basic_lob_table (x varchar2 (30), b blob, c clob);
  insert into basic_lob_table values('one', '010101010101010101010101010101', 'onetwothreefour');
  select * from basic_lob_table where x='one' and ROWNUM = 1;

  CREATE OR REPLACE PROCEDURE ReadBasicBLOB (outBlob OUT BLOB)
  IS
  BEGIN
      SELECT b INTO outBlob FROM basic_lob_table where x='one' and ROWNUM = 1;
  END;
  /

  CREATE OR REPLACE procedure doSquareInteger(z IN OUT Integer)
  is
  begin
    z := z * z;
  end;
  /
