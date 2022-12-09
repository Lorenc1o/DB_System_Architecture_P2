-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION url" to load this file. \quit

CREATE OR REPLACE FUNCTION url_in(cstring)
    RETURNS pg_url
    AS '$libdir/url', 'url_in'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION url_out(pg_url)
    RETURNS cstring
    AS '$libdir/url', 'url_out'
    LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE pg_url
(
    INPUT = url_in,
    OUTPUT = url_out
    --RECEIVE = url_recv,
    --SEND = url_send
);
COMMENT ON TYPE pg_url IS 'URL type implementation for PostgreSQL';

-- Constructor functions

-- URL(protocol, host, port, file)
CREATE OR REPLACE FUNCTION pg_url(cstring, cstring, int, cstring)
RETURNS pg_url
AS '$libdir/url', 'url_constructor_from_fields'
LANGUAGE C IMMUTABLE STRICT;

-- URL(protocol, host, file)
CREATE OR REPLACE FUNCTION pg_url(cstring, cstring, cstring)
RETURNS pg_url
AS '$libdir/url', 'url_constructor_from_fields_default_port'
LANGUAGE C IMMUTABLE STRICT;

-- URL(full_url_as_string)
CREATE OR REPLACE FUNCTION pg_url(cstring)
RETURNS pg_url
AS '$libdir/url', 'url_constructor_from_string'
LANGUAGE C IMMUTABLE STRICT;

-- URL(full_url_aas_text)
CREATE OR REPLACE FUNCTION pg_url
(text)
RETURNS pg_url
AS '$libdir/url', 'url_constructor_from_text'
LANGUAGE C IMMUTABLE STRICT;

-- URL(url, relative file)
-- Note: this function is not fully standard compliant. 
--      We concatenate relative file to url, assuming the path is relative.
CREATE OR REPLACE FUNCTION pg_url(pg_url, cstring)
RETURNS pg_url
AS '$libdir/url', 'url_constructor_context_spec'
LANGUAGE C IMMUTABLE STRICT;

-- Accessor functions

