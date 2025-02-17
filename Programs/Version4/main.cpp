//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2020-12-01, rev. 2023-12-04
//
//    This is public domain code.  By all means appropriate it and change is to your
//    heart's content.

#include <iostream>
#include <string>
#include <random>
#include <thread>
//
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <mutex>
//
#include "gl_frontEnd.h"

//    feel free to "un-use" std if this is against your beliefs.
using namespace std;

#if 0
//-----------------------------------------------------------------------------
#pragma mark -
#pragma mark Private Functions' Prototypes
//-----------------------------------------------------------------------------
#endif

void initializeApplication(void);
GridPosition getNewFreePosition(void);
Direction newDirection(Direction forbiddenDir = Direction::NUM_DIRECTIONS);
TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd);
void generateWalls(void);
void generatePartitions(void);
void moveTraveler(Traveler traveler);   // Function to move the traveler
void updateCurrentSegment(TravelerSegment &previousSegment, Direction &newDir, bool &addNewSegment, int travIndex); // Function to update the current segment
void getNewDirection(vector<Direction> &possibleDirections, int travIndex);     // Function to fill possibleDirection vector with possible directions
bool boundsCheckObstacles(Direction newDir, int travelerIndex, int segmentIndex);   // Function to check if the traveler is going out of bounds
bool checkExit(Direction newDir, int travelerIndex, int segmentIndex);      // Function to check if the traveler has reached the exit
void finishAndTerminateSegment(int &travIndex);     // Function to finish and terminate the segment
void checkIfSpaceIsPartition(Direction &newDir, int travIndex, bool &partitionIsNotBlocked, bool &keepMoving);      // Function to check if the space the traveler wants to move to is a partition
void findPartitionsIndex(Direction &newDir, int &index, int &travIndex);       // Function to find the index of the partition
void movePartitionNorth(int &partitionIndex);       // Function to move the partition north
void movePartitionSouth(int &partitionIndex);    // Function to move the partition south
void movePartitionEast(int &partitionIndex);    // Function to move the partition east
void movePartitionWest(int &partitionIndex);    // Function to move the partition west
void moveVerticalPartitionToTheNorth(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked);   // Function to move the vertical partition to the north
void moveHorizontalPartitionToTheNorth(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked, bool &keepMoving);  // Function to move the horizontal partition to the north
void moveVerticalPartitionToTheSouth(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked);  // Function to move the vertical partition to the south
void moveHorizontalPartitionToTheSouth(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked, bool &keepMoving);  // Function to move the horizontal partition to the south
void moveHorizontalPartitionToTheEast(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked); // Function to move the horizontal partition to the east
void moveHorizontalPartitionToTheWest(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked); // Function to move the horizontal partition to the west
void moveVerticalPartitionToTheWest(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked, bool &keepMoving); // Function to move the vertical partition to the west
void moveVerticalPartitionToTheEast(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked, bool &keepMoving); // Function to move the vertical partition to the east

#if 0
//-----------------------------------------------------------------------------
#pragma mark -
#pragma mark Application-level Global Variables
//-----------------------------------------------------------------------------
#endif

//    Don't rename any of these variables
//-------------------------------------
//    The state grid and its dimensions (arguments to the program)
SquareType** grid;
unsigned int numRows = 0;    //    height of the grid
unsigned int numCols = 0;    //    width
unsigned int numTravelers = 0;    //    initial number
unsigned int movesToGrowNewSegment = 0;
unsigned int numTravelersDone = 0;
unsigned int numLiveThreads = 0;        //    the number of live traveler threads
const int headIndex = 0;
vector<Traveler> travelerList;
vector<SlidingPartition> partitionList;
GridPosition    exitPos;    //    location of the exit
vector<thread> threads; // Vector of threads
bool stillGoing = true; // Boolean to keep the threads going
mutex gridLock; // Mutex for the grid
vector<mutex*> travelerLocks;   // Vector of mutexes for each traveler

//    travelers' sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int travelerSleepTime = 500000;

//    An array of C-string where you can store things you want displayed
//    in the state pane to display (for debugging purposes?)
//    Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
time_t launchTime;

//    Random generators:  For uniform distributions
const unsigned int MAX_NUM_INITIAL_SEGMENTS = 6;
random_device randDev;
default_random_engine engine(randDev());
uniform_int_distribution<unsigned int> unsignedNumberGenerator(0, numeric_limits<unsigned int>::max());
uniform_int_distribution<unsigned int> segmentNumberGenerator(0, MAX_NUM_INITIAL_SEGMENTS);
uniform_int_distribution<unsigned int> segmentDirectionGenerator(0, static_cast<unsigned int>(Direction::NUM_DIRECTIONS)-1);
uniform_int_distribution<unsigned int> headsOrTails(0, 1);
uniform_int_distribution<unsigned int> rowGenerator;
uniform_int_distribution<unsigned int> colGenerator;


#if 0
//-----------------------------------------------------------------------------
#pragma mark -
#pragma mark Functions called by the front end
//-----------------------------------------------------------------------------
#endif
//==================================================================================
//    These are the functions that tie the simulation with the rendering.
//    Some parts are "don't touch."  Other parts need your intervention
//    to make sure that access to critical section is properly synchronized
//==================================================================================

void drawTravelers(void)
{
    //-----------------------------
    //    You may have to sychronize things here
    //-----------------------------
    for (unsigned int k=0; k<travelerList.size(); k++)
    {
        //    here I would test if the traveler thread is still live
        if (travelerList[k].stillAlive) {
            drawTraveler(travelerList[k]);
        }
    }
}

void updateMessages(void)
{
    //    Here I hard-code a few messages that I want to see displayed
    //    in my state pane.  The number of live robot threads will
    //    always get displayed.  No need to pass a message about it.
    unsigned int numMessages = 4;
    sprintf(message[0], "We created %d travelers", numTravelers);
    sprintf(message[1], "%d travelers solved the maze", numTravelersDone);
    sprintf(message[2], "I like cheese");
    sprintf(message[3], "Simulation run time: %ld s", time(NULL)-launchTime);
    
    //---------------------------------------------------------
    //    This is the call that makes OpenGL render information
    //    about the state of the simulation.
    //
    //    You *must* synchronize this call.
    //---------------------------------------------------------
    drawMessages(numMessages, message);
}

