-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION url" to load this file. \quit

CREATE FUNCTION url_test(text) RETURNS text
    AS '$libdir/url'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION url_in(cstring)
    RETURN url
    AS '$libdir/url', 'url_in'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION url_out(url)
    RETURN cstring
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

CREATE TYPE url (
    INPUT = url_in,
    OUTPUT = url_out
    -- RECEIVE = url_recv,
    -- SEND = url_send
);

--CREATE OR REPLACE FUNCTION url(protocol text, host text, port int, file text)
--RETURNS url
--AS '$libdir/url', 'url_constructor_all_fields'
--LANGUAGE C IMMUTABLE STRICT;
