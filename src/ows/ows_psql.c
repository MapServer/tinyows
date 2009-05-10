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
#include <time.h>

#include "ows.h"


void ows_psql_prepare(ows * o, buffer * request_name, buffer * parameters, buffer * sql)
{
    buffer *prepare;
    PGresult *res;

    assert(o != NULL);
    assert(request_name != NULL);
    assert(parameters != NULL);
    assert(sql != NULL);

    prepare = buffer_init();

    buffer_add_str(prepare, "PREPARE ");
    buffer_copy(prepare, request_name);
    buffer_add_str(prepare, " ");
    buffer_copy(prepare, parameters);
    buffer_add_str(prepare, " AS ");
    buffer_copy(prepare, sql);

    res = PQexec(o->pg, prepare->buf);
    buffer_free(prepare);

    if (PQresultStatus(res) == PGRES_COMMAND_OK)
        list_add_by_copy(o->psql_requests, request_name);

    PQclear(res);
}


/*
 * Return the name of the id column from table matching layer name
 */
buffer *ows_psql_id_column(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o != NULL);
    assert(o->layers != NULL);
    assert(layer_name != NULL);

    for (ln = o->layers->first; ln != NULL; ln = ln->next)
        if (ln->layer->name != NULL
                && ln->layer->storage != NULL
                && ln->layer->name->use == layer_name->use
                && strcmp(ln->layer->name->buf, layer_name->buf) == 0)
            return ln->layer->storage->pkey;

    assert(0); /* Should not happen */
    return NULL;
}


/*
 * Return geometry columns from the table matching layer name
 */
list *ows_psql_geometry_column(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o != NULL);
    assert(o->layers != NULL);
    assert(layer_name != NULL);

    for (ln = o->layers->first; ln != NULL; ln = ln->next)
        if (ln->layer->name != NULL
                && ln->layer->storage != NULL
                && ln->layer->name->use == layer_name->use
                && strcmp(ln->layer->name->buf, layer_name->buf) == 0)
            return ln->layer->storage->geom_columns;

    assert(0); /* Should not happen */
    return NULL;
}


/*
 * Check if a given WKT geometry is or not valid
 */
bool ows_psql_is_geometry_valid(ows * o, buffer * geom)
{
    buffer *sql;
    PGresult *res;

    assert(o != NULL);
    assert(geom != NULL);

    sql = buffer_init();
    buffer_add_str(sql, "SELECT ST_isvalid(ST_geometryfromtext('");
    buffer_copy(sql, geom);
    buffer_add_str(sql, "', -1));");
    
    res = PQexec(o->pg, sql->buf);
    buffer_free(sql);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return false;
    }

    if ((char) PQgetvalue(res, 0, 0)[0] ==  't') {
        PQclear(res);
        return true;
    }

    PQclear(res);
    return false;
}

/*
 * Check if the specified column from a layer_name is (or not) a geometry column
 */
bool ows_psql_is_geometry_column(ows * o, buffer * layer_name, buffer * column)
{
    ows_layer_node *ln;

    assert(o != NULL);
    assert(o->layers != NULL);
    assert(layer_name != NULL);
    assert(column != NULL);

    for (ln = o->layers->first; ln != NULL; ln = ln->next)
        if (ln->layer->name != NULL
                && ln->layer->storage != NULL
                && ln->layer->name->use == layer_name->use
                && strcmp(ln->layer->name->buf, layer_name->buf) == 0)
            return in_list(ln->layer->storage->geom_columns, column);

    assert(0); /* should not happen */
    return false;
}


/*
 * Return a list of not null properties from the table matching layer name
 */
list *ows_psql_not_null_properties(ows * o, buffer * layer_name)
{
    ows_layer_node *ln;

    assert(o != NULL);
    assert(o->layers != NULL);
    assert(layer_name != NULL);

    for (ln = o->layers->first; ln != NULL; ln = ln->next)
        if (ln->layer->name != NULL
                && ln->layer->storage != NULL
                && ln->layer->name->use == layer_name->use
                && strcmp(ln->layer->name->buf, layer_name->buf) == 0)
            return ln->layer->storage->not_null_columns;

    assert(0); /* should not happen */
    return false;
}


/*
 * Return the number of the specified column from the table matching layer name
 */
