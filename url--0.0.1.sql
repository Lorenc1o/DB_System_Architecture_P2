-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION url" to load this file. \quit

CREATE FUNCTION url_in(cstring)
    RETURNS pg_url
    AS '$libdir/url', 'url_in'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION url_out(pg_url)
    RETURNS cstring
    AS '$libdir/url', 'url_out'
    LANGUAGE C IMMUTABLE STRICT;

-- CREATE FUNCTION url_recv(internal)
--     RETURN url
--     AS '$libdir/url', 'url_recv'
--     LANGUAGE C IMMUTABLE STRICT;

-- CREATE FUNCTION url_send(url)
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

CREATE FUNCTION get_host(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_host'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION get_port(pg_url)
RETURNS int
AS '$libdir/url', 'get_port'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION get_protocol(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_protocol'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION get_file(pg_url)
RETURNS cstring
AS '$libdir/url', 'get_file'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION toString(pg_url)
RETURNS cstring
AS '$libdir/url', 'toString'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION pg_equals(pg_url, pg_url) 
RETURNS boolean
AS '$libdir/url', 'equals'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION pg_not_equals(pg_url, pg_url)
RETURNS boolean
AS '$libdir/url', 'not_equals'
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