void handleKeyboardEvent(unsigned char c, int x, int y)
{
    int ok = 0;

    switch (c)
    {
        //    'esc' to quit
        case 27:
            stillGoing = false;
            /** Join all the threads */
            for (auto& thread: threads) {
                thread.join();
            }
            
            // Free the memory for the mutexes
            for (unsigned int i = 0; i < travelerLocks.size(); i++) {
                delete travelerLocks[i];
            }
            exit(0);
            break;

        //    slowdown
        case ',':
            slowdownTravelers();
            ok = 1;
            break;

        //    speedup
        case '.':
            speedupTravelers();
            ok = 1;
            break;

        default:
            ok = 1;
            break;
    }
    if (!ok)
    {
        //    do something?
    }
}


//------------------------------------------------------------------------
//    You shouldn't have to touch this one.  Definitely if you don't
//    add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupTravelers(void)
{
    //    decrease sleep time by 20%, but don't get too small
    int newSleepTime = (8 * travelerSleepTime) / 10;
    
    if (newSleepTime > MIN_SLEEP_TIME)
    {
        travelerSleepTime = newSleepTime;
    }
}

void slowdownTravelers(void)
{
    //    increase sleep time by 20%.  No upper limit on sleep time.
    //    We can slow everything down to admistrative pace if we want.
    travelerSleepTime = (12 * travelerSleepTime) / 10;
}




//------------------------------------------------------------------------
//    You shouldn't have to change anything in the main function besides
//    initialization of the various global variables and lists
//------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    //    We know that the arguments  of the program  are going
    //    to be the width (number of columns) and height (number of rows) of the
    //    grid, the number of travelers, etc.
    //    So far, I hard code-some values

    // Checking arguments
    if (argc != 5 && argc != 4) {
        perror("usage: invalid arguments");
        exit(-1);
    } else if (argc == 4) {
        numRows = stoi(argv[2]);
        numCols = stoi(argv[1]);
        numTravelers = stoi(argv[3]);
        movesToGrowNewSegment = INT_MAX;
    } else {
        numRows = stoi(argv[2]);
        numCols = stoi(argv[1]);
        numTravelers = stoi(argv[3]);
        movesToGrowNewSegment = stoi(argv[4]);
    }
    numLiveThreads = 0;
    numTravelersDone = 0;

    //    Even though we extracted the relevant information from the argument
    //    list, I still need to pass argc and argv to the front-end init
    //    function because that function passes them to glutInit, the required call
    //    to the initialization of the glut library.
    initializeFrontEnd(argc, argv);
    
    //    Now we can do application-level initialization
    initializeApplication();

    launchTime = time(NULL);

    //    Now we enter the main loop of the program and to a large extend
    //    "lose control" over its execution.  The callback functions that
    //    we set up earlier will be called when the corresponding event
    //    occurs
    glutMainLoop();

    
    //    Free allocated resource before leaving (not absolutely needed, but
    //    just nicer.  Also, if you crash there, you know something is wrong
    //    in your code.
    for (unsigned int i=0; i< numRows; i++)
        delete []grid[i];
    delete []grid;
    for (int k=0; k<MAX_NUM_MESSAGES; k++)
        delete []message[k];
    delete []message;
    
    //    This will probably never be executed (the exit point will be in one of the
    //    call back functions).
    return 0;
}


//==================================================================================
//
//    This is a function that you have to edit and add to.
//
//==================================================================================


void initializeApplication(void)
{
    //    Initialize some random generators
    rowGenerator = uniform_int_distribution<unsigned int>(0, numRows-1);
    colGenerator = uniform_int_distribution<unsigned int>(0, numCols-1);

    //    Allocate the grid
    grid = new SquareType*[numRows];
    for (unsigned int i=0; i<numRows; i++)
    {
        grid[i] = new SquareType[numCols];
        for (unsigned int j=0; j< numCols; j++)
            grid[i][j] = SquareType::FREE_SQUARE;
        
    }

    message = new char*[MAX_NUM_MESSAGES];
    for (unsigned int k=0; k<MAX_NUM_MESSAGES; k++)
        message[k] = new char[MAX_LENGTH_MESSAGE+1];
        
    //---------------------------------------------------------------
    //    All the code below to be replaced/removed
    //    I initialize the grid's pixels to have something to look at
    //---------------------------------------------------------------
    //    Yes, I am using the C random generator after ranting in class that the C random
    //    generator was junk.  Here I am not using it to produce "serious" data (as in a
    //    real simulation), only wall/partition location and some color
    srand((unsigned int) time(NULL));

    //    generate a random exit
    exitPos = getNewFreePosition();
    grid[exitPos.row][exitPos.col] = SquareType::EXIT;

    //    Generate walls and partitions
    generateWalls();
    generatePartitions();
    
    //    Initialize traveler info structs
    //    You will probably need to replace/complete this as you add thread-related data
    float** travelerColor = createTravelerColors(numTravelers);
    for (unsigned int k=0; k<numTravelers; k++) {
        GridPosition pos = getNewFreePosition();
        //    Note that treating an enum as a sort of integer is increasingly
        //    frowned upon, as C++ versions progress
        Direction dir = static_cast<Direction>(segmentDirectionGenerator(engine));

        TravelerSegment seg = {pos.row, pos.col, dir};
        Traveler traveler;
        traveler.segmentList.push_back(seg);
        traveler.index = k;
        grid[pos.row][pos.col] = SquareType::TRAVELER;        //Use this line to change the grid
        traveler.stillAlive = true;

        //    I add 0-n segments to my travelers
        unsigned int numAddSegments = segmentNumberGenerator(engine);
        TravelerSegment currSeg = traveler.segmentList[0];
        bool canAddSegment = true;
        cout << "Traveler " << k << " at (row=" << pos.row << ", col=" <<
        pos.col << "), direction: " << dirStr(dir) << ", with up to " << numAddSegments << " additional segments" << endl;
        cout << "\t";

        for (unsigned int s=0; s<numAddSegments && canAddSegment; s++){
            TravelerSegment newSeg = newTravelerSegment(currSeg, canAddSegment);
            if (canAddSegment){
                traveler.segmentList.push_back(newSeg);
                grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
                currSeg = newSeg;
                cout << dirStr(newSeg.dir) << "  ";
            }
        }
        cout << endl;

        for (unsigned int c=0; c<4; c++)
            traveler.rgba[c] = travelerColor[k][c];
        
        travelerList.push_back(traveler);
    }
    
    //    free array of colors
    for (unsigned int k=0; k<numTravelers; k++)
        delete []travelerColor[k];
    delete []travelerColor;
    
    // Initialize the traveler locks
    travelerLocks.resize(numTravelers);
    for (unsigned int i = 0; i < numTravelers; i++) {
        travelerLocks[i] = new mutex();
    }
    
    for (int x = 0; x < static_cast<int>(partitionList.size()); x++) {
        partitionList[x].index = x;
    }

    for(unsigned int i = 0; i < numTravelers; i++) {
        threads.push_back(thread(moveTraveler, travelerList[i]));
        numLiveThreads++;
    }
}

