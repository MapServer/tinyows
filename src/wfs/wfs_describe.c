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

#include "../ows/ows.h"


/*
 * Describe the layer_name in GML according
 * with PostGIS table definition
 */
static void wfs_complex_type(ows * o, wfs_request * wr,
                             buffer * layer_name)
{
    array *table;
    array_node *an;
    list *mandatory_prop;

    assert(o != NULL);
    assert(wr != NULL);
    assert(layer_name != NULL);

    mandatory_prop = ows_psql_not_null_properties(o, layer_name);

    fprintf(o->output, "<xs:complexType name='");
    buffer_flush(layer_name, o->output);
    fprintf(o->output, "Type'>\n");
    fprintf(o->output, " <xs:complexContent>\n");
    fprintf(o->output,
            "  <xs:extension base='gml:AbstractFeatureType'>\n");
    fprintf(o->output, "   <xs:sequence>\n");

    table = ows_psql_describe_table(o, layer_name);

    assert(table != NULL);

    /* Output the description of the layer_name */
    for (an = table->first; an != NULL; an = an->next) {
            fprintf(o->output, "    <xs:element name ='");
            buffer_flush(an->key, o->output);
            fprintf(o->output, "' type='%s' ", ows_psql_to_xsd(an->value));

            if (in_list(mandatory_prop, an->key))
                fprintf(o->output, "nillable='false' minOccurs='1' ");
            else
                fprintf(o->output, "nillable='true' minOccurs='0' ");

            fprintf(o->output, "maxOccurs='1'/>\n");
    }

    fprintf(o->output, "   </xs:sequence>\n");
    fprintf(o->output, "  </xs:extension>\n");
    fprintf(o->output, " </xs:complexContent>\n");
    fprintf(o->output, "</xs:complexType>\n");
}


/*
 * Execute the DescribeFeatureType request according to version
 * (GML version differ between WFS 1.0.0 and WFS 1.1.0)
 */
void wfs_describe_feature_type(ows * o, wfs_request * wr)
{
    int wfs_version;
    list_node *elemt, *ln;
    list *prefix, *typ;
    buffer *namespace;

    assert(o != NULL);
    assert(wr != NULL);

    wfs_version = ows_version_get(o->request->version);
    prefix = ows_layer_list_prefix(o->layers, wr->typename);
    if (prefix->first == NULL) {
            list_free(prefix);
            ows_error(o, OWS_ERROR_CONFIG_FILE,
                    "Not a single layer is available. Check config file",
                    "describe");
            return;
    }

    if (wr->format == WFS_GML2)
    	fprintf(o->output, "Content-Type: text/xml; subtype=gml/2.1.2\n\n");
    else
    	fprintf(o->output, "Content-Type: text/xml; subtype=gml/3.1.1\n\n");

    fprintf(o->output, "<?xml version='1.0' encoding='%s'?>\n", o->encoding->buf);

    /* if all layers belong to different prefixes, import the matching namespaces */
    if (prefix->first->next != NULL) {
        fprintf(o->output,
                "<xs:schema xmlns:xs='http://www.w3.org/2001/XMLSchema' xmlns='http://www.w3.org/2001/XMLSchema' ");
        fprintf(o->output, "elementFormDefault='qualified'> ");

        for (elemt = prefix->first; elemt != NULL; elemt = elemt->next) {
            namespace = ows_layer_server(o->layers, elemt->value);
            fprintf(o->output, "<xs:import namespace='%s' ",
                    namespace->buf);
            fprintf(o->output, "schemaLocation='%s?service=WFS&amp;version=",
                    o->online_resource->buf);

            if (wfs_version == 100)
                fprintf(o->output, "1.0.0&amp;request=DescribeFeatureType&amp;typename=");
            else
                fprintf(o->output, "1.1.0&amp;request=DescribeFeatureType&amp;typename=");

            /* print the describeFeatureType request with typenames for each prefix */
            typ = ows_layer_list_by_prefix(o->layers, wr->typename, elemt->value);

            for (ln = typ->first; ln != NULL; ln = ln->next) {
                fprintf(o->output, "%s", ln->value->buf);

                if (ln->next != NULL) fprintf(o->output, ",");
            }

            list_free(typ);
            fprintf(o->output, "' />\n\n");
        }

        fprintf(o->output, "</xs:schema>\n");
    }
    /* if all layers belong to the same prefix, print the xsd schema describing features */
    else {
        namespace = ows_layer_server(o->layers, prefix->first->value);
        fprintf(o->output,
                "<xs:schema targetNamespace='%s' ", namespace->buf);
        fprintf(o->output,
                "xmlns:%s='%s' ", prefix->first->value->buf, namespace->buf);
        fprintf(o->output, "xmlns:ogc='http://www.opengis.net/ogc' ");
        fprintf(o->output, "xmlns:xs='http://www.w3.org/2001/XMLSchema' ");
        fprintf(o->output, "xmlns='http://www.w3.org/2001/XMLSchema' ");
        fprintf(o->output, "xmlns:gml='http://www.opengis.net/gml' ");
        fprintf(o->output, "elementFormDefault='qualified' ");

        if (wfs_version == 100)
            fprintf(o->output, "version='1.0'>\n");
        else
            fprintf(o->output, "version='1.1'>\n");

        fprintf(o->output,
                "<xs:import namespace='http://www.opengis.net/gml'");

        if (wfs_version == 100)
            fprintf(o->output,
                    " schemaLocation='http://schemas.opengis.net/gml/2.1.2/feature.xsd'/>\n");
        else
            fprintf(o->output,
                    " schemaLocation='http://schemas.opengis.net/gml/3.1.1/base/gml.xsd'/>\n");

        /* Describe each feature type specified in the request */
        for (elemt = wr->typename->first; elemt != NULL;
                elemt = elemt->next)

        {
            fprintf(o->output, "<xs:element name='");
            buffer_flush(elemt->value, o->output);
            fprintf(o->output, "' type='%s:", prefix->first->value->buf);
            buffer_flush(elemt->value, o->output);
            fprintf(o->output,
                    "Type' substitutionGroup='gml:_Feature' />\n");
            wfs_complex_type(o, wr, elemt->value);
        }

        fprintf(o->output, "</xs:schema>");
    }

    list_free(prefix);
}


