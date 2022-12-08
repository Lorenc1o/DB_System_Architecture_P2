drop extension if exists url cascade;
create extension url;
--
-- Test url_test
--
drop table if exists test_url;
create table test_url (id int, u pg_url);
insert into test_url values (1, 'http://www.test.com:80/file');
insert into test_url values (2, 'test.es');
insert into test_url values (3, 'http://www.test.com');
insert into test_url values (4, pg_url('ftp://www.test.com/file2?param1=1&param2=2'));
insert into test_url values (5, pg_url('http','example.com',80,'file3'));
insert into test_url values (6, pg_url('ftp','example.com','dir1/file4?param3=3#ref1'));
insert into test_url values (7, 'https://user1:pwd1@www.test.com');
insert into test_url values (8, 'https://user2@www.test.com');
insert into test_url values (9, pg_url('example.com', '/dir2/file5#ref2'));
insert into test_url values (10, pg_url('example.com', 'www.file6.be'));


select * from test_url;
select id, get_authority(u) from test_url;
select id, get_default_port(u) from test_url;
select id, u, get_file(u), get_path(u), get_query(u), get_ref(u) from test_url;
select id, get_host(u) from test_url;
select id, get_port(u) from test_url;
select id, get_protocol(u) from test_url;
select id, get_userinfo(u) from test_url;

drop table if exists test_equals;
create table test_equals (id int, u1 pg_url, u2 pg_url);
insert into test_equals values (1, 'http://www.test.com:80/file', 'http://www.test.com:80/file');
insert into test_equals values (2, 'http://www.test.com:80/file', 'https://www.test2.es:95/file');
insert into test_equals values (3, 'http://www.test.com:80/file2', 'http://www.test.com:80/file');
select * from test_equals;
select * from test_equals where pg_url_equals(u1, u2);
select * from test_equals where not pg_url_equals(u1, u2);
select * from test_equals where u1 = u2;
select * from test_equals where same_host(u1,u2);
select * from test_equals where same_file(u1,u2);
select * from test_equals where u1 <> u2;
select * from test_equals where u1 < u2;
select * from test_equals where u1 <= u2;
select * from test_equals where u1 > u2;
select * from test_equals where u1 >= u2;

create index test_url_u on test_url using btree (u);

set enable_seqscan = off;

explain select * from test_url where u = 'test.es';
explain select * from test_url where same_host(u,'test.es');
explain select same_host(u,'test.es') from test_url;
explain select same_file(u,'test.es') from test_url;
explain select pg_url_equals(u,'test.es') from test_url;

set enable_seqscan = off;

explain select * from test_url where u = 'test.es';
explain select * from test_url where same_host(u,'test.es');
explain select same_host(u,'test.es') from test_url;
explain select same_file(u,'test.es') from test_url;
explain select pg_url_equals(u,'test.es') from test_url;

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