int ows_psql_num_column(ows * o, buffer * layer_name, buffer * column)
{
    buffer *sql, *request_name, *parameters;
    PGresult *res;
    int number;

    assert(o != NULL);
    assert(layer_name != NULL);
    assert(column != NULL);

    sql = buffer_init();

    buffer_add_str(sql, "SELECT a.attnum ");
    buffer_add_str(sql, "FROM pg_class c, pg_attribute a, pg_type t ");
    buffer_add_str(sql, "WHERE c.relname = $1 ");
    buffer_add_str(sql, "AND a.attnum > 0 AND a.attrelid = c.oid ");
    buffer_add_str(sql, "AND a.atttypid = t.oid AND a.attname = $2");

    /* initialize the request's name and parameters */
    request_name = buffer_init();
    buffer_add_str(request_name, "num_column");
    parameters = buffer_init();
    buffer_add_str(parameters, "(text,text)");

    /* check if the request has already been executed */
    if (!in_list(o->psql_requests, request_name))
        ows_psql_prepare(o, request_name, parameters, sql);

    /* execute the request */
    buffer_empty(sql);
    buffer_add_str(sql, "EXECUTE num_column('");
    buffer_copy(sql, layer_name);
    buffer_add_str(sql, "','");
    buffer_copy(sql, column);
    buffer_add_str(sql, "')");

    res = PQexec(o->pg, sql->buf);
    buffer_free(sql);
    buffer_free(parameters);
    buffer_free(request_name);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return -1;
    }

    number = atoi(PQgetvalue(res, 0, 0)) - 1;
    PQclear(res);

    return number;
}


/*
 * Return the column's name matching the specified number from table
 */
buffer *ows_psql_column_name(ows * o, buffer * layer_name, int number)
{
    buffer *sql, *request_name, *parameters;
    PGresult *res;
    buffer *column;

    assert(o != NULL);
    assert(layer_name != NULL);

    sql = buffer_init();
    column = buffer_init();

    buffer_add_str(sql, "SELECT a.attname ");
    buffer_add_str(sql, "FROM pg_class c, pg_attribute a, pg_type t ");
    buffer_add_str(sql, "WHERE c.relname = $1 ");
    buffer_add_str(sql, "AND a.attnum > 0 AND a.attrelid = c.oid ");
    buffer_add_str(sql, "AND a.atttypid = t.oid AND a.attnum = $2");

    /* initialize the request's name and parameters */
    request_name = buffer_init();
    buffer_add_str(request_name, "column_name");
    parameters = buffer_init();
    buffer_add_str(parameters, "(text,int)");

    /* check if the request has already been executed */
    if (!in_list(o->psql_requests, request_name))
        ows_psql_prepare(o, request_name, parameters, sql);

    /* execute the request */
    buffer_empty(sql);
    buffer_add_str(sql, "EXECUTE column_name('");
    buffer_copy(sql, layer_name);
    buffer_add_str(sql, "',");
    buffer_add_int(sql, number);
    buffer_add_str(sql, ")");

    res = PQexec(o->pg, sql->buf);
    buffer_free(sql);
    buffer_free(parameters);
    buffer_free(request_name);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        PQclear(res);
        return column;
    }

    buffer_add_str(column, PQgetvalue(res, 0, 0));
    PQclear(res);

    return column;
}


/*
 * Retrieve description of a table matching a given layer name
 */
array *ows_psql_describe_table(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o != NULL);
    assert(o->layers != NULL);
    assert(layer_name != NULL);

    for (ln = o->layers->first; ln != NULL; ln = ln->next)
        if (ln->layer->name != NULL
                && ln->layer->storage != NULL
                && ln->layer->name->use == layer_name->use
                && strcmp(ln->layer->name->buf, layer_name->buf) == 0)
            return ln->layer->storage->attributes;

    assert(0); /* should not happen */
    return NULL;
}


/*
 * Convert a PostgreSql type to a valid
 * OGC XMLSchema's type
 */
