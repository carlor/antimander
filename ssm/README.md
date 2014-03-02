`ssm` uses a simple algorithm to partition census blocks into districts, in a
manner roughly similar to its namesake the Shortest Splitline Method.

##### Building #####
Just use `make`.

##### Usage #####
First, convert the census blocks into a textual graph format described thus:

    {number of vertices}
    {vert0 weight} {vert0 number of neighbors} {neighbor 0} {neighbor 1} ...
    ...


Run `ssm` thus, with `divs` as the number of districts desired:

    $ ssm divide {divs} < input_graph.txt > output.txt

The output will be a list giving every vertex from `0` to `N-1` a district from
`0` to `divs-1`.

##### License #####
Available under the GNU GPL, see `LICENSE` file.