-- getUserInfo(url)
CREATE OR REPLACE FUNCTION get_userinfo(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_userinfo'
LANGUAGE C IMMUTABLE STRICT;

-- getHost(url)
CREATE OR REPLACE FUNCTION get_host(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_host'
LANGUAGE C IMMUTABLE STRICT;

-- getAuthority(url)
CREATE OR REPLACE FUNCTION get_authority(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_authority'
LANGUAGE C IMMUTABLE STRICT;

-- getPort(url)
CREATE OR REPLACE FUNCTION get_port(pg_url)
RETURNS int
AS '$libdir/url', 'get_port'
LANGUAGE C IMMUTABLE STRICT;

-- getProtocol(url)
CREATE OR REPLACE FUNCTION get_protocol(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_protocol'
LANGUAGE C IMMUTABLE STRICT;

-- getDefaultPort(url)
CREATE OR REPLACE FUNCTION get_default_port(pg_url)
RETURNS int
AS '$libdir/url', 'get_default_port'
LANGUAGE C IMMUTABLE STRICT;

-- getFile(url)
CREATE OR REPLACE FUNCTION get_file(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_file'
LANGUAGE C IMMUTABLE STRICT;

-- getPath(url)
CREATE OR REPLACE FUNCTION get_path(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_path'
LANGUAGE C IMMUTABLE STRICT;

-- getQuery(url)
CREATE OR REPLACE FUNCTION get_query(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_query'
LANGUAGE C IMMUTABLE STRICT;

-- getRef(url)
CREATE OR REPLACE FUNCTION get_ref(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_ref'
LANGUAGE C IMMUTABLE STRICT;

-- toString(url)
CREATE OR REPLACE FUNCTION toString(pg_url)
RETURNS cstring
AS '$libdir/url', 'toString'
LANGUAGE C IMMUTABLE STRICT;

-- CAST from test to url
CREATE CAST(text as pg_url) WITH FUNCTION pg_url(text) AS ASSIGNMENT;

-- CAST from url to text
CREATE CAST(pg_url as text) WITH INOUT AS ASSIGNMENT;

-- equals(url1, url2)
-- This is the internal representation of the function, which is not
-- the one that is directly called by the user. The user calls the
-- function EQUALS(url1, url2) which is defined in the next section.
CREATE OR REPLACE FUNCTION pg_url_equals_internal(pg_url, pg_url) 
RETURNS boolean
AS '$libdir/url', 'equals'
LANGUAGE C IMMUTABLE STRICT;

-- notEquals(url1, url2)
CREATE OR REPLACE FUNCTION pg_not_equals(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'not_equals'
LANGUAGE C IMMUTABLE STRICT;


-- sameFile(url1, url2)
-- This is the internal representation of the function, which is not
-- the one that is directly called by the user. The user calls the
-- function SAMEFILE(url1, url2) which is defined in the next section.
CREATE OR REPLACE FUNCTION same_file_internal(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'same_file'
LANGUAGE C IMMUTABLE STRICT;

-- lessThan(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_less_than(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'less_than'
LANGUAGE C IMMUTABLE STRICT;

-- lessThanOrEqual(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_less_than_or_equal(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'less_than_or_equal'
LANGUAGE C IMMUTABLE STRICT;

-- greaterThan(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_greater_than(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'greater_than'
LANGUAGE C IMMUTABLE STRICT;

-- greaterThanOrEqual(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_greater_than_or_equal(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'greater_than_or_equal'
LANGUAGE C IMMUTABLE STRICT;

-- urlCompare(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_cmp(pg_url, pg_url)
RETURNS int
AS '$libdir/url', 'pg_url_cmp'
LANGUAGE C IMMUTABLE STRICT;

-- Index support functions

-- sameHost(url1, url2)
-- This is the internal representation of the function, which is not
--      the one that is directly called by the user. The user calls the
--      function SAMEHOST(url1, url2) which is defined in the next section.
-- This one is necessary to define the B-tree operators
CREATE OR REPLACE FUNCTION same_host_internal(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'same_host'
LANGUAGE C IMMUTABLE STRICT;

-- differentHost(url1, url2)
CREATE OR REPLACE FUNCTION different_host(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'different_host'
LANGUAGE C IMMUTABLE STRICT;

-- equality operator for the B-tree index
-- Defined using the internal function same_host_internal
CREATE OPERATOR =
(
	LEFTARG = pg_url,
	RIGHTARG = pg_url,
	PROCEDURE = same_host_internal,
	COMMUTATOR = '=',
	NEGATOR = '<>',
	RESTRICT = eqsel,
	JOIN = eqjoinsel
);
COMMENT ON OPERATOR = (pg_url, pg_url) IS 'equal hosts?';

-- inequality operator for the B-tree index
-- Defined using the internal function different_host
CREATE OPERATOR <>
(
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = different_host,
    COMMUTATOR = '<>',
    NEGATOR = '=',
    RESTRICT = neqsel,
    JOIN = neqjoinsel
);
COMMENT ON OPERATOR <> (pg_url, pg_url) IS 'hosts not equal?';

-- sameHost(url1, url2)
-- This is the function that is called by the user to test if two
--      URLs have the same host.
-- This function is index-supported, we call the operator = used by the B-tree
CREATE OR REPLACE FUNCTION same_host(pg_url, pg_url)
RETURNS boolean
AS 'SELECT $1 = $2'
LANGUAGE SQL IMMUTABLE STRICT;

-- sameFile(url1, url2)
-- This is the function that is called by the user to test if two
--      URLs have the same file.
-- This function is index-supported:
--      - First, we filter the results using the operator = used by the B-tree,
--        which gives us those URLs with the required host.
--      - Second, we refine the result using the internal comparator only in
--        those URLs with the same host.
-- This strategy is thus called FILTER-AND-REFINE.
CREATE OR REPLACE FUNCTION same_file(pg_url, pg_url)
RETURNS boolean
AS 'SELECT $1 = $2 AND same_file_internal($1, $2)'
LANGUAGE SQL IMMUTABLE STRICT;

-- equals(url1, url2)
-- This is the function that is called by the user to test if two
--      URLs are equal.
-- This function is index-supported. 
-- We perform again the FILTER-AND-REFINE strategy, but now we 
--      refine twice, first ensuring that the files are equal, 
--      then comparing the complete URLs.
CREATE OR REPLACE FUNCTION pg_url_equals(pg_url, pg_url)
RETURNS boolean
AS 'SELECT same_file($1,$2) AND pg_url_equals_internal($1, $2)'
LANGUAGE SQL IMMUTABLE STRICT;

-- lessThanHost(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_less_than_host(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'less_than_host'
LANGUAGE C IMMUTABLE STRICT;

-- lessThanOrEqualHost(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_less_than_or_equal_host(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'less_than_or_equal_host'
LANGUAGE C IMMUTABLE STRICT;

-- greaterThanHost(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_greater_than_host(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'greater_than_host'
LANGUAGE C IMMUTABLE STRICT;

-- greaterThanOrEqualHost(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_greater_than_or_equal_host(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'greater_than_or_equal_host'
LANGUAGE C IMMUTABLE STRICT;

-- hostCompare(url1, url2)
CREATE OR REPLACE FUNCTION pg_url_cmp_host(pg_url, pg_url)
RETURNS int
AS '$libdir/url', 'pg_url_cmp_host'
LANGUAGE C IMMUTABLE STRICT;

-- less-than operator for the B-tree index
-- Defined using the internal function pg_url_less_than_host
CREATE OPERATOR <
(
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = pg_url_less_than_host,
    COMMUTATOR = '>',
    NEGATOR = '>=',
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);
COMMENT ON OPERATOR < (pg_url, pg_url) IS 'less than host?';

-- less-than-or-equal operator for the B-tree index
-- Defined using the internal function pg_url_less_than_or_equal_host
CREATE OPERATOR <=
(
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = pg_url_less_than_or_equal_host,
    COMMUTATOR = '>=',
    NEGATOR = '>',
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);
COMMENT ON OPERATOR <= (pg_url, pg_url) IS 'less than or equal host?';

-- greater-than operator for the B-tree index
-- Defined using the internal function pg_url_greater_than_host
CREATE OPERATOR >
(
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = pg_url_greater_than_host,
    COMMUTATOR = '<',
    NEGATOR = '<=',
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);
COMMENT ON OPERATOR > (pg_url, pg_url) IS 'greater than host?';

-- greater-than-or-equal operator for the B-tree index
-- Defined using the internal function pg_url_greater_than_or_equal_host
CREATE OPERATOR >=
(
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = pg_url_greater_than_or_equal_host,
    COMMUTATOR = '<=',
    NEGATOR = '<',
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);
COMMENT ON OPERATOR >= (pg_url, pg_url) IS 'greater than or equal host?';

-- Define the BTREE index on the pg_url type using the explained functions
-- It will construct the BTree following the order of the hosts of the URLs.
-- When two URLs have the same host, the BTree will used the deduplication strategy:
--      The deduplication strategy consist on storing only once each encountered host,
--      and storing the URLs with the same host in a linked list.
CREATE OPERATOR CLASS pg_url_ops 
DEFAULT FOR TYPE pg_url USING btree AS
    OPERATOR 1 <,
    OPERATOR 2 <=,
    OPERATOR 3 =,
    OPERATOR 4 >=,
    OPERATOR 5 >,
    FUNCTION 1 pg_url_cmp_host(pg_url, pg_url);