void moveTraveler(Traveler traveler) {
    
    // Declaring variables
    bool exitFound = false;
    TravelerSegment previousSegment;
    Direction newDir;
    vector<Direction> possibleDirections;
    unsigned int moveCount = 0;
    bool addNewSegment = false;
    int travIndex = traveler.index;
    bool partitionIsNotBlocked = true;

    // Seed the RNG
    std::random_device rd;
    std::mt19937 gen(rd());
    
    //   Do until exit is found or the traveler is blocked
    while(stillGoing && !exitFound) {
        
        bool keepMoving = true;
        
        // Lock the grid
        gridLock.lock();

        // Get possible directions
        getNewDirection(possibleDirections, travIndex);

        if (!possibleDirections.empty()) {
            std::uniform_int_distribution<int> distribution(0, (int) possibleDirections.size() - 1);
            
            newDir = possibleDirections[distribution(gen)];
            possibleDirections.clear();
            
            // Check if a partition is in the way
            checkIfSpaceIsPartition(newDir, travIndex, partitionIsNotBlocked, keepMoving);
            
            // Check if we need to add a segment
            if (moveCount == movesToGrowNewSegment || travelerList[travIndex].segmentList.size() == 1) {
                addNewSegment = true;
                moveCount = 0;
            }
            
            // Check if the traveler has reached the exit
            exitFound = checkExit(newDir, travIndex, headIndex);

            if (exitFound) {
                // If so, terminate the traveler
                finishAndTerminateSegment(travIndex);
            } else {
                // Check if a partition is in the way of this move still
                if (partitionIsNotBlocked) { 
                    // If not, update the current segment
                    updateCurrentSegment(previousSegment, newDir, addNewSegment, travIndex);
                } else {
                    partitionIsNotBlocked = true;
                }
            }
            moveCount++;
        }
        
        // Unlock the grid
        gridLock.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(travelerSleepTime));
    }

}


void checkIfSpaceIsPartition(Direction &newDir, int travIndex, bool &partitionIsNotBlocked, bool &keepMoving) {
    int partitionIndex;

    // Check if the space the traveler wants to move to is a partition
    if (newDir == Direction::NORTH && travelerList[travIndex].segmentList[headIndex].row > 0) {
        // Checking North Vertical
        if (grid[travelerList[travIndex].segmentList[headIndex].row - 1][travelerList[travIndex].segmentList[headIndex].col] == SquareType::VERTICAL_PARTITION) {
            moveVerticalPartitionToTheNorth(newDir, travIndex, partitionIndex, partitionIsNotBlocked);
        }
        
        // Check North horizontal
        if (grid[travelerList[travIndex].segmentList[headIndex].row - 1][travelerList[travIndex].segmentList[headIndex].col] == SquareType::HORIZONTAL_PARTITION) {
            moveHorizontalPartitionToTheNorth(newDir, travIndex, partitionIndex, partitionIsNotBlocked, keepMoving);
        }
        
    } else if (newDir == Direction::SOUTH && travelerList[travIndex].segmentList[headIndex].row < numRows - 1) {
        // Checking South Vertical
        if (grid[travelerList[travIndex].segmentList[headIndex].row + 1][travelerList[travIndex].segmentList[headIndex].col] == SquareType::VERTICAL_PARTITION) {
            moveVerticalPartitionToTheSouth(newDir, travIndex, partitionIndex, partitionIsNotBlocked);
        }
        
        // Check South horizontal
        if (grid[travelerList[travIndex].segmentList[headIndex].row + 1][travelerList[travIndex].segmentList[headIndex].col] == SquareType::HORIZONTAL_PARTITION) {
            moveHorizontalPartitionToTheSouth(newDir, travIndex, partitionIndex, partitionIsNotBlocked, keepMoving);
        }

    } else if (newDir == Direction::EAST && travelerList[travIndex].segmentList[headIndex].col < numCols - 1) {
        // Checking East Horizontal
        if (grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[headIndex].col + 1] == SquareType::HORIZONTAL_PARTITION) {
            moveHorizontalPartitionToTheEast(newDir, travIndex, partitionIndex, partitionIsNotBlocked);
        }

        if (grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[headIndex].col + 1] == SquareType::VERTICAL_PARTITION) {
            moveVerticalPartitionToTheEast(newDir, travIndex, partitionIndex, partitionIsNotBlocked, keepMoving);
        }

        //else if
    } else if (newDir == Direction::WEST && travelerList[travIndex].segmentList[headIndex].col > 0) {
        // Checking West Horizontal
        if (grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[headIndex].col - 1] == SquareType::HORIZONTAL_PARTITION) {
            moveHorizontalPartitionToTheWest(newDir, travIndex, partitionIndex, partitionIsNotBlocked);
        }
        
        if (grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[headIndex].col - 1] == SquareType::VERTICAL_PARTITION) {
            moveVerticalPartitionToTheWest(newDir, travIndex, partitionIndex, partitionIsNotBlocked, keepMoving);
        }
    }
}

