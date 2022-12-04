drop extension if exists url cascade;
create extension url;
--
-- Test url_test
--
drop table if exists test_url;
create table test_url (id int, u pg_url);
/*
insert into test_url values (1, 'http://www.test.com:80/file');
insert into test_url values (2, 'test.es');
insert into test_url values (3, 'http://www.test.com');
insert into test_url values (4, pg_url('ftp://www.test.com/file?param1=1&param2=2'));
insert into test_url values (5, pg_url('http','example.com',80,'file'));
insert into test_url values (6, pg_url('ftp','example.com','file'));
*/
-- -->> Constructor context spec
-- Exchanging each element
insert into test_url values (18, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','http://'));
insert into test_url values (28, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','www.spec.com'));
insert into test_url values (38, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','http://www.spec.com/'));
insert into test_url values (48, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','/filespec/spec/spec2/'));
insert into test_url values (58, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','?query=testing&spec=2'));
insert into test_url values (68, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','#specRef'));
insert into test_url values (78, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','ftp://www.spec.com/filespec/spec?specQ#ref')); 

--If spec has no scheme, the scheme component is inherited from the context URL.
insert into test_url values (71, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','www.spec.com/filespec/spec?specQ#ref')); 
insert into test_url values (82, pg_url('fttp://www.test.com/file?param1=1&param2=2','http://www.spec.com/')); --F

--TODO: The reference is parsed into the scheme, authority, path, query and fragment parts. If the path component is empty and the scheme, authority, and query components are undefined, then the new URL is a reference to the current document. Otherwise, the fragment and query parts present in the spec are used in the new URL.

insert into test_url values (17, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','#ref')); 
insert into test_url values (27, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','')); 
insert into test_url values (37, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','?query=testing&spec=2#specRef')); 

--If the scheme component is defined in the given spec and does not match the scheme of the context, then the new URL is created as an absolute URL based on the spec alone. Otherwise the scheme component is inherited from the context URL.

insert into test_url values (49, pg_url('ftp://www.test.com/file?param1=1&param2=2','http://www.spec.com/')); --F
insert into test_url values (58, pg_url('ftp://www.test.com/file?param1=1&param2=2','ftp://www.spec.com/')); --F
insert into test_url values (7, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','www.spec.com/filespec/spec?specQ#specref')); 
select * from test_url;

--If the authority component is present in the spec then the spec is treated as absolute and the spec authority and path will replace the context authority and path. 
insert into test_url values (10, pg_url('ftp://www.test.com/file?param1=1&param2=2','www.geeksforgeeks.org:80/url-samefile-method-in-java-with-examples/'));
insert into test_url values (11, pg_url('ftp://www.test.com/file?param1=1&param2=2','www.test.com/x/file?param1=testtest'));

--If the authority component is absent in the spec then the authority of the new URL will be inherited from the context.
insert into test_url values (12, pg_url('ftp://www.test.com/file?param1=1&param2=2','/url-samefile-method-in-java-with-examples/'));
insert into test_url values (13, pg_url('ftp://www.test.com','?param1=1&param2=2'));

--TODO:If the spec's path component begins with a slash character "/" then the path is treated as absolute and the spec path replaces the context path.
insert into test_url values (14, pg_url('ftp://www.test.com/','/filespec/spec?specQ#specref'));
insert into test_url values (24, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','/filespec/spec#specref'));

--Otherwise, the path is treated as a relative path and is appended to the context path.
insert into test_url values (15, pg_url('www.test.com/huiui/','?param1=1&param2=2'));
insert into test_url values (16, pg_url('ftp://www.test.com/filecontext/con','?param1=1&param2=2'));
insert into test_url values (17, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','moimo/kjoil=moi'));
insert into test_url values (18, pg_url('ftp://www.test.com/filecontext/con','param1=1&param2=2'));
insert into test_url values (19, pg_url('ftp://www.test.com/filecontext/con','#specRef'));
insert into test_url values (7, 'https://user1:pwd1@www.test.com');
insert into test_url values (8, 'https://user2@www.test.com');

-- -->> Constructor context spec
-- Exchanging each element
insert into test_url values (18, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','http://'));
insert into test_url values (28, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','www.spec.com'));
insert into test_url values (38, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','http://www.spec.com/'));
insert into test_url values (48, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','/filespec/spec/spec2/'));
insert into test_url values (58, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','?query=testing&spec=2'));
insert into test_url values (68, pg_url('fttp://www.test.com/file?param1=1&param2=2#contextRef','#specRef'));
insert into test_url values (78, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','ftp://www.spec.com/filespec/spec?specQ#ref')); 
*/
--If spec has no scheme, the scheme component is inherited from the context URL.
insert into test_url values (71, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','www.spec.com/filespec/spec?specQ#ref')); 
insert into test_url values (82, pg_url('fttp://www.test.com/file?param1=1&param2=2','http://www.spec.com/')); --F

select * from test_url;
--TODO: The reference is parsed into the scheme, authority, path, query and fragment parts. If the path component is empty and the scheme, authority, and query components are undefined, then the new URL is a reference to the current document. Otherwise, the fragment and query parts present in the spec are used in the new URL.

insert into test_url values (17, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','#ref')); 
insert into test_url values (27, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','')); 
insert into test_url values (37, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','?query=testing&spec=2#specRef')); 

--If the scheme component is defined in the given spec and does not match the scheme of the context, then the new URL is created as an absolute URL based on the spec alone. Otherwise the scheme component is inherited from the context URL.

insert into test_url values (49, pg_url('ftp://www.test.com/file?param1=1&param2=2','http://www.spec.com/')); --F
insert into test_url values (58, pg_url('ftp://www.test.com/file?param1=1&param2=2','ftp://www.spec.com/')); --F
insert into test_url values (7, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','www.spec.com/filespec/spec?specQ#specref')); 
select * from test_url;

--If the authority component is present in the spec then the spec is treated as absolute and the spec authority and path will replace the context authority and path. 
insert into test_url values (10, pg_url('ftp://www.test.com/file?param1=1&param2=2','www.geeksforgeeks.org:80/url-samefile-method-in-java-with-examples/'));
insert into test_url values (11, pg_url('ftp://www.test.com/file?param1=1&param2=2','www.test.com/x/file?param1=testtest'));

--If the authority component is absent in the spec then the authority of the new URL will be inherited from the context.
insert into test_url values (12, pg_url('ftp://www.test.com/file?param1=1&param2=2','/url-samefile-method-in-java-with-examples/'));
insert into test_url values (13, pg_url('ftp://www.test.com','?param1=1&param2=2'));

--TODO:If the spec's path component begins with a slash character "/" then the path is treated as absolute and the spec path replaces the context path.
insert into test_url values (14, pg_url('ftp://www.test.com/','/filespec/spec?specQ#specref'));
insert into test_url values (24, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','/filespec/spec#specref'));

--Otherwise, the path is treated as a relative path and is appended to the context path.
insert into test_url values (15, pg_url('www.test.com/huiui/','?param1=1&param2=2'));
insert into test_url values (16, pg_url('ftp://www.test.com/filecontext/con','?param1=1&param2=2'));
insert into test_url values (17, pg_url('http://www.test.com/filecontext/context?param1=1&param2=2#ref','moimo/kjoil=moi'));
insert into test_url values (18, pg_url('ftp://www.test.com/filecontext/con','param1=1&param2=2'));
insert into test_url values (19, pg_url('ftp://www.test.com/filecontext/con','#specRef'));




-- --> Functions testing
select * from test_url;
select id, get_host(u) from test_url;
select id, get_port(u) from test_url;
select id, get_protocol(u) from test_url;
select id, get_file(u) from test_url;
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