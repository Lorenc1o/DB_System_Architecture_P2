drop extension if exists url cascade;
create extension url;
--
-- Test url_test
--
drop table if exists test_url;
create table test_url (id int, u pg_url);
insert into test_url values (1, 'test');

select * from test_url;
select id, get_host(u) from test_url;
select id, get_port(u) from test_url;
select id, get_protocol(u) from test_url;
select id, get_file(u) from test_url;