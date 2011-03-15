%{


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <libpq-fe.h>
#include <libxml/xmlreader.h>
#include "../ows_define.h"
#include "../ows_struct.h"
#include "../ows_api.h"

enum map_md_state {
    MAP_MD_UPDATESEQUENCE,
    MAP_MD_SCHEMAS_LOCATION,
    MAP_MD_ABSTRACT,
    MAP_MD_ACCESSCONSTRAINTS,
    MAP_MD_ENCODING,
    MAP_MD_FEES,
    MAP_MD_KEYWORDLIST,
    MAP_MD_MAXFEATURES,
    MAP_MD_NAMESPACE_PREFIX,
    MAP_MD_NAMESPACE_URI,
    MAP_MD_ONLINERESOURCE,
    MAP_MD_TITLE,
    MAP_MD_SRS,
    MAP_MD_FEATURE_COLLECTION,
    MAP_MD_SCHEMA_DIR,
    MAP_MD_SKIP
};

enum map_lmd_state {
    MAP_LMD_EXCLUDE_ITEMS,
    MAP_LMD_FEATURE_ID,
    MAP_LMD_INCLUDE_ITEMS,
    MAP_LMD_XML_ITEMS,
    MAP_LMD_ABSTRACT,
    MAP_LMD_EXTENT,
    MAP_LMD_KEYWORDLIST,
    MAP_LMD_METADATAURL_FORMAT,
    MAP_LMD_METADATAURL_HREF,
    MAP_LMD_METADATAURL_TYPE,
    MAP_LMD_NAMESPACE_PREFIX,
    MAP_LMD_NAMESPACE_URI,
    MAP_LMD_SRS,
    MAP_LMD_TITLE,
    MAP_LMD_TABLE,
    MAP_LMD_SCHEMA,
    MAP_LMD_WRITABLE,
    MAP_LMD_SKIP
};

enum map_prop_state {
    MAP_NAME
};

enum map_layer_prop_state {
    MAP_LAYER_NAME,
    MAP_LAYER_CONNECTION
	
};

/*
 * Global Vars 
 */
ows *map_o = NULL;
ows_layer *map_l = NULL;
list * map_srid = NULL;
buffer *map_ns_prefix = NULL;
buffer *map_ns_uri = NULL;
bool map_is_postgis = false;
enum map_md_state map_md_state;
enum map_lmd_state map_lmd_state;
enum map_prop_state map_prop_state;
enum map_layer_prop_state map_layer_prop_state;




static void metadata_key(char * yytext)
{
	if     (!strncmp("ows_updatesequence", yytext, 18))
		map_md_state = MAP_MD_UPDATESEQUENCE;
	else if(!strncmp("ows_schemas_location", yytext, 20))
		map_md_state = MAP_MD_SCHEMAS_LOCATION;
	else if(!strncmp("wfs_abstract", yytext, 12))
		map_md_state = MAP_MD_ABSTRACT;
	else if(!strncmp("wfs_accessconstraints", yytext, 21))
		map_md_state = MAP_MD_ACCESSCONSTRAINTS;
	else if(!strncmp("wfs_encoding", yytext, 12))
		map_md_state = MAP_MD_ENCODING;
	else if(!strncmp("wfs_fees", yytext, 8))
		map_md_state = MAP_MD_FEES;
	else if(!strncmp("wfs_keywordlist", yytext, 15))
		map_md_state = MAP_MD_KEYWORDLIST;
	else if(!strncmp("wfs_maxfeatures", yytext, 15))
		map_md_state = MAP_MD_MAXFEATURES;
	else if(!strncmp("wfs_namespace_prefix", yytext, 20))
		map_md_state = MAP_MD_NAMESPACE_PREFIX;
	else if(!strncmp("wfs_namespace_uri", yytext, 17))
		map_md_state = MAP_MD_NAMESPACE_URI;
	/* wfs_onlineresource and wfs_service_onlineresource 
           are not used (not the same binary...) */
	else if(!strncmp("tinyows_online_resource", yytext, 23))
		map_md_state = MAP_MD_ONLINERESOURCE;
	else if(!strncmp("wfs_title", yytext, 9))
		map_md_state = MAP_MD_TITLE;
	else if(!strncmp("wfs_srs", yytext, 7))
		map_md_state = MAP_MD_SRS;
	else if(!strncmp("wfs_feature_collection", yytext, 22))
		map_md_state = MAP_MD_FEATURE_COLLECTION;
	else if(!strncmp("tinyows_schema_dir", yytext, 18))
		map_md_state = MAP_MD_SCHEMA_DIR;
	else map_md_state = MAP_MD_SKIP;
}

static void metadata_value(char * yytext)
{
	switch(map_md_state)
	{
		case MAP_MD_SKIP: return;
		case MAP_MD_UPDATESEQUENCE: 	return; /* TODO */
		case MAP_MD_SCHEMAS_LOCATION:	return; /* TODO */
		case MAP_MD_FEATURE_COLLECTION: return; /* TODO */
		case MAP_MD_ABSTRACT:	
			map_o->metadata->abstract = buffer_init();
        		buffer_add_str(map_o->metadata->abstract, yytext);
			return;
		case MAP_MD_ACCESSCONSTRAINTS:
			map_o->metadata->access_constraints = buffer_init();
       			buffer_add_str(map_o->metadata->access_constraints, yytext);	
			return;
		case MAP_MD_ENCODING:
       			buffer_add_str(map_o->encoding, yytext);	
			return;
		case MAP_MD_FEES:
			map_o->metadata->fees = buffer_init();
       			buffer_add_str(map_o->metadata->fees, yytext);	
			return;
		case MAP_MD_KEYWORDLIST:
			map_o->metadata->keywords = list_explode_str(',', yytext);
			return;
		case MAP_MD_MAXFEATURES:
			map_o->max_features = atoi(yytext);
			return;
		case MAP_MD_NAMESPACE_PREFIX:
			map_ns_prefix = buffer_init();
			buffer_add_str(map_ns_prefix, yytext);
			return;
		case MAP_MD_NAMESPACE_URI:
			map_ns_uri = buffer_init();
			buffer_add_str(map_ns_uri, yytext);
			return;
		case MAP_MD_ONLINERESOURCE:
		        buffer_add_str(map_o->online_resource, yytext);
			return;
		case MAP_MD_TITLE:
		    	map_o->metadata->title = buffer_init();
        		buffer_add_str(map_o->metadata->title, yytext);
			return;
		case MAP_MD_SRS:
			map_srid = list_explode_str(',', yytext);
			return;
		case MAP_MD_SCHEMA_DIR:
        		buffer_add_str(map_o->schema_dir, yytext);
			return;
	}
}

static void metadata_layer_key(char * yytext)
{
	     if(!strncmp("gml_exclude_items", yytext, 17))
		map_lmd_state = MAP_LMD_EXCLUDE_ITEMS;
	else if(!strncmp("gml_featureid", yytext, 13) ||
                !strncmp("wfs_featureid", yytext, 13) ||
                !strncmp("ows_featureid", yytext, 13))
		map_lmd_state = MAP_LMD_FEATURE_ID;
	/* TODO groups handle */
	else if(!strncmp("gml_include_items", yytext, 17))
		map_lmd_state = MAP_LMD_INCLUDE_ITEMS;
	/* TODO ALIAS handle */
	/* TODO TYPE handle */
	else if(!strncmp("gml_xml_items", yytext, 13))
		map_lmd_state = MAP_LMD_XML_ITEMS;

	/* Not support gml_geometries 
                       gml_*_type
                       gml_*_occurances
           as i'm not sure it really make sense here... */

	else if(!strncmp("wfs_abstract", yytext, 12))
		map_lmd_state = MAP_LMD_ABSTRACT;
	else if(!strncmp("wfs_extent", yytext, 10))
		map_lmd_state = MAP_LMD_EXTENT;
	else if(!strncmp("wfs_keywordlist", yytext, 15))
		map_lmd_state = MAP_LMD_KEYWORDLIST;

	/* Not sure wfs_namespace_prefix and 
	            wfs_namespace_uri are supported on LAYER METADATA in Mapfile */
	else if(!strncmp("wfs_namespace_prefix", yytext, 20))
		map_lmd_state = MAP_LMD_NAMESPACE_PREFIX;
	else if(!strncmp("wfs_namespace_uri", yytext, 17))
		map_lmd_state = MAP_LMD_NAMESPACE_URI;

	else if(!strncmp("wfs_metadataurl_format", yytext, 22))
		map_lmd_state = MAP_LMD_METADATAURL_FORMAT;
	else if(!strncmp("wfs_metadataurl_href", yytext, 20))
		map_lmd_state = MAP_LMD_METADATAURL_HREF;
	else if(!strncmp("wfs_metadataurl_type", yytext, 20))
		map_lmd_state = MAP_LMD_METADATAURL_TYPE;
	else if(!strncmp("wfs_srs", yytext, 7))
		map_lmd_state = MAP_LMD_SRS;
	else if(!strncmp("wfs_title", yytext, 9))
		map_lmd_state = MAP_LMD_TITLE;
	else if(!strncmp("tinyows_table", yytext, 13))
		map_lmd_state = MAP_LMD_TABLE;
	else if(!strncmp("tinyows_writable", yytext, 16))
		map_lmd_state = MAP_LMD_WRITABLE;
	else map_lmd_state = MAP_LMD_SKIP;
}

static void metadata_layer_value(char * yytext)
{
	switch(map_lmd_state)
	{
	case MAP_LMD_SKIP: return;
	case MAP_LMD_EXCLUDE_ITEMS:
	case MAP_LMD_FEATURE_ID:
	case MAP_LMD_INCLUDE_ITEMS:
	case MAP_LMD_XML_ITEMS:
	case MAP_LMD_EXTENT:
	case MAP_LMD_METADATAURL_FORMAT:
	case MAP_LMD_METADATAURL_HREF:
	case MAP_LMD_METADATAURL_TYPE:
		 /* TODO */ return;

	case MAP_LMD_NAMESPACE_URI:
		buffer_add_str(map_l->ns_uri, yytext);
		return;
	case MAP_LMD_NAMESPACE_PREFIX:
		buffer_add_str(map_l->ns_prefix, yytext);
		return;
	case MAP_LMD_SRS:
		map_l->srid = list_explode_str(',', yytext);
		return;
	case MAP_LMD_KEYWORDLIST:
		map_l->keywords = list_explode_str(',', yytext);
		return;
	case MAP_LMD_ABSTRACT:
		map_l->abstract = buffer_init();
       		buffer_add_str(map_l->abstract, yytext);
		return;
	case MAP_LMD_TITLE:
		map_l->title = buffer_init();
       		buffer_add_str(map_l->title, yytext);
		return;
	case MAP_LMD_SCHEMA:
       		buffer_add_str(map_l->storage->schema, yytext);
		return;
	case MAP_LMD_TABLE:
       		buffer_add_str(map_l->storage->table, yytext);
		return;
	case MAP_LMD_WRITABLE:
		map_l->writable=atoi(yytext)?1:0;
		return;
	}
}

static void properties(char * yytext)
{
	char *p;

	/* remove surrounding quotes */
	for (p = yytext; *p ; p++); *(--p) = 0; yytext++;

	switch(map_prop_state) {
	case MAP_NAME:
		map_o->metadata->name = buffer_init();
       		buffer_add_str(map_o->metadata->name, yytext);
		return;
	}
	
}

static void layer_properties(char * yytext)
{
	char *p;

	/* remove surrounding quotes */
	for (p = yytext; *p ; p++); *(--p) = 0; yytext++;

	switch(map_layer_prop_state) {
	case MAP_LAYER_NAME:
		map_l->name = buffer_init();
       		buffer_add_str(map_l->name, yytext);
		return;
	case MAP_LAYER_CONNECTION:
		/* TODO: Having a Layer connection notion in TinyOWS */
       		buffer_add_str(map_o->pg_dsn, yytext);
		return;
	}
}


static void end_layer()
{
	if (!map_is_postgis) ows_layer_free(map_l);
	else {
		if (!map_l->storage->table->use)
			buffer_copy(map_l->storage->table, map_l->name);
		if (!map_l->storage->schema->use)
			buffer_add_str(map_l->storage->schema, "public");
		ows_layer_list_add(map_o->layers, map_l);
	}
		
	map_is_postgis = false;
}


%}

