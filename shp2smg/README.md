`shp2smg` is a tool which converts a polygon shapefile into an `ssm`-compatible
graph.

##### Building #####
1. Install [`shapelib`](http://shapelib.maptools.org/) using standard settings.
2. Call `make`.

##### Usage #####
Give the name of the shapefile as the command line argument, and pipe the
standard output to an `smg` file, like so:

    $ shp2smg shapefile.shp > graph.smg

Notably, in order to have weights (other than a default `1`), there must be a
`dbf` file with the same base name, with a column `POP10`. Also, to be
considered neighbors, polygons must share at least two explicit points.

##### License #####
Available under the GNU GPL, see `LICENSE` file.