char *ows_psql_to_xsd(buffer * type)
{
    assert(type != NULL);

    if (buffer_cmp(type, "geometry"))
        return "gml:GeometryPropertyType";
    else if (buffer_cmp(type, "int2"))
        return "short";
    else if (buffer_cmp(type, "int4"))
        return "int";
    else if (buffer_cmp(type, "int8"))
        return "long";
    else if (buffer_cmp(type, "float4"))
        return "float";
    else if (buffer_cmp(type, "float8"))
        return "double";
    else if (buffer_cmp(type, "bool"))
        return "boolean";
    else if (buffer_cmp(type, "bytea"))
        return "byte";
    else if (buffer_cmp(type, "date"))
        return "date";
    else if (buffer_cmp(type, "time"))
        return "time";
    else if (buffer_cmp(type, "timestamptz"))
        return "dateTime";
    else
        return "string";
}


/*
 * Convert a date from PostgreSQL to XML
 */
buffer *ows_psql_timestamp_to_xml_time(char *timestamp)
{
    buffer *time;

    assert(timestamp != NULL);

    time = buffer_init();
    buffer_add_str(time, timestamp);

    buffer_replace(time, " ", "T");

    if (check_regexp(time->buf, "\\+"))
        buffer_add_str(time, ":00");
    else
        buffer_add_str(time, "Z");

    return time;
}


/*
 * Return the type of the property passed in parameter
 */
buffer *ows_psql_type(ows * o, buffer * layer_name, buffer * property)
{
    ows_layer_node *ln;

    assert(o != NULL);
    assert(o->layers != NULL);
    assert(layer_name != NULL);
    assert(property != NULL);

    for (ln = o->layers->first; ln != NULL; ln = ln->next) {
        if (ln->layer->name != NULL
                && ln->layer->storage != NULL
                && ln->layer->name->use == layer_name->use
                && strcmp(ln->layer->name->buf, layer_name->buf) == 0)
            return array_get(ln->layer->storage->attributes, property->buf);
    }

    assert(0); /* should not happen */
    return NULL;
}



buffer *ows_psql_generate_id(ows * o, buffer * layer_name)
{
    ows_layer_node *ln;
    buffer * id, *sql_id;
    FILE *fp;
    PGresult * res;
    int i, seed_len;
    char * seed = NULL;

    assert(o != NULL);
    assert(o->layers != NULL);
    assert(layer_name != NULL);

    /* Retrieve layer node pointer */
    for (ln = o->layers->first; ln != NULL; ln = ln->next) {
        if (ln->layer->name != NULL
                && ln->layer->storage != NULL
                && ln->layer->name->use == layer_name->use
                && strcmp(ln->layer->name->buf, layer_name->buf) == 0) break;
    }
    assert(ln != NULL);

    id = buffer_init();

    /* If PK have a sequence in PostgreSQL database,
     * retrieve next available sequence value
     */
     if (ln->layer->storage->pkey_sequence != NULL) {
         sql_id = buffer_init();
         buffer_add_str(sql_id, "SELECT nextval('");
         buffer_copy(sql_id, ln->layer->storage->pkey_sequence);
         buffer_add_str(sql_id, "');");
         res = PQexec(o->pg, sql_id->buf);
         buffer_free(sql_id);

         if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1) {
             buffer_add_str(id, (char *) PQgetvalue(res, 0, 0));
             PQclear(res);
             return id;
         }

         /* FIXME: Shouldn't we return an error there instead ? */
         PQclear(res);
     } 
     
     /*
      * If we don't have a PostgreSQL Sequence, we will try to 
      * generate a pseudo random keystring using /dev/urandom
      * Will so work only on somes/commons Unix system
      */
     seed_len = 6;
     seed = malloc(sizeof(char) * (seed_len * 3 + 1));  /* multiply by 3 to be able to deal
                                                           with hex2dec conversion */ 
     assert(seed != NULL);
     seed[0] = '\0';

     fp=fopen("/dev/urandom","r");
     if (fp != NULL) {
         for (i=0 ; i<seed_len ; i++)
             sprintf(seed,"%s%03d",seed,fgetc(fp));
         fclose(fp);
         buffer_add_str(id, seed);
         free(seed);

         return id;
     } 
     free(seed);

    /* Case where we not using PostgreSQL sequence, 
     * and OS don't have a /dev/urandom support
     * This case don't prevent to produce ID collision
     * Don't use it unless really no others choices !!!
     */
     srand((int) (time(NULL) ^ rand() % 1000) + 42); 
     srand((rand() % 1000 ^ rand() % 1000) + 42); 
     buffer_add_int(id, rand());

     return id;
}

/*
 * vim: expandtab sw=4 ts=4
 */