%option noyywrap
%option nounput
%option never-interactive

%s MAP
%s METADATA
%s PROPERTIES
%s LAYER_PROPERTIES
%s LAYER_METADATA
%s LAYER_CONNECTIONTYPE
%s LAYER_DUMP
%s LAYER
%s FEATURE
%s GRID
%s JOIN
%s CLASS
%s STYLE
%s LABEL
%s PROJECTION
%s KEY_DOUBLE
%s KEY_SINGLE
%s METADATA_MIDDLE
%s LAYER_METADATA_MIDDLE
%s VALUE_DOUBLE
%s VALUE_SINGLE
%s LAYER_KEY_DOUBLE
%s LAYER_KEY_SINGLE
%s LAYER_VALUE_DOUBLE
%s LAYER_VALUE_SINGLE

%%

<INITIAL,MAP,LAYER,METADATA,LAYER_METADATA># {
	 char c;
	 while ( (c = input()) != '\n' || c != EOF ) break;
}

<INITIAL>MAP			{BEGIN MAP;}
<MAP>END			{BEGIN INITIAL;}
<MAP>METADATA 			{BEGIN METADATA;}
<METADATA>END 			{BEGIN MAP;}
<MAP>LAYER 			{BEGIN LAYER; map_l = ows_layer_init();}
<MAP>NAME			{BEGIN PROPERTIES; map_prop_state = MAP_NAME;}
<LAYER>NAME			{BEGIN LAYER_PROPERTIES; map_layer_prop_state = MAP_LAYER_NAME;}
<LAYER>CONNECTION		{BEGIN LAYER_PROPERTIES; map_layer_prop_state = MAP_LAYER_CONNECTION;}
<LAYER>CONNECTIONTYPE		{BEGIN LAYER_CONNECTIONTYPE;}
<LAYER_CONNECTIONTYPE>POSTGIS	{BEGIN LAYER; map_is_postgis=true;}
<LAYER_CONNECTIONTYPE>[A-Za-z]+	{BEGIN LAYER; map_is_postgis=false;}
<LAYER>DUMP			{BEGIN LAYER_DUMP;}
<LAYER_DUMP>TRUE		{BEGIN LAYER; map_l->retrievable = true;}
<LAYER_DUMP>[A-Za-z]+		{BEGIN LAYER; map_l->retrievable = false;}
<LAYER>FEATURE 			{BEGIN FEATURE;}
<FEATURE>END 			{BEGIN LAYER;}
<LAYER>GRID 			{BEGIN GRID;}
<GRID>END 			{BEGIN LAYER;}
<LAYER>PROJECTION 		{BEGIN PROJECTION;}
<PROJECTION>END 		{BEGIN LAYER;}
<LAYER>JOIN 			{BEGIN JOIN;}
<JOIN>END 			{BEGIN LAYER;}
<LAYER>METADATA 		{BEGIN LAYER_METADATA;}
<LAYER_METADATA>END 		{BEGIN LAYER;}
<LAYER>CLASS 			{BEGIN CLASS;}
<CLASS>STYLE 			{BEGIN STYLE;}
<CLASS>LABEL 			{BEGIN LABEL;}
<CLASS>END 			{BEGIN LAYER;}
<STYLE>END 			{BEGIN CLASS;}
<LABEL>END 			{BEGIN CLASS;}
<LAYER>END 			{BEGIN MAP; end_layer();}

