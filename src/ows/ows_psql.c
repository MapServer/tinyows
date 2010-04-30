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
 * Return the column number of the id column from table matching layer name
 * (needed in wfs_get_feature only)
 */
int ows_psql_column_number_id_column(ows * o, buffer * layer_name)
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
            return ln->layer->storage->pkey_column_number;

    assert(0); /* Should not happen */
    return -1;
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
 * Return schema name from a given layer 
 */
buffer *ows_psql_schema_name(ows * o, buffer * layer_name)
{
    ows_layer_node *ln = NULL;

    assert(o != NULL);
    assert(o->layers != NULL);
    assert(layer_name != NULL);

    for (ln = o->layers->first; ln != NULL; ln = ln->next)
        if (ln->layer->name != NULL
                && ln->layer->storage != NULL
                && !strcmp(ln->layer->name->buf, layer_name->buf))
            return ln->layer->storage->schema;

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
 * Return the column's name matching the specified number from table
 * (Only use in specific FE position function, so not directly inside
 *  storage handle mechanism)
 */
buffer *ows_psql_column_name(ows * o, buffer * layer_name, int number)
{
    buffer *sql;
    PGresult *res;
    buffer *column;

    assert(o != NULL);
    assert(layer_name != NULL);

    sql = buffer_init();
    column = buffer_init();

    buffer_add_str(sql, "SELECT a.attname ");
    buffer_add_str(sql, "FROM pg_class c, pg_attribute a, pg_type t ");
    buffer_add_str(sql, "WHERE c.relname ='");
    buffer_copy(sql, layer_name);
    buffer_add_str(sql, "' AND a.attnum > 0 AND a.attrelid = c.oid ");
    buffer_add_str(sql, "AND a.atttypid = t.oid AND a.attnum = ");
    buffer_add_int(sql, number);

    res = PQexec(o->pg, sql->buf);
    buffer_free(sql);

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
    if (buffer_cmp(type, "geography"))
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


/*
 * Generate a new buffer id supposed to be unique for a given layer name
 */
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
             sprintf(seed,"%s%03d", seed, fgetc(fp));
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
 * Return the number of rows returned by the specified requests
 */
int ows_psql_number_features(ows * o, list * from, list * where)
{
    buffer *sql;
    PGresult *res;
    list_node *ln_from, *ln_where;
    int nb;

    assert(o != NULL);
    assert(from != NULL);
    assert(where != NULL);

    nb = 0;

    /* checks if from list and where list have the same size */
    if (from->size != where->size) return nb;


    for (ln_from = from->first, ln_where = where->first; ln_from != NULL;
             ln_from = ln_from->next, ln_where = ln_where->next) {
             sql = buffer_init();

             /* execute the request */
             buffer_add_str(sql, "SELECT count(*) FROM \"");
             buffer_copy(sql, ln_from->value);
             buffer_add_str(sql, "\" ");
             buffer_copy(sql, ln_where->value);
             res = PQexec(o->pg, sql->buf);
             buffer_free(sql);

             if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                 PQclear(res);
                 return -1;
             }
             nb = nb + atoi(PQgetvalue(res, 0, 0));
             PQclear(res);
     }

    return nb;
}


static xmlNodePtr ows_psql_recursive_parse_gml(ows * o, xmlNodePtr n)
{
    xmlNodePtr c;
    static xmlNodePtr result=NULL;

    assert(o != NULL);
    assert(n != NULL);

    if (result) return result;  /* avoid recursive loop */

    /* We are looking for the geometry part inside GML doc */
    for (; n ; n = n->next) {

        if (n->type != XML_ELEMENT_NODE) continue;

        /* GML SF Geometries Types */
        if (   !strcmp((char *) n->name, "Point")
            || !strcmp((char *) n->name, "LineString")
            || !strcmp((char *) n->name, "Curve")
            || !strcmp((char *) n->name, "Polygon")
            || !strcmp((char *) n->name, "Surface")
            || !strcmp((char *) n->name, "MultiPoint")
            || !strcmp((char *) n->name, "MultiLineString")
            || !strcmp((char *) n->name, "MultiCurve")
            || !strcmp((char *) n->name, "MultiPolygon")
            || !strcmp((char *) n->name, "MultiSurface")
            || !strcmp((char *) n->name, "MultiGeometry")) {

            return n;
        }
        /* TODO Add check on namespace GML 3 and GML 3.2 */

        /* Recursive exploration */
        if (n->children)
            for (c = n->children ; c ; c = c->next)
                if ((result = ows_psql_recursive_parse_gml(o, c)))
                    return result;
    }

    return NULL;
}


/*
 * Transform a GML geometry to PostGIS EWKT
 * Return NULL on error
 */
buffer * ows_psql_gml_to_sql(ows * o, xmlNodePtr n)
{
    PGresult *res;
    xmlNodePtr g;
    buffer *result, *sql, *gml;

    assert(o != NULL);
    assert(n != NULL);

    g = ows_psql_recursive_parse_gml(o, n);
    if (!g) return NULL;    /* No Geometry founded in GML doc */

    /* Retrieve the sub doc and launch GML parse via PostGIS */
    gml = buffer_init();
    cgi_add_xml_into_buffer(gml, g);
    
    sql = buffer_init();
    buffer_add_str(sql, "SELECT ST_GeomFromGML('");
    buffer_add_str(sql, gml->buf);
    buffer_add_str(sql, "')");

    res = PQexec(o->pg, sql->buf);
    buffer_free(gml);


    /* GML Parse errors cases */
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        buffer_free(sql);
        PQclear(res);
        return NULL;
    }

    result = buffer_init();
    buffer_add_str(result, PQgetvalue(res, 0, 0));
    PQclear(res);

    /* Check if geometry is valid */
    if (o->check_valid_geom) {

        buffer_empty(sql);
        buffer_add_str(sql, "SELECT ST_IsValid('");
        buffer_add_str(sql, result->buf);
        buffer_add_str(sql, "')");

        res = PQexec(o->pg, sql->buf);

        if (   PQresultStatus(res) != PGRES_TUPLES_OK
            || PQntuples(res) != 1
            || (char) PQgetvalue(res, 0, 0)[0] !=  't') {
            buffer_free(sql);
            buffer_free(result);
            PQclear(res);
            return NULL;
        }
    }

    buffer_free(sql);
    PQclear(res);

    return result;
}


/*
 * vim: expandtab sw=4 ts=4
 */
