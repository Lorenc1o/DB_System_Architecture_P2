-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION url" to load this file. \quit

CREATE FUNCTION url_test(text) RETURNS text
    AS '$libdir/url'
    LANGUAGE C IMMUTABLE STRICT;

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