void moveVerticalPartitionToTheNorth(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked) {
    
    if (grid[travelerList[travIndex].segmentList[headIndex].row - 1][travelerList[travIndex].segmentList[headIndex].col] == SquareType::VERTICAL_PARTITION) {
        findPartitionsIndex(newDir, partitionIndex, travIndex); /* Find partitions index */
        
        if (partitionList[partitionIndex].blockList[headIndex].row > 0 && (grid[partitionList[partitionIndex].blockList[headIndex].row - 1][partitionList[partitionIndex].blockList[headIndex].col]) == SquareType::FREE_SQUARE) {
            
            partitionIsNotBlocked = true;
            movePartitionNorth(partitionIndex); /* Move partition */
        } else {
            partitionIsNotBlocked = false;
        }
    }
}

void moveHorizontalPartitionToTheNorth(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked, bool &keepMoving) {
    findPartitionsIndex(newDir, partitionIndex, travIndex); /* Find partitions index */
    size_t lengthOfPartition = partitionList[partitionIndex].blockList.size();
    bool keepTryingEast = true;
    bool keepTryingWest = true;
    
    while (keepMoving) { /**< This stops if the traveler is able to move to the space it wants or if the space never becomes free */
        while (keepTryingEast) { /**< Try moving to the east */
            if ((partitionList[partitionIndex].blockList[lengthOfPartition - 1].col + 1 < numCols) && ((grid[partitionList[partitionIndex].blockList[lengthOfPartition - 1].row][partitionList[partitionIndex].blockList[lengthOfPartition - 1].col + 1]) == SquareType::FREE_SQUARE)) {
                
                partitionIsNotBlocked = true;
                movePartitionEast(partitionIndex); /* Move partition */
            } else {
                partitionIsNotBlocked = false;
                keepTryingEast = false;
            }
            
            // This stops the movement if the space the traveler is trying to move to is free
            if (grid[travelerList[travIndex].segmentList[headIndex].row - 1][travelerList[travIndex].segmentList[headIndex].col] == SquareType::FREE_SQUARE) {
                keepTryingEast = false;
                keepTryingWest = false;
                keepMoving = false;
            }
        }
        
        while (keepTryingWest) { /**< Try moving to the west */
            if (partitionList[partitionIndex].blockList[headIndex].col > 0 && (grid[partitionList[partitionIndex].blockList[headIndex].row][partitionList[partitionIndex].blockList[headIndex].col - 1]) == SquareType::FREE_SQUARE) {
                partitionIsNotBlocked = true;
                movePartitionWest(partitionIndex); /* Move partition */
            } else {
                partitionIsNotBlocked = false;
                keepTryingWest = false;
                keepMoving = false;
            }
            
            // This stops the movement if the space the traveler is trying to move to is free
            if (grid[travelerList[travIndex].segmentList[headIndex].row - 1][travelerList[travIndex].segmentList[headIndex].col] == SquareType::FREE_SQUARE) {
                keepTryingEast = false;
                keepTryingWest = false;
                keepMoving = false;
            }
        }
    }
}

void moveVerticalPartitionToTheSouth(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked) {
    findPartitionsIndex(newDir, partitionIndex, travIndex); /* Find partitions index */
    size_t lengthOfPartition = partitionList[partitionIndex].blockList.size();
    
    if ((partitionList[partitionIndex].blockList[lengthOfPartition - 1].row + 1 < numRows) && (grid[partitionList[partitionIndex].blockList[lengthOfPartition - 1].row + 1][partitionList[partitionIndex].blockList[lengthOfPartition - 1].col]) == SquareType::FREE_SQUARE) {
        partitionIsNotBlocked = true;
        movePartitionSouth(partitionIndex); /* Move partition */
    } else {
        partitionIsNotBlocked = false;
    }
}

void moveHorizontalPartitionToTheSouth(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked, bool &keepMoving) {
    findPartitionsIndex(newDir, partitionIndex, travIndex); /* Find partitions index */
    size_t lengthOfPartition = partitionList[partitionIndex].blockList.size();
    bool keepTryingEast = true;
    bool keepTryingWest = true;
    
    while (keepMoving) { /**< This stops if the traveler is able to move to the space it wants or if the space never becomes free */
        while (keepTryingEast) { /**< Try moving to the east */
            if ((partitionList[partitionIndex].blockList[lengthOfPartition - 1].col + 1 < numCols) && ((grid[partitionList[partitionIndex].blockList[lengthOfPartition - 1].row][partitionList[partitionIndex].blockList[lengthOfPartition - 1].col + 1]) == SquareType::FREE_SQUARE)) {
                
                partitionIsNotBlocked = true;
                movePartitionEast(partitionIndex); /* Move partition */
            } else {
                partitionIsNotBlocked = false;
                keepTryingEast = false;
            }
            
            // This stops the movement if the space the traveler is trying to move to is free
            if (grid[travelerList[travIndex].segmentList[headIndex].row + 1][travelerList[travIndex].segmentList[headIndex].col] == SquareType::FREE_SQUARE) {
                keepTryingEast = false;
                keepTryingWest = false;
                keepMoving = false;
            }
        }
        
        while (keepTryingWest) { /**< Try moving to the west */
            if (partitionList[partitionIndex].blockList[headIndex].col > 0 && (grid[partitionList[partitionIndex].blockList[headIndex].row][partitionList[partitionIndex].blockList[headIndex].col - 1]) == SquareType::FREE_SQUARE) {
                
                partitionIsNotBlocked = true;
                movePartitionWest(partitionIndex); /* Move partition */
            } else {
                partitionIsNotBlocked = false;
                keepTryingWest = false;
                keepMoving = false;
            }
            
            // This stops the movement if the space the traveler is trying to move to is free
            if (grid[travelerList[travIndex].segmentList[headIndex].row + 1][travelerList[travIndex].segmentList[headIndex].col] == SquareType::FREE_SQUARE) {
                keepTryingEast = false;
                keepTryingWest = false;
                keepMoving = false;
            }
        }
    }
}