<LAYER_METADATA,METADATA,METADATA_MIDDLE,LAYER_METADATA_MIDDLE,LAYER_PROPERTIES>[ \t\n] {}

<METADATA>["]			{BEGIN KEY_DOUBLE;}
<METADATA>[']			{BEGIN KEY_SINGLE;}
<KEY_DOUBLE>["]			{BEGIN METADATA_MIDDLE;}
<KEY_SINGLE>[']			{BEGIN METADATA_MIDDLE;}
<METADATA_MIDDLE>["]		{BEGIN VALUE_DOUBLE;}
<METADATA_MIDDLE>[']		{BEGIN VALUE_SINGLE;}
<VALUE_SINGLE>[']		{BEGIN METADATA;}
<VALUE_DOUBLE>["]		{BEGIN METADATA;}

<LAYER_METADATA>["]		{BEGIN LAYER_KEY_DOUBLE;}
<LAYER_METADATA>[']		{BEGIN LAYER_KEY_SINGLE;}
<LAYER_KEY_DOUBLE>["]		{BEGIN LAYER_METADATA_MIDDLE;}
<LAYER_KEY_SINGLE>[']		{BEGIN LAYER_METADATA_MIDDLE;}
<LAYER_METADATA_MIDDLE>["]	{BEGIN LAYER_VALUE_DOUBLE;}
<LAYER_METADATA_MIDDLE>[']	{BEGIN LAYER_VALUE_SINGLE;}
<LAYER_VALUE_SINGLE>[']		{BEGIN LAYER_METADATA;}
<LAYER_VALUE_DOUBLE>["]		{BEGIN LAYER_METADATA;}

<PROPERTIES>['][^']*[']		{properties(yytext); BEGIN MAP;}
<PROPERTIES>["][^"]*["]		{properties(yytext); BEGIN MAP;}		
<LAYER_PROPERTIES>['][^']*[']	{layer_properties(yytext); BEGIN LAYER;}
<LAYER_PROPERTIES>["][^"]*["]	{layer_properties(yytext); BEGIN LAYER;}		
<KEY_SINGLE>[^']+		{metadata_key(yytext);}
<KEY_DOUBLE>[^"]+		{metadata_key(yytext);}
<LAYER_KEY_SINGLE>[^']+		{metadata_layer_key(yytext);}
<LAYER_KEY_DOUBLE>[^"]+		{metadata_layer_key(yytext);}

<VALUE_DOUBLE>[a-zA-Z0-9.:/ '_-]+		{metadata_value(yytext);}
<VALUE_SINGLE>[a-zA-Z0-9.:/ "_-]+		{metadata_value(yytext);}
<LAYER_VALUE_DOUBLE>[a-zA-Z0-9.:/ '_-]+		{metadata_layer_value(yytext);}
<LAYER_VALUE_SINGLE>[a-zA-Z0-9.:/ "_-]+		{metadata_layer_value(yytext);}
. {}
[ \t\n'"_-] {}

<INITIAL><<EOF>> { yyterminate(); }
<<EOF>> { yyterminate(); ows_error(map_o, OWS_ERROR_CONFIG_FILE, "MapFile parse error", "parse_config_file"); }

%%

void ows_parse_config_mapfile(ows * o, const char *filename)
{
	ows_layer_node *ln;

	assert(o);
	assert(filename);

	if (!(yyin = fopen(filename, "r"))) {
		ows_error(o, OWS_ERROR_CONFIG_FILE, "Unable to open MapFile !", "parse_config_file");
		return;
	}

	map_o = o;
	map_o->metadata = ows_metadata_init();
	map_o->layers = ows_layer_list_init();

 	while( yylex() ) ;

	if (!map_o->db_encoding) {
		map_o->db_encoding = buffer_init();
		buffer_add_str(map_o->db_encoding, OWS_DEFAULT_DB_ENCODING);
	}
	
	if (map_srid) {
		for (ln = o->layers->first ; ln ; ln = ln->next)
			list_add_list(ln->layer->srid, map_srid);
		list_free(map_srid);
	}

	if (map_ns_uri) {
		for (ln = o->layers->first ; ln ; ln = ln->next)
			if (!ln->layer->ns_uri->use)
				buffer_copy(ln->layer->ns_uri, map_ns_uri); 
		buffer_free(map_ns_uri);
	}
	
	if (map_ns_prefix) {
		for (ln = o->layers->first ; ln ; ln = ln->next)
			if (!ln->layer->ns_prefix->use)
				buffer_copy(ln->layer->ns_prefix, map_ns_prefix); 
		buffer_free(map_ns_prefix);
	}
		
	fclose(yyin);
}