/*
 * Generate a WFS Schema related to current Server (all valid layers)
 * with GML XSD import 
 * This is needed by WFS Insert operation validation as libxml2 only handle
 * a single schema validation at a time
 */
buffer * wfs_generate_schema(ows * o)
{
    int wfs_version;
    list_node *elemt, *t;
    list *prefix, *typename;
    buffer *namespace;
    buffer *schema;
    list * layers;

    assert(o != NULL);

    wfs_version = ows_version_get(o->request->version);
    schema = buffer_init();
    layers = ows_layer_list_having_storage(o->layers);

    buffer_add_str(schema, "<?xml version='1.0' encoding='");
    buffer_copy(schema, o->encoding);
    buffer_add_str(schema, "'?>\n");

    prefix = ows_layer_list_prefix(o->layers, layers);

    buffer_add_str(schema, "<xs:schema xmlns:xs='http://www.w3.org/2001/XMLSchema'");
    buffer_add_str(schema, " xmlns='http://www.w3.org/2001/XMLSchema'");
    buffer_add_str(schema, " elementFormDefault='qualified'>\n");

    buffer_add_str(schema, "<xs:import namespace='http://www.opengis.net/wfs' ");
    buffer_add_str(schema, "schemaLocation='");
    buffer_copy(schema, o->schema_dir);
    if (wfs_version == 100) buffer_add_str(schema, WFS_SCHEMA_100_TRANS);
    else buffer_add_str(schema, WFS_SCHEMA_110);

    buffer_add_str(schema, "'/>\n");

    for (elemt = prefix->first; elemt != NULL; elemt = elemt->next) {
        namespace = ows_layer_server(o->layers, elemt->value);
        buffer_add_str(schema, "<xs:import namespace='");
        buffer_copy(schema, namespace);
        buffer_add_str(schema, "' schemaLocation='");

        buffer_copy(schema, o->online_resource);
        buffer_add_str(schema, "?service=WFS&amp;request=DescribeFeatureType");

        if (elemt->next || elemt != prefix->first) {
            buffer_add_str(schema, "&amp;Typename=");

            typename = ows_layer_list_by_prefix(o->layers, layers, elemt->value);
            for (t = typename->first; t != NULL; t = t->next) {
        	buffer_copy(schema, t->value);
	  	if (t->next) buffer_add(schema, ',');
	    }
        } 

        if (wfs_version == 100)
            buffer_add_str(schema, "&amp;version=1.0.0");
        else
            buffer_add_str(schema, "&amp;version=1.1.0");

        buffer_add_str(schema, "'/>\n");
    }

    buffer_add_str(schema, "</xs:schema>");
    list_free(prefix);
    list_free(layers);

    return schema;
}


/*
 * vim: expandtab sw=4 ts=4
 */