void moveHorizontalPartitionToTheEast(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked) {
    findPartitionsIndex(newDir, partitionIndex, travIndex); /* Find partitions index */
    size_t lengthOfPartition = partitionList[partitionIndex].blockList.size();
    
    if ((partitionList[partitionIndex].blockList[lengthOfPartition - 1].col + 1 < numCols) && ((grid[partitionList[partitionIndex].blockList[lengthOfPartition - 1].row][partitionList[partitionIndex].blockList[lengthOfPartition - 1].col + 1]) == SquareType::FREE_SQUARE)) {
        
//                cout << "grid row: " << (partitionList[partitionIndex].blockList[headIndex].row - 1) << ", grid col: " << partitionList[partitionIndex].blockList[headIndex].col << '\n';
        partitionIsNotBlocked = true;
        movePartitionEast(partitionIndex); /* Move partition */
    } else {
        partitionIsNotBlocked = false;
    }
}

void moveHorizontalPartitionToTheWest(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked) {
    if (grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[headIndex].col - 1] == SquareType::HORIZONTAL_PARTITION) {
        findPartitionsIndex(newDir, partitionIndex, travIndex); /* Find partitions index */
        if (partitionList[partitionIndex].blockList[headIndex].col > 0 && (grid[partitionList[partitionIndex].blockList[headIndex].row][partitionList[partitionIndex].blockList[headIndex].col - 1]) == SquareType::FREE_SQUARE) {
            
            partitionIsNotBlocked = true;
            movePartitionWest(partitionIndex); /* Move partition */
        } else {
            partitionIsNotBlocked = false;
        }
    }
}

void moveVerticalPartitionToTheWest(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked, bool &keepMoving) {
    findPartitionsIndex(newDir, partitionIndex, travIndex); /* Find partitions index */
    size_t lengthOfPartition = partitionList[partitionIndex].blockList.size();
    bool keepTryingNorth = true;
    bool keepTryingSouth = true;
    
    while (keepMoving) { /**< This stops if the traveler is able to move to the space it wants or if the space never becomes free */
        while (keepTryingNorth) { /**< Try moving to the north */
            if (partitionList[partitionIndex].blockList[headIndex].row > 0 && (grid[partitionList[partitionIndex].blockList[headIndex].row-1][partitionList[partitionIndex].blockList[headIndex].col]) == SquareType::FREE_SQUARE) {
                
                partitionIsNotBlocked = true;
                movePartitionNorth(partitionIndex); /* Move partition */
            } else {
                partitionIsNotBlocked = false;
                keepTryingNorth = false;
            }
            
            // This stops the movement if the space the traveler is trying to move to is free
            if (grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[headIndex].col - 1] == SquareType::FREE_SQUARE) {
                keepTryingNorth = false;
                keepTryingSouth = false;
                keepMoving = false;
            }
        }
        
        while (keepTryingSouth) { /**< Try moving to the south */
            if (partitionList[partitionIndex].blockList[lengthOfPartition-1].row < (numRows - 1) && (grid[partitionList[partitionIndex].blockList[lengthOfPartition-1].row+1][partitionList[partitionIndex].blockList[lengthOfPartition-1].col]) == SquareType::FREE_SQUARE) {
                
                partitionIsNotBlocked = true;
                movePartitionSouth(partitionIndex); /* Move partition */
            } else {
                partitionIsNotBlocked = false;
                keepTryingSouth = false;
                keepMoving = false;
            }
            
            // This stops the movement if the space the traveler is trying to move to is free
            if (grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[headIndex].col - 1] == SquareType::FREE_SQUARE) {
                keepTryingNorth = false;
                keepTryingSouth = false;
                keepMoving = false;
            }
        }
    }
}

void moveVerticalPartitionToTheEast(Direction &newDir, int &travIndex, int &partitionIndex, bool &partitionIsNotBlocked, bool &keepMoving) {
    findPartitionsIndex(newDir, partitionIndex, travIndex); /* Find partitions index */
    size_t lengthOfPartition = partitionList[partitionIndex].blockList.size();
    bool keepTryingNorth = true;
    bool keepTryingSouth = true;
    
    while (keepMoving) { /**< This stops if the traveler is able to move to the space it wants or if the space never becomes free */
        while (keepTryingNorth) { /**< Try moving to the north */
            if (partitionList[partitionIndex].blockList[headIndex].row > 0 && (grid[partitionList[partitionIndex].blockList[headIndex].row-1][partitionList[partitionIndex].blockList[headIndex].col]) == SquareType::FREE_SQUARE) {
                
                partitionIsNotBlocked = true;
                movePartitionNorth(partitionIndex); /* Move partition */
            } else {
                partitionIsNotBlocked = false;
                keepTryingNorth = false;
            }
            
            // This stops the movement if the space the traveler is trying to move to is free
            if (grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[headIndex].col + 1] == SquareType::FREE_SQUARE) {
                keepTryingNorth = false;
                keepTryingSouth = false;
                keepMoving = false;
            }
        }
        
        while (keepTryingSouth) { /**< Try moving to the south */
            if (partitionList[partitionIndex].blockList[lengthOfPartition-1].row < (numRows - 1) && (grid[partitionList[partitionIndex].blockList[lengthOfPartition-1].row+1][partitionList[partitionIndex].blockList[lengthOfPartition-1].col]) == SquareType::FREE_SQUARE) {
                
                partitionIsNotBlocked = true;
                movePartitionSouth(partitionIndex); /* Move partition */
            } else {
                partitionIsNotBlocked = false;
                keepTryingSouth = false;
                keepMoving = false;
            }
            
            // This stops the movement if the space the traveler is trying to move to is free
            if (grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[headIndex].col + 1] == SquareType::FREE_SQUARE) {
                keepTryingNorth = false;
                keepTryingSouth = false;
                keepMoving = false;
            }
        }
    }
}

