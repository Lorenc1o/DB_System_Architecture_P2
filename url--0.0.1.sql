-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION url" to load this file. \quit

CREATE FUNCTION url_test(cstring) RETURNS text
    AS '$libdir/url'
    LANGUAGE C IMMUTABLE STRICT;
