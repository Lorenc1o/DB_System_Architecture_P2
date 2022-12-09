drop extension if exists url cascade;
create extension url;
--
-- Test url_test
--
drop table if exists test_url;
create table test_url (u pg_url);

-- Test the different constructors and introduce different cases to test the getters
-- URL(String spec)
insert into test_url values ('http://www.test.com:80/file');
insert into test_url values ('test.es');
insert into test_url values ('http://www.test.com');
insert into test_url values (pg_url('ftp://www.test.com/file2?param1=1&param2=2'));
insert into test_url values ('https://user1:pwd1@www.test.com');
insert into test_url values ('https://user2@www.test.com');
insert into test_url values ('ftp://www.test.com/file');
-- URL(String protocol, String host, int port, String file)
insert into test_url values (pg_url('http','example.com',80,'file3'));
-- URL(String protocol, String host, String file)
insert into test_url values (pg_url('ftp','example.com','dir1/file4?param3=3#ref1'));
-- URL(URL context, String spec)
insert into test_url values (pg_url('example.com', '/dir2/file5#ref2'));
insert into test_url values (pg_url('example.com', 'www.file6.be'));

-- Test the different getters
select * from test_url;
select u, get_protocol(u), get_authority(u), get_userinfo(u), get_host(u), get_port(u), get_default_port(u), get_file(u), get_path(u), get_query(u), get_ref(u) from test_url;

-- Test the comparison functions
drop table if exists test_equals;
create table test_equals (u1 pg_url, u2 pg_url);
insert into test_equals values ('http://www.test.com:80/file', 'http://www.test.com:80/file');
insert into test_equals values ('http://www.test.com:80/file', 'https://www.test2.es:95/file');
insert into test_equals values ('http://www.test.com:80/file2', 'http://www.test.com:80/file');
select * from test_equals;
-- equals
select * from test_equals where pg_url_equals(u1, u2);
select * from test_equals where not pg_url_equals(u1, u2);
-- same_host
select * from test_equals where u1 = u2; -- same host
select * from test_equals where same_host(u1,u2); -- same host
select * from test_equals where u1 <> u2; -- different host
select * from test_equals where u1 < u2; -- less than host
select * from test_equals where u1 <= u2; -- less than or equal host
select * from test_equals where u1 > u2; -- greater than host
select * from test_equals where u1 >= u2; -- greater than or equal host
-- same_file
select * from test_equals where same_file(u1,u2);

create index test_url_u on test_url using btree (u);

set enable_seqscan = off;

explain select * from test_url where u = 'test.es';
explain select * from test_url where same_host(u,'www.test.com');
explain select * from test_url where same_file(u,'www.test.com/file');
explain select * from test_url where pg_url_equals(u,'http://www.test.com/file');

select * from test_url where same_host(u,'www.test.com');
select * from test_url where same_file(u,'www.test.com/file');
select * from test_url where pg_url_equals(u,'http://www.test.com/file');

set enable_seqscan = on;

select * from test_url where same_host(u,'www.test.com');
select * from test_url where same_file(u,'www.test.com/file');
select * from test_url where pg_url_equals(u,'http://www.test.com/file');