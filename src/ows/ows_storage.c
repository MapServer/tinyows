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
#include <string.h>

#include "ows.h"


ows_layer_storage * ows_layer_storage_init()
{
    ows_layer_storage * storage;

    storage = malloc(sizeof(ows_layer_storage));
    assert(storage != NULL);

    /* default values:  srid='-1' */
    storage->schema = buffer_init();
    storage->srid = -1;
    storage->geom_columns = list_init();
    storage->is_degree = true;
    storage->table = buffer_init();
    storage->pkey = NULL;
    storage->pkey_sequence = NULL;
    storage->pkey_column_number = -1;
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

    if (storage->pkey_sequence != NULL)
        buffer_free(storage->pkey_sequence);

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

    fprintf(output, "pkey_column_number: %i\n", storage->pkey_column_number);

    if (storage->pkey_sequence != NULL) {
        fprintf(output, "pkey_sequence: ");
        buffer_flush(storage->pkey_sequence, output);
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
 * And if success try also to retrieve a related pkey sequence
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

    res = PQexec(o->pg, sql->buf);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        buffer_free(sql);
        ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
                  "Unable to access pg_* tables.", "pkey column");
    }

    /* Layer could have no Pkey indeed... (An SQL view for example) */
    if (PQntuples(res) == 1) {
        l->storage->pkey = buffer_init();
        buffer_add_str(l->storage->pkey, PQgetvalue(res, 0, 0));
        buffer_empty(sql);
        PQclear(res);


        /* Retrieve the Pkey column number */
        buffer_add_str(sql, "SELECT a.attnum "); 
        buffer_add_str(sql, "FROM pg_class c, pg_attribute a, pg_type t, pg_namespace n");
        buffer_add_str(sql, " WHERE a.attnum > 0 AND a.attrelid = c.oid");
        buffer_add_str(sql, " AND a.atttypid = t.oid AND n.nspname='");
        buffer_copy(sql, l->storage->schema);
        buffer_add_str(sql, "' AND c.relname='");
        buffer_copy(sql, l->storage->table);
        buffer_add_str(sql, "' AND a.attname='");
        buffer_copy(sql, l->storage->pkey);
        buffer_add_str(sql, "'");
        res = PQexec(o->pg, sql->buf);
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res);
            buffer_free(sql);
            ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
                  "Unable to find pkey column number.", "pkey_column number");
        }

        /* -1 because column number start at 1 */
        l->storage->pkey_column_number = atoi(PQgetvalue(res, 0, 0)) - 1;
        buffer_empty(sql);
        PQclear(res);

        /* Now try to find a sequence related to this Pkey */
        buffer_add_str(sql, "SELECT pg_get_serial_sequence('");
        buffer_copy(sql, l->storage->schema);
        buffer_add_str(sql, ".\"");
        buffer_copy(sql, l->storage->table);
	    buffer_add_str(sql, "\"', '");
        buffer_copy(sql, l->storage->pkey);
	    buffer_add_str(sql, "');");

        res = PQexec(o->pg, sql->buf);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res);
            buffer_free(sql);
            ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
                  "Unable to use pg_get_serial_sequence.", "pkey_sequence retrieve");
        }
        
        /* Even if no sequence found, this function return an empty row
         * so we must check that result string returned > 0 char
         */
        if (PQntuples(res) == 1 && strlen((char *) PQgetvalue(res, 0, 0)) > 0) {
            l->storage->pkey_sequence = buffer_init();
            buffer_add_str(l->storage->pkey_sequence, PQgetvalue(res, 0, 0));
        }
    }

    PQclear(res);
    buffer_free(sql);
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
                  "Some layer(s) described in config file are not available from geometry_columns table",
                  "storage");
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


void ows_layers_storage_flush(ows * o, FILE * output)
{
    ows_layer_node *ln;

    assert(o != NULL);
    assert(o->layers != NULL);

    for (ln = o->layers->first; ln != NULL; ln = ln->next) {
        if (ln->layer->storage != NULL) {
                fprintf(output, " - %s.%s  -> %i ", ln->layer->storage->schema->buf,
                                                        ln->layer->storage->table->buf,
                                                        ln->layer->storage->srid);
                if (ln->layer->retrievable) fprintf(output, "R");
                if (ln->layer->writable)   fprintf(output, "W");
                fprintf(output, "\n");
        }
    }
} 


void ows_layers_storage_fill(ows * o)
{
    ows_layer_node *ln;
    buffer * sql;
    PGresult *res;
    int i, end;
    bool filled;

    assert(o != NULL);
    assert(o->layers != NULL);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT DISTINCT f_table_schema, f_table_name FROM geometry_columns");
    res = PQexec(o->pg, sql->buf);
    buffer_free(sql);

    for (ln = o->layers->first; ln != NULL; ln = ln->next) {
        filled = false;

        for (i = 0, end = PQntuples(res); i < end; i++) {
            if (buffer_cmp(ln->layer->name, (char *) PQgetvalue(res, i, 1))) {
                ows_layer_storage_fill(o, ln->layer);
                filled = true;
            }
        }
            
        if (!filled) {
            if (ln->layer->storage != NULL)
                ows_layer_storage_free(ln->layer->storage);
                ln->layer->storage = NULL;
        }
    }
    PQclear(res);

}

/*
 * vim: expandtab sw=4 ts=4
 */
