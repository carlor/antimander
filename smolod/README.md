`smolod` is a set of simple utilities to format the output of `ssm`.

##### Building #####
Install shapelib (see `shp2smg`), and `make`.

##### Usage #####
To do anything, you need to save the `ssm` output into a text file with the
extension `.smo`.

There are currently two actions `smolod` can do:
1. Give each tract its district id as a DBF column:

    $ smolod input.smo tracts.dbf colname

2. Create a new shapefile with district polygons:

    $ smolod dmap tracts.shp districts.shp
 