void findPartitionsIndex(Direction &newDir, int &index, int &travIndex) {
    
    // Loop through the partitions and match the index to the partition that is currently in the way
    for (SlidingPartition partition : partitionList) {
        
        for (GridPosition gridPos : partition.blockList) {
            
            if ((newDir == Direction::NORTH) && (gridPos.row == travelerList[travIndex].segmentList[headIndex].row - 1) && (gridPos.col == travelerList[travIndex].segmentList[headIndex].col)) {
                index = partition.index;
            } else if ((newDir == Direction::SOUTH) && (gridPos.row == travelerList[travIndex].segmentList[headIndex].row + 1) && (gridPos.col == travelerList[travIndex].segmentList[headIndex].col)) {
                index = partition.index;
            } else if ((newDir == Direction::EAST) && (gridPos.row == travelerList[travIndex].segmentList[headIndex].row) && (gridPos.col == travelerList[travIndex].segmentList[headIndex].col + 1)) {
                index = partition.index;
            } else if ((newDir == Direction::WEST) && (gridPos.row == travelerList[travIndex].segmentList[headIndex].row) && (gridPos.col == travelerList[travIndex].segmentList[headIndex].col - 1)) {
                index = partition.index;
            }
            else if ((newDir == Direction::EAST) && (gridPos.row == travelerList[travIndex].segmentList[headIndex].row) && (gridPos.col == travelerList[travIndex].segmentList[headIndex].col + 1)) {
                index = partition.index;
            }
            else if ((newDir == Direction::WEST) && (gridPos.row == travelerList[travIndex].segmentList[headIndex].row) && (gridPos.col == travelerList[travIndex].segmentList[headIndex].col - 1)) {
                index = partition.index;
            }
        }
    }
}

void movePartitionNorth(int &partitionIndex) {
    for (size_t blockListIndex = 0; blockListIndex < partitionList[partitionIndex].blockList.size(); blockListIndex++) {
        partitionList[partitionIndex].blockList[blockListIndex].row -= 1;
        grid[partitionList[partitionIndex].blockList[blockListIndex].row][partitionList[partitionIndex].blockList[blockListIndex].col] = SquareType::VERTICAL_PARTITION;
        
        // Update the last element in the block list to a free square
        if (blockListIndex == partitionList[partitionIndex].blockList.size() - 1) {
            grid[partitionList[partitionIndex].blockList[blockListIndex].row + 1][partitionList[partitionIndex].blockList[headIndex].col] = SquareType::FREE_SQUARE;
        }
    }
}

void movePartitionSouth(int &partitionIndex) {
    for (int blockListIndex = static_cast<int>(partitionList[partitionIndex].blockList.size() - 1); blockListIndex >= 0; blockListIndex--) {
        partitionList[partitionIndex].blockList[blockListIndex].row += 1;
        grid[partitionList[partitionIndex].blockList[blockListIndex].row][partitionList[partitionIndex].blockList[blockListIndex].col] = SquareType::VERTICAL_PARTITION;
        
        // Update the head element in the block list to a free square
        if (blockListIndex == headIndex) {
            grid[partitionList[partitionIndex].blockList[blockListIndex].row - 1][partitionList[partitionIndex].blockList[headIndex].col] = SquareType::FREE_SQUARE;
        }
    }
}

void movePartitionEast(int &partitionIndex) {
    for (int blockListIndex = static_cast<int>(partitionList[partitionIndex].blockList.size() - 1); blockListIndex >= 0; blockListIndex--) {
        partitionList[partitionIndex].blockList[blockListIndex].col += 1;
        grid[partitionList[partitionIndex].blockList[blockListIndex].row][partitionList[partitionIndex].blockList[blockListIndex].col] = SquareType::HORIZONTAL_PARTITION;
        
        // Update the head element in the block list to a free square
        if (blockListIndex == headIndex) {
            grid[partitionList[partitionIndex].blockList[blockListIndex].row][partitionList[partitionIndex].blockList[headIndex].col - 1] = SquareType::FREE_SQUARE;
        }
    }
}

void movePartitionWest(int &partitionIndex) {
    for (int blockListIndex = 0; blockListIndex < partitionList[partitionIndex].blockList.size(); blockListIndex++) {
        partitionList[partitionIndex].blockList[blockListIndex].col -= 1;
        grid[partitionList[partitionIndex].blockList[blockListIndex].row][partitionList[partitionIndex].blockList[blockListIndex].col] = SquareType::HORIZONTAL_PARTITION;
        
        // Update the last element in the block list to a free square
        if (blockListIndex == (partitionList[partitionIndex].blockList.size() - 1)) {
            grid[partitionList[partitionIndex].blockList[headIndex].row][partitionList[partitionIndex].blockList[blockListIndex].col + 1] = SquareType::FREE_SQUARE;
        }
    }

}

void finishAndTerminateSegment(int &travIndex) {
    travelerLocks[travIndex]->lock();
    //Freeing all traveler spaces
    for(unsigned int k = 0; k < travelerList[travIndex].segmentList.size(); k++) {
        int tempRow = travelerList[travIndex].segmentList[k].row;
        int tempCol = travelerList[travIndex].segmentList[k].col;
        grid[tempRow][tempCol] = SquareType::FREE_SQUARE;
    }
    travelerLocks[travIndex]->unlock();
    
    travelerList[travIndex].stillAlive = false; /* Removes the traveler from the screen */
    numTravelersDone++;
    numLiveThreads--;

    cout << "Traveler " << travIndex << " has found the exit!" << '\n';
    
}

void getNewDirection(vector<Direction> &possibleDirections, int travIndex) {

    // Check North
    for (Direction dir : {Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST}) {
        if (boundsCheckObstacles(dir, travIndex, headIndex)) {
            possibleDirections.push_back(dir);
        }
    }
}

