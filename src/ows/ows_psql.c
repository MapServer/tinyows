/* 
  Copyright (c) <2007> <Barbara Philippot - Olivier Courtin>

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


void ows_psql_prepare(ows * o, buffer * request_name, buffer * parameters,
   buffer * sql)
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
#if 0
	buffer *sql;
	PGresult *res;
	buffer *b, *request_name, *parameters;

	assert(o != NULL);
	assert(layer_name != NULL);

	sql = buffer_init();
	b = buffer_init();

	/* retrieve the column defined like primary key */
	buffer_add_str(sql, "SELECT column_name ");
	buffer_add_str(sql, "FROM information_schema.constraint_column_usage ");
	buffer_add_str(sql, "WHERE table_name = $1 AND constraint_name = (");

	buffer_add_str(sql, "SELECT c.conname ");
	buffer_add_str(sql, "FROM pg_class r, pg_constraint c ");
	buffer_add_str(sql, "WHERE r.oid = c.conrelid AND relname = $1 ");
	buffer_add_str(sql, "AND c.contype = 'p')");

	/* retrieve the column named id */
	buffer_add_str(sql, " UNION ");
	buffer_add_str(sql, "SELECT a.attname ");
	buffer_add_str(sql, "FROM pg_class c, pg_attribute a ");
	buffer_add_str(sql, "WHERE c.relname = $1 ");
	buffer_add_str(sql, "AND a.attname = 'id' AND a.attrelid = c.oid");

	/* initialize the request's name and parameters */
	request_name = buffer_init();
	buffer_add_str(request_name, "id_column");
	parameters = buffer_init();
	buffer_add_str(parameters, "(text)");

	/* check if the request has already been executed */
	if (!in_list(o->psql_requests, request_name))
		ows_psql_prepare(o, request_name, parameters, sql);

	/* execute the request */
	buffer_empty(sql);
	buffer_add_str(sql, "EXECUTE id_column('");
	buffer_copy(sql, layer_name);
	buffer_add_str(sql, "')");

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);
	buffer_free(parameters);
	buffer_free(request_name);

	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
	{
		PQclear(res);
		return b;
	}

	buffer_add_str(b, PQgetvalue(res, 0, 0));
	PQclear(res);

	return b;
#endif
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
	buffer_add_str(sql, "SELECT isvalid(geometryfromtext('");
	buffer_copy(sql, geom);
	buffer_add_str(sql, "', -1));");

	res = PQexec(o->pg, sql->buf);
	buffer_free(sql);

	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
	{
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

	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
	{
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

	if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
	{
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
		return "datetime";
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

    for (ln = o->layers->first; ln != NULL; ln = ln->next)
        if (ln->layer->name != NULL 
	        && ln->layer->storage != NULL
            && ln->layer->name->use == layer_name->use
            && strcmp(ln->layer->name->buf, layer_name->buf) == 0)
            return array_get(ln->layer->storage->attributes, property->buf);

    assert(0); /* should not happen */
    return NULL;
}


#if 0
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
	if (from->size != where->size)
		return nb;

	for (ln_from = from->first, ln_where = where->first;
         ln_from != NULL;
	     ln_from = ln_from->next, ln_where = ln_where->next)
	{
		sql = buffer_init();

		/* execute the request */
		buffer_add_str(sql, "SELECT count(*) FROM \"");
		buffer_copy(sql, ln_from->value);
		buffer_add_str(sql, "\" ");
		buffer_copy(sql, ln_where->value);

		res = PQexec(o->pg, sql->buf);
		buffer_free(sql);

		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			PQclear(res);
			return -1;
		}
		nb = nb + atoi(PQgetvalue(res, 0, 0));
		PQclear(res);
	}

	return nb;
}
#endif


/*
 * vim: expandtab sw=4 ts=4
 */
