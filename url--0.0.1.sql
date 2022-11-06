-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION url" to load this file. \quit

CREATE FUNCTION url_test(text) RETURNS text
    AS '$libdir/url'
    LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE url AS (
    protocol text,
    host text,
    port int,
    file text
);

CREATE OR REPLACE FUNCTION url(protocol text, host text, port int, file text)
RETURNS url
AS '$libdir/url', 'url_constructor_all_fields'
LANGUAGE C IMMUTABLE STRICT;
