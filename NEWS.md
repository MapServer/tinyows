1.2.2   (2024-07-31) *in memory of Olivier Courtin*
 - Include stdlib.h for atoi ([#105](https://github.com/MapServer/tinyows/pull/105)) (Bas Couwenberg)
 
1.2.1   (2024-05-17) *in memory of Olivier Courtin*
 - Fix JSON encoding ([#100](https://github.com/MapServer/tinyows/pull/100)) (Kévin Dubuc)
 - Use stdbool.h instead of custom definitions ([#99](https://github.com/MapServer/tinyows/pull/99)) (Bas Couwenberg)
 - Handle check when using TINYOWS_MAPFILE ([#97](https://github.com/MapServer/tinyows/pull/97)) (Jeff McKenna)
 
1.2.0   (2021-06-11) *in memory of Olivier Courtin*
 - Fix custom types conversion (Vincent Mora)
 - Add support for geometry-less tables (Even Rouault)
 - Do not list non-existing tables (Even Rouault)
 - ows_layer_storage_fill(): fix crash when several bad configured layers are found (Even Rouault)
 - Fix crash when invalid 'pkey' is specified in configuration file (Even Rouault)
 - Implement matchCase for PropertyIsLike filter (Louis Jencka)
 - Fix axis order issue on GetFeature 1.1 (Even Rouault)
 - Avoid repeated prefix of the typename (Even Rouault)
 - fe_distance_functions(): fix crashes / incorrect behaviour (Even Rouault)
 - Fix unable to use separator chars as underscore in typename (Olivier Courtin)
 
1.1.1   (2015-06-29)
 - security release (Olivier Courtin)

1.1.0   (2012-11-13)
 - Add include_items and exclude_items config handling. To choose exactly which columns to retrieve (Serge Dikiy)
 - XSD max length and enumeration constraint handling (Serge Dikiy & Alan Boudreault)
 - First real support of typename namespace provided in request (Olivier Courtin)
 - Generate PK value using PostgresSQL DEFAULT value if present (Serge Dikiy) 
 - Add pkey configuration in layer config. Usefull for instance when retrieving data from a VIEW. (Serge Dikiy)
 - Buffer copy performance improve (Serge Dikiy)
 - Rewrite/fix max features handling (Olivier Courtin)
 - Extent layer's properties allowed to inherit (Olivier Courtin)
 - Several bugfixes as usual (special thanks to Andreas Peri, Serge Dikiy and Jukka Rahkonen for detailled reports)

1.0.0   (2012-02-08)
 - Configuration change with broken backward compatibility:
    * default config file is now /etc/tinyows.xml
    * default schema dir is now $PREFIX/share/tinyows/schema 
    * rename server and prefix to ns_uri ans ns_prefix
    * rename wfs_display_bbox to display_bbox
 - Encoding support, written by Carlos Ruiz: cruizch@gmail.com
 - Estimated_bbox option for GetCapabilities response (default is false)
 - Schema cache for fast-cgi mode (huge performance improvement on transaction operations)
 - Improve drasticaly GetCapabilities performance on huge layer (Thanks to Nicklas Aven for report)
 - Add ability to use different names for layer and storage table (table property)
 - Mapfile config file support (use related TINYOWS_MAPFILE env. var)
 - Debug option available from configure step (--enable-debug) 
 - Improve result from --check option 
 - Add wfs_default_version config file option, to set server default WFS Version
 - Add gml_ns config file option, to set if any, layers properties using GML namespace
 - Add log_level config file option, to allow more granularity in log output
 - PostGIS version init check (support 1.5 and coming 2.0)
 - Update XSD schema (WFS, FE, GML), so need a new 'make install' step if you upgrade
 - CITE WFS-T 1.0.0 SF-0 full compliant (require PostGIS 2.0)
 - CITE WFS-T 1.1.0 SF-0 full compliant (require PostGIS 2.0)
 - Lot of debug stuff (a special thanks to Boris Leukert, Jukka Rahkonen and Even Rouault for detailled reports)
 - Security fixes (SQL Injection vulnerability - Reported by Even Rouault)

0.9.0   (2010-06-19)
 - Fast-CGI support
 - Error log handle
 - Improve --check behaviour
 - And a still a lot of debug stuff

0.8.0   (2010-05-01)
 - PostgreSQL schema support 
 - Command line --check option, to check configure stuff
 - JSON output format for GetFeature (use OUTPUTFORMAT=application/json)
 - PostGIS geography support (Need PostGIS 1.5) 
 - Input request log mechanism
 - Ability to deactivate XSD schema and/or OGC SFS validation 
 - Up to date documentation and OpenLayers integration step by step HowTo
 - Slighlty improve performance for GetFeature operation
 - And a lot of debug stuff
