# 412-Final-Project
## Final group project repo, Brendan Rice, Jake Murphy

There are 5 different versions of this final project.

To build the final version of this project, run the following command in the terminal: 

```
 g++ -Wall -std=c++20 ./Programs/Version5/*.cpp -lGL -lglut -o final -pthread
```

Building other versions require changing the 'Version5' to 'VersionN' for a given version 'N'

To run the program, run the following command in the terminal:

```
 ./final 10 15 1 8
```

In this example:

final - the name of the executable we build already.

10 - the number of columns in the grid

15 - the number of rows in the grid

1 - the number of travelers in the grid

## Important Notice

Almost all the work we did was within the main.cpp file, with minor changes to other surrounding files within the repo. The other files were provided for us by our Professor.

8 - the number of spaces traveled before a traveler grows

Each of these numbers can be adjusted.
 
