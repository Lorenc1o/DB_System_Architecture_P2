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

-- CREATE OR REPLACE FUNCTION url_recv(internal)
--     RETURN url
--     AS '$libdir/url', 'url_recv'
--     LANGUAGE C IMMUTABLE STRICT;

-- CREATE OR REPLACE FUNCTION url_send(url)
--     RETURN bytea
--     AS '$libdir/url', 'url_send'
--     LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE pg_url (
    INPUT = url_in,
    OUTPUT = url_out
    -- RECEIVE = url_recv,
    -- SEND = url_send
);
COMMENT ON TYPE pg_url IS 'URL type implementation for PostgreSQL';

CREATE OR REPLACE FUNCTION pg_url(cstring, cstring, int, cstring)
RETURNS pg_url
AS '$libdir/url', 'url_constructor_from_fields'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pg_url(cstring, cstring, cstring)
RETURNS pg_url
AS '$libdir/url', 'url_constructor_from_fields_default_port'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pg_url(cstring)
RETURNS pg_url
AS '$libdir/url', 'url_constructor_from_string'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION get_host(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_host'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION get_port(pg_url)
RETURNS int
AS '$libdir/url', 'get_port'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION get_protocol(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_protocol'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION get_file(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_file'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION toString(pg_url)
RETURNS cstring
AS '$libdir/url', 'toString'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pg_equals(pg_url, pg_url) 
RETURNS boolean
AS '$libdir/url', 'equals'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pg_not_equals(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'not_equals'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pg_less_than(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'less_than'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pg_less_than_or_equal(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'less_than_or_equal'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pg_greater_than(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'greater_than'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pg_greater_than_or_equal(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'greater_than_or_equal'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION pg_url_cmp(pg_url, pg_url)
RETURNS int
AS '$libdir/url', 'pg_url_cmp'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR = (
	LEFTARG = pg_url,
	RIGHTARG = pg_url,
	PROCEDURE = pg_equals,
	COMMUTATOR = '=',
	NEGATOR = '<>',
	RESTRICT = eqsel,
	JOIN = eqjoinsel
);
COMMENT ON OPERATOR =(pg_url, pg_url) IS 'equals?';

CREATE OPERATOR <> (
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = pg_not_equals,
    COMMUTATOR = '<>',
    NEGATOR = '=',
    RESTRICT = neqsel,
    JOIN = neqjoinsel
);
COMMENT ON OPERATOR <>(pg_url, pg_url) IS 'not equals?';

CREATE OPERATOR < (
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = pg_less_than,
    COMMUTATOR = '>',
    NEGATOR = '>=',
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);
COMMENT ON OPERATOR <(pg_url, pg_url) IS 'less than?';

CREATE OPERATOR <= (
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = pg_less_than_or_equal,
    COMMUTATOR = '>=',
    NEGATOR = '>',
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);
COMMENT ON OPERATOR <=(pg_url, pg_url) IS 'less than or equal?';

CREATE OPERATOR > (
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = pg_greater_than,
    COMMUTATOR = '<',
    NEGATOR = '<=',
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);
COMMENT ON OPERATOR >(pg_url, pg_url) IS 'greater than?';

CREATE OPERATOR >= (
    LEFTARG = pg_url,
    RIGHTARG = pg_url,
    PROCEDURE = pg_greater_than_or_equal,
    COMMUTATOR = '<=',
    NEGATOR = '<',
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);
COMMENT ON OPERATOR >=(pg_url, pg_url) IS 'greater than or equal?';

CREATE OPERATOR CLASS pg_url_ops
DEFAULT FOR TYPE pg_url USING btree AS
    OPERATOR 1 <,
    OPERATOR 2 <=,
    OPERATOR 3 =,
    OPERATOR 4 >=,
    OPERATOR 5 >,
    FUNCTION 1 pg_url_cmp(pg_url, pg_url);