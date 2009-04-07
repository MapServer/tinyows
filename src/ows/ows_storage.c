/*
  Copyright (c) <2007-2009> <Barbara Philippot - Olivier Courtin>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ows.h"


ows_layer_storage * ows_layer_storage_init()
{
    ows_layer_storage * storage;

    storage = malloc(sizeof(ows_layer_storage));
    assert(storage != NULL);

    /* default values:  schema='public',    srid='-1' */
    storage->schema = buffer_init();
    buffer_add_str(storage->schema, "public");
    storage->srid = -1;
    storage->geom_columns = list_init();
    storage->is_degree = true;
    storage->table = buffer_init();
    storage->pkey = buffer_init();
    storage->attributes = array_init();
    storage->not_null_columns = list_init();

    return storage;
}


void ows_layer_storage_free(ows_layer_storage * storage)
{
    assert(storage != NULL);

    if (storage->schema != NULL)
        buffer_free(storage->schema);

    if (storage->table != NULL)
        buffer_free(storage->table);

    if (storage->pkey != NULL)
        buffer_free(storage->pkey);

    if (storage->geom_columns != NULL)
        list_free(storage->geom_columns);

    if (storage->attributes != NULL)
        array_free(storage->attributes);

    if (storage->not_null_columns != NULL)
        list_free(storage->not_null_columns);

    free(storage);
    storage = NULL;
}


#ifdef OWS_DEBUG
void ows_layer_storage_flush(ows_layer_storage * storage, FILE * output)
{
    assert(storage != NULL);
    assert(output != NULL);

    if (storage->schema != NULL) {
        fprintf(output, "schema: ");
        buffer_flush(storage->schema, output);
        fprintf(output, "\n");
    }

    if (storage->table != NULL) {
        fprintf(output, "table: ");
        buffer_flush(storage->table, output);
        fprintf(output, "\n");
    }

    if (storage->geom_columns != NULL) {
        fprintf(output, "geom_columns: ");
        list_flush(storage->geom_columns, output);
        fprintf(output, "\n");
    }

    fprintf(output, "srid: %i\n", storage->srid);
    fprintf(output, "is_degree: %i\n", storage->is_degree ? 1 : 0);

    if (storage->pkey != NULL) {
        fprintf(output, "pkey: ");
        buffer_flush(storage->pkey, output);
        fprintf(output, "\n");
    }

    if (storage->attributes != NULL) {
        fprintf(output, "attributes: ");
        array_flush(storage->attributes, output);
        fprintf(output, "\n");
    }

    if (storage->not_null_columns != NULL) {
        fprintf(output, "not_null_columns: ");
        list_flush(storage->not_null_columns, output);
        fprintf(output, "\n");
    }

}
#endif


/*
 * Retrieve not_null columns of a table related a given layer
 */
static void ows_storage_fill_not_null(ows * o, ows_layer * l)
{
    buffer *sql, *b;
    PGresult *res;
    int i, end;

    assert(o != NULL);
    assert(l != NULL);
    assert(l->storage != NULL);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT a.attname AS field ");
    buffer_add_str(sql, "FROM pg_class c, pg_attribute a, pg_type t, pg_namespace n ");
    buffer_add_str(sql, "WHERE n.nspname = '");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND c.relname = '");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "' AND c.relnamespace = n.oid ");
    buffer_add_str(sql, "AND a.attnum > 0 AND a.attrelid = c.oid ");
    buffer_add_str(sql, "AND a.atttypid = t.oid AND a.attnotnull = 't'");

    res = PQexec(o->pg, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
                  "Unable to access pg_* tables.", "not_null columns");
    }

    for (i = 0, end = PQntuples(res); i < end; i++) {
        b = buffer_init();
        buffer_add_str(b, PQgetvalue(res, i, 0));
        list_add(l->storage->not_null_columns, b);
    }

    PQclear(res);
}

/*
 * Retrieve pkey column of a table related a given layer
 */
static void ows_storage_fill_pkey(ows * o, ows_layer * l)
{
    buffer *sql;
    PGresult *res;

    assert(o != NULL);
    assert(l != NULL);
    assert(l->storage != NULL);

    sql = buffer_init();

    buffer_add_str(sql, "SELECT c.column_name ");
    buffer_add_str(sql, "FROM information_schema.constraint_column_usage c, pg_namespace n ");
    buffer_add_str(sql, "WHERE n.nspname = '");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND c.table_name = '");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "' AND c.constraint_name = (");

    buffer_add_str(sql, "SELECT c.conname ");
    buffer_add_str(sql, "FROM pg_class r, pg_constraint c, pg_namespace n ");
    buffer_add_str(sql, "WHERE r.oid = c.conrelid AND relname = '");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "' AND r.relnamespace = n.oid ");
    buffer_add_str(sql, " AND n.nspname = '");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND c.contype = 'p')");

