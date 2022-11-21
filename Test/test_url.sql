drop extension if exists url cascade;
create extension url;
--
-- Test url_test
--
drop table if exists test_url;
create table test_url (id int, u pg_url);
insert into test_url values (1, 'http://www.test.com:80/file');
insert into test_url values (1, 'test.es');
insert into test_url values (1, 'https://www.test.com');

select * from test_url;
select id, get_host(u) from test_url;
select id, get_port(u) from test_url;
select id, get_protocol(u) from test_url;
select id, get_file(u) from test_url;

drop table if exists test_equals;
create table test_equals (id int, u1 pg_url, u2 pg_url);
insert into test_equals values (1, 'http://www.test.com:80/file', 'http://www.test.com:80/file');
insert into test_equals values (2, 'http://www.test.com:80/file', 'https://www.test2.es:95/file');
select * from test_equals;
select * from test_equals where pg_equals(u1, u2);
select * from test_equals where not pg_equals(u1, u2);
select * from test_equals where u1 = u2;
select * from test_equals where u1 <> u2;