void updateCurrentSegment(TravelerSegment &previousSegment, Direction &newDir, bool &addNewSegment, int travIndex) {
    
    previousSegment = travelerList[travIndex].segmentList[headIndex];
    
    travelerLocks[travIndex]->lock();
    // Updating the head of the segment
    if (newDir == Direction::NORTH) {
        travelerList[travIndex].segmentList[headIndex].row -= 1;
        grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[0].col] = SquareType::TRAVELER;
    } else if (newDir == Direction::SOUTH) {
        travelerList[travIndex].segmentList[headIndex].row += 1;
        grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[0].col] = SquareType::TRAVELER;
    } else if (newDir == Direction::EAST) {
        travelerList[travIndex].segmentList[headIndex].col += 1;
        grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[0].col] = SquareType::TRAVELER;
    } else if (newDir == Direction::WEST) {
        travelerList[travIndex].segmentList[headIndex].col -= 1;
        grid[travelerList[travIndex].segmentList[headIndex].row][travelerList[travIndex].segmentList[0].col] = SquareType::TRAVELER;
    }
    travelerLocks[travIndex]->unlock();
    
    travelerLocks[travIndex]->lock();
    // Updating the rest of the segment
    travelerList[travIndex].segmentList[headIndex].dir = newDir;
    for(unsigned int i = 1; i < travelerList[travIndex].segmentList.size(); i++) {
        // Update the current segment to the previous and store the current segment in the previous
        std::swap(previousSegment, travelerList[travIndex].segmentList[i]);
        
        if(i == travelerList[travIndex].segmentList.size() - 1 && !addNewSegment) {
            grid[previousSegment.row][previousSegment.col] = SquareType::FREE_SQUARE;
        }
    }
    travelerLocks[travIndex]->unlock();
    
    if (addNewSegment) {
        travelerLocks[travIndex]->lock();
        travelerList[travIndex].segmentList.push_back(previousSegment);
        travelerLocks[travIndex]->unlock();
        addNewSegment = false;
    }
}

/**
 Check bounds of the map and the spots around the traveler.
 */
bool boundsCheckObstacles(Direction newDir, int travelerIndex, int segmentIndex){
    Direction currentDir = travelerList[travelerIndex].segmentList[segmentIndex].dir;
    int row = travelerList[travelerIndex].segmentList[segmentIndex].row;
    int col = travelerList[travelerIndex].segmentList[segmentIndex].col;

    if (newDir == Direction::NORTH && currentDir != Direction::SOUTH) {
        return
        (row > 0 && grid[row - 1][col] == SquareType::FREE_SQUARE)
        || (row > 0 && grid[row - 1][col] == SquareType::EXIT)
        || (row > 0 && grid[row - 1][col] == SquareType::VERTICAL_PARTITION)
        || (row > 0 && grid[row - 1][col] == SquareType::HORIZONTAL_PARTITION);
    } else if (newDir == Direction::SOUTH && currentDir != Direction::NORTH) {
        return 
        (row + 1 < static_cast<int>(numRows))
        && (grid[row + 1][col] == SquareType::FREE_SQUARE
        || grid[row + 1][col] == SquareType::EXIT
        || grid[row + 1][col] == SquareType::VERTICAL_PARTITION
        || grid[row + 1][col] == SquareType::HORIZONTAL_PARTITION);
    } else if (newDir == Direction::EAST && currentDir != Direction::WEST) {
        return 
           (col + 1 < static_cast<int>(numCols) && grid[row][col + 1] == SquareType::FREE_SQUARE) 
        || (col + 1 < static_cast<int>(numCols) && grid[row][col + 1] == SquareType::EXIT)
        || (col + 1 < static_cast<int>(numCols) && grid[row][col + 1] == SquareType::VERTICAL_PARTITION)
        || (col + 1 < static_cast<int>(numCols) && grid[row][col + 1] == SquareType::HORIZONTAL_PARTITION);
    } else if (newDir == Direction::WEST && currentDir != Direction::EAST) {
        return 
           (col > 0 && grid[row][col - 1] == SquareType::FREE_SQUARE) 
        || (col > 0 && grid[row][col - 1] == SquareType::EXIT)
        || (col > 0 && grid[row][col - 1] == SquareType::VERTICAL_PARTITION)
        || (col > 0 && grid[row][col - 1] == SquareType::HORIZONTAL_PARTITION);
    } else {
        return false;
    }
}

bool checkExit(Direction newDir, int travelerIndex, int segmentIndex) {

    // Each of these checks the bounds of the map and if the grid space is exit
    if (newDir == Direction::NORTH) {
        return travelerList[travelerIndex].segmentList[headIndex].row > 0 && grid[travelerList[travelerIndex].segmentList[headIndex].row - 1][travelerList[travelerIndex].segmentList[headIndex].col] == SquareType::EXIT;
    } else if (newDir == Direction::SOUTH) {
        return travelerList[travelerIndex].segmentList[headIndex].row + 1 < numRows && grid[travelerList[travelerIndex].segmentList[headIndex].row + 1][travelerList[travelerIndex].segmentList[headIndex].col] == SquareType::EXIT;
    } else if (newDir == Direction::EAST) {
        return travelerList[travelerIndex].segmentList[headIndex].col + 1 < numCols && grid[travelerList[travelerIndex].segmentList[headIndex].row][travelerList[travelerIndex].segmentList[headIndex].col + 1] == SquareType::EXIT;
    } else if (newDir == Direction::WEST) {
        return travelerList[travelerIndex].segmentList[headIndex].col > 0 && grid[travelerList[travelerIndex].segmentList[headIndex].row][travelerList[travelerIndex].segmentList[headIndex].col - 1] == SquareType::EXIT;
    } else {
        return false;
    }
}

//------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Generation Helper Functions
#endif
//------------------------------------------------------

GridPosition getNewFreePosition(void)
{
    GridPosition pos;

    bool noGoodPos = true;
    while (noGoodPos)
    {
        unsigned int row = rowGenerator(engine);
        unsigned int col = colGenerator(engine);
        if (grid[row][col] == SquareType::FREE_SQUARE)
        {
            pos.row = row;
            pos.col = col;
            noGoodPos = false;
        }
    }
    return pos;
}

Direction newDirection(Direction forbiddenDir)
{
    bool noDir = true;

    Direction dir = Direction::NUM_DIRECTIONS;
    while (noDir)
    {
        dir = static_cast<Direction>(segmentDirectionGenerator(engine));
        noDir = (dir==forbiddenDir);
    }
    return dir;
}


TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd)
{
    TravelerSegment newSeg;
    switch (currentSeg.dir)
    {
        case Direction::NORTH:
            if (    currentSeg.row < numRows-1 &&
                    grid[currentSeg.row+1][currentSeg.col] == SquareType::FREE_SQUARE)
            {
                newSeg.row = currentSeg.row+1;
                newSeg.col = currentSeg.col;
                newSeg.dir = newDirection(Direction::SOUTH);
                grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
                canAdd = true;
            }
            //    no more segment
            else
                canAdd = false;
            break;

        case Direction::SOUTH:
            if (    currentSeg.row > 0 &&
                    grid[currentSeg.row-1][currentSeg.col] == SquareType::FREE_SQUARE)
            {
                newSeg.row = currentSeg.row-1;
                newSeg.col = currentSeg.col;
                newSeg.dir = newDirection(Direction::NORTH);
                grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
                canAdd = true;
            }
            //    no more segment
            else
                canAdd = false;
            break;

        case Direction::WEST:
            if (    currentSeg.col < numCols-1 &&
                    grid[currentSeg.row][currentSeg.col+1] == SquareType::FREE_SQUARE)
            {
                newSeg.row = currentSeg.row;
                newSeg.col = currentSeg.col+1;
                newSeg.dir = newDirection(Direction::EAST);
                grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
                canAdd = true;
            }
            //    no more segment
            else
                canAdd = false;
            break;

        case Direction::EAST:
            if (    currentSeg.col > 0 &&
                    grid[currentSeg.row][currentSeg.col-1] == SquareType::FREE_SQUARE)
            {
                newSeg.row = currentSeg.row;
                newSeg.col = currentSeg.col-1;
                newSeg.dir = newDirection(Direction::WEST);
                grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
                canAdd = true;
            }
            //    no more segment
            else
                canAdd = false;
            break;
        
        default:
            canAdd = false;
    }
    
    return newSeg;
}

void generateWalls(void)
{
    const unsigned int NUM_WALLS = (numCols+numRows)/4;

    //    I decide that a wall length  cannot be less than 3  and not more than
    //    1/4 the grid dimension in its Direction
    const unsigned int MIN_WALL_LENGTH = 3;
    const unsigned int MAX_HORIZ_WALL_LENGTH = numCols / 3;
    const unsigned int MAX_VERT_WALL_LENGTH = numRows / 3;
    const unsigned int MAX_NUM_TRIES = 20;

    bool goodWall = true;
    
    //    Generate the vertical walls
    for (unsigned int w=0; w< NUM_WALLS; w++)
    {
        goodWall = false;
        
        //    Case of a vertical wall
        if (headsOrTails(engine))
        {
            //    I try a few times before giving up
            for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
            {
                //    let's be hopeful
                goodWall = true;
                
                //    select a column index
                unsigned int HSP = numCols/(NUM_WALLS/2+1);
                unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*HSP;
                unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_WALL_LENGTH-MIN_WALL_LENGTH+1);
                
                //    now a random start row
                unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
                for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
                {
                    if (grid[row][col] != SquareType::FREE_SQUARE)
                        goodWall = false;
                }
                
                //    if the wall first, add it to the grid
                if (goodWall)
                {
                    for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
                    {
                        grid[row][col] = SquareType::WALL;
                    }
                }
            }
        }
        // case of a horizontal wall
        else
        {
            goodWall = false;
            
            //    I try a few times before giving up
            for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
            {
                //    let's be hopeful
                goodWall = true;
                
                //    select a column index
                unsigned int VSP = numRows/(NUM_WALLS/2+1);
                unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*VSP;
                unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_WALL_LENGTH-MIN_WALL_LENGTH+1);
                
                //    now a random start row
                unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
                for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
                {
                    if (grid[row][col] != SquareType::FREE_SQUARE)
                        goodWall = false;
                }
                
                //    if the wall first, add it to the grid
                if (goodWall)
                {
                    for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
                    {
                        grid[row][col] = SquareType::WALL;
                    }
                }
            }
        }
    }
}


void generatePartitions(void)
{
    const unsigned int NUM_PARTS = (numCols+numRows)/4;

    //    I decide that a partition length  cannot be less than 3  and not more than
    //    1/4 the grid dimension in its Direction
    const unsigned int MIN_PARTITION_LENGTH = 3;
    const unsigned int MAX_HORIZ_PART_LENGTH = numCols / 3;
    const unsigned int MAX_VERT_PART_LENGTH = numRows / 3;
    const unsigned int MAX_NUM_TRIES = 20;

    bool goodPart = true;

    for (unsigned int w=0; w< NUM_PARTS; w++)
    {
        goodPart = false;
        
        //    Case of a vertical partition
        if (headsOrTails(engine))
        {
            //    I try a few times before giving up
            for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
            {
                //    let's be hopeful
                goodPart = true;
                
                //    select a column index
                unsigned int HSP = numCols/(NUM_PARTS/2+1);
                unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*HSP + HSP/2;
                unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_PART_LENGTH-MIN_PARTITION_LENGTH+1);
                
                //    now a random start row
                unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
                for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
                {
                    if (grid[row][col] != SquareType::FREE_SQUARE)
                        goodPart = false;
                }
                
                //    if the partition is possible,
                if (goodPart)
                {
                    //    add it to the grid and to the partition list
                    SlidingPartition part;
                    part.isVertical = true;
                    for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
                    {
                        grid[row][col] = SquareType::VERTICAL_PARTITION;
                        GridPosition pos = {row, col};
                        part.blockList.push_back(pos);
                    }
                    partitionList.push_back(part);
                }
            }
        }
        // case of a horizontal partition
        else
        {
            goodPart = false;
            
            //    I try a few times before giving up
            for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
            {
                //    let's be hopeful
                goodPart = true;
                
                //    select a column index
                unsigned int VSP = numRows/(NUM_PARTS/2+1);
                unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*VSP + VSP/2;
                unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_PART_LENGTH-MIN_PARTITION_LENGTH+1);
                
                //    now a random start row
                unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
                for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
                {
                    if (grid[row][col] != SquareType::FREE_SQUARE)
                        goodPart = false;
                }
                
                //    if the wall first, add it to the grid and build SlidingPartition object
                if (goodPart)
                {
                    SlidingPartition part;
                    part.isVertical = false;
                    for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
                    {
                        grid[row][col] = SquareType::HORIZONTAL_PARTITION;
                        GridPosition pos = {row, col};
                        part.blockList.push_back(pos);
                    }
                    partitionList.push_back(part);
                }
            }
        }
    }
}