#if 0
    /* retrieve the column named id */
    buffer_add_str(sql, " UNION ");
    buffer_add_str(sql, "SELECT a.attname ");
    buffer_add_str(sql, "FROM pg_class c, pg_attribute a, pg_namespace n ");
    buffer_add_str(sql, "WHERE c.relname = ' ");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "' AND c.relnamespace = n.oid ");
    buffer_add_str(sql, " AND n.nspname = '");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND a.attname = 'id' AND a.attrelid = c.oid");
#endif

    res = PQexec(o->pg, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
                  "Unable to access pg_* tables.", "pkey column");
    }

    if (PQntuples(res) == 1)
        buffer_add_str(l->storage->pkey, PQgetvalue(res, 0, 0));

    PQclear(res);
}


/*
 * Retrieve columns name and type of a table related a given layer
 */
static void ows_storage_fill_attributes(ows * o, ows_layer * l)
{
    buffer *sql;
    PGresult *res;
    buffer *b, *t;
    int i, end;

    assert(o != NULL);
    assert(l != NULL);
    assert(l->storage != NULL);

    sql = buffer_init();

    buffer_add_str(sql, "SELECT a.attname AS field, t.typname AS type ");
    buffer_add_str(sql, "FROM pg_class c, pg_attribute a, pg_type t, pg_namespace n ");
    buffer_add_str(sql, "WHERE n.nspname = '");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND c.relname = '");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "' AND c.relnamespace = n.oid ");
    buffer_add_str(sql, "AND a.attnum > 0 AND a.attrelid = c.oid ");
    buffer_add_str(sql, "AND a.atttypid = t.oid");

    res = PQexec(o->pg, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
                  "Unable to access pg_* tables.", "fill_attributes");
    }

    for (i = 0, end = PQntuples(res); i < end; i++) {
        b = buffer_init();
        t = buffer_init();
        buffer_add_str(b, PQgetvalue(res, i, 0));
        buffer_add_str(t, PQgetvalue(res, i, 1));
        array_add(l->storage->attributes, b, t);

    }

    PQclear(res);
}


static void ows_layer_storage_fill(ows * o, ows_layer * l)
{
    buffer * sql;
    PGresult *res;
    int i, end;

    assert(o != NULL);
    assert(l != NULL);
    assert(l->storage != NULL);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT srid, f_geometry_column FROM geometry_columns");
    buffer_add_str(sql, " WHERE f_table_schema='");
    buffer_copy(sql, l->storage->schema);
    buffer_add_str(sql, "' AND f_table_name='");
    buffer_copy(sql, l->storage->table);
    buffer_add_str(sql, "';");

    res = PQexec(o->pg, sql->buf);
    buffer_empty(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
                  "Unable to access geometry_columns table", "storage");
    }

    l->storage->srid = atoi(PQgetvalue(res, 0, 0));

    for (i = 0, end = PQntuples(res); i < end; i++)
        list_add_str(l->storage->geom_columns, PQgetvalue(res, i, 1));

    buffer_add_str(sql, "SELECT * FROM spatial_ref_sys WHERE srid=");
    buffer_add_str(sql, PQgetvalue(res, 0, 0));
    buffer_add_str(sql, " AND proj4text like '%%units=m%%'");

    PQclear(res);

    res = PQexec(o->pg, sql->buf);
    buffer_free(sql);

    if (PQntuples(res) != 1)
        l->storage->is_degree = true;
    else
        l->storage->is_degree = false;

    PQclear(res);

    ows_storage_fill_attributes(o, l);
    ows_storage_fill_not_null(o, l);
    ows_storage_fill_pkey(o, l);
}

void ows_layers_storage_fill(ows * o)
{
    ows_layer_node *ln;

    assert(o != NULL);
    assert(o->layers != NULL);

    for (ln = o->layers->first; ln != NULL; ln = ln->next)
        if (ows_layer_match_table(o, ln->layer->name))
            ows_layer_storage_fill(o, ln->layer);
        else {
            ows_layer_storage_free(ln->layer->storage);
            ln->layer->storage = NULL;
        }
}

/*
 * vim: expandtab sw=4 ts=4
 */
