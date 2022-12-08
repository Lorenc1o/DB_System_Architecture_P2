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
insert into test_url values (4, pg_url('ftp://www.test.com/file?param1=1&param2=2'));
insert into test_url values (5, pg_url('http','example.com',80,'file'));
insert into test_url values (6, pg_url('ftp','example.com','file'));
insert into test_url values (7, pg_url('http://ulb.be', '/file'));
insert into test_url values (7, pg_url('http://ulb.be', 'www.file.com/file2'));


-- --> Functions testing
select * from test_url;
select id, get_host(u) from test_url;
select id, get_port(u) from test_url;
select id, get_protocol(u) from test_url;
select id, get_file(u) from test_url;
select id, get_userinfo(u) from test_url;
select id, get_default_port(u) from test_url;
select id, getAuthority(u) from test_url;
select id, getPath(u) from test_url;
select id, getQuery(u) from test_url;
select id, getRef(u) from test_url;

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