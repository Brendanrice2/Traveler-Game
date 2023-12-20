//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2020-12-01, rev. 2023-12-04
//
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.

#include <iostream>
#include <string>
#include <random>
#include <thread>
//
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <climits>
//
#include "gl_frontEnd.h"

//	feel free to "un-use" std if this is against your beliefs.
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
void moveTraveler(Traveler traveler);	// Function to move the traveler
void updateCurrentSegment(TravelerSegment &previousSegment, Direction &newDir, bool &addNewSegment, int travIndex);		// Function to update the current segment
void getNewDirection(vector<Direction> &possibleDirections, int travIndex);		// Function to fill possibleDirections vector
bool boundsCheckObstacles(Direction newDir, int travelerIndex, int segmentIndex);	// Function to check if the traveler can move in the direction
bool checkExit(Direction newDir, int travelerIndex, int segmentIndex);		// Function to check if the traveler has reached the exit
void finishAndTerminateSegment(int &travIndex);		// Function to free all the traveler spaces and remove the traveler from the screen

#if 0
//-----------------------------------------------------------------------------
#pragma mark -
#pragma mark Application-level Global Variables
//-----------------------------------------------------------------------------
#endif

//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
SquareType** grid;
unsigned int numRows = 0;	//	height of the grid
unsigned int numCols = 0;	//	width
unsigned int numTravelers = 0;	//	initial number
unsigned int movesToGrowNewSegment = 0;
unsigned int numTravelersDone = 0;
unsigned int numLiveThreads = 0;		//	the number of live traveler threads
const int headIndex = 0;	//	constant for head index
vector<Traveler> travelerList;
vector<SlidingPartition> partitionList;	
GridPosition	exitPos;	//	location of the exit
vector<thread> threads; // vector of threads
bool stillGoing = true;		//	Boolean to determine if the simulation is still running

//	travelers' sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int travelerSleepTime = 100000;

//	An array of C-string where you can store things you want displayed
//	in the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
time_t launchTime;

//	Random generators:  For uniform distributions
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
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================

void drawTravelers(void)
{
	//-----------------------------
	//	You may have to sychronize things here
	//-----------------------------
	for (unsigned int k=0; k<travelerList.size(); k++)
	{
		//	here I would test if the traveler thread is still live
        if (travelerList[k].stillAlive) {
            drawTraveler(travelerList[k]);
        }
	}
}

void updateMessages(void)
{
	//	Here I hard-code a few messages that I want to see displayed
	//	in my state pane.  The number of live robot threads will
	//	always get displayed.  No need to pass a message about it.
	unsigned int numMessages = 4;
	sprintf(message[0], "We created %d travelers", numTravelers);
	sprintf(message[1], "%d travelers solved the maze", numTravelersDone);
	sprintf(message[2], "I like cheese");
	sprintf(message[3], "Simulation run time: %ld s", time(NULL)-launchTime);
	
	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//---------------------------------------------------------
	drawMessages(numMessages, message);
}

void handleKeyboardEvent(unsigned char c, int x, int y)
{
	int ok = 0;

	switch (c)
	{
		//	'esc' to quit
		case 27:
            stillGoing = false;
            /** Join all the threads */
            for (auto& thread: threads) {
                thread.join();
            }

            exit(0);
			break;

		//	slowdown
		case ',':
			slowdownTravelers();
			ok = 1;
			break;

		//	speedup
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
		//	do something?
	}
}


//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupTravelers(void)
{
	//	decrease sleep time by 20%, but don't get too small
	int newSleepTime = (8 * travelerSleepTime) / 10;
	
	if (newSleepTime > MIN_SLEEP_TIME)
	{
		travelerSleepTime = newSleepTime;
	}
}

void slowdownTravelers(void)
{
	//	increase sleep time by 20%.  No upper limit on sleep time.
	//	We can slow everything down to admistrative pace if we want.
	travelerSleepTime = (12 * travelerSleepTime) / 10;
}




//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function besides
//	initialization of the various global variables and lists
//------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	//	We know that the arguments  of the program  are going
	//	to be the width (number of columns) and height (number of rows) of the
	//	grid, the number of travelers, etc.
	//	So far, I hard code-some values

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

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv);
	
	//	Now we can do application-level initialization
	initializeApplication();

	launchTime = time(NULL);

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();

	
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (unsigned int i=0; i< numRows; i++)
		delete []grid[i];
	delete []grid;
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		delete []message[k];
	delete []message;
	
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


//==================================================================================
//
//	This is a function that you have to edit and add to.
//
//==================================================================================


void initializeApplication(void)
{
	//	Initialize some random generators
	rowGenerator = uniform_int_distribution<unsigned int>(0, numRows-1);
	colGenerator = uniform_int_distribution<unsigned int>(0, numCols-1);

	//	Allocate the grid
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
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	real simulation), only wall/partition location and some color
	srand((unsigned int) time(NULL));

	//	generate a random exit
	exitPos = getNewFreePosition();
	grid[exitPos.row][exitPos.col] = SquareType::EXIT;

	//	Generate walls and partitions
	generateWalls();
	generatePartitions();
	
	//	Initialize traveler info structs
	//	You will probably need to replace/complete this as you add thread-related data
	float** travelerColor = createTravelerColors(numTravelers);
	for (unsigned int k=0; k<numTravelers; k++) {
		GridPosition pos = getNewFreePosition();
		//	Note that treating an enum as a sort of integer is increasingly
		//	frowned upon, as C++ versions progress
		Direction dir = static_cast<Direction>(segmentDirectionGenerator(engine));

		TravelerSegment seg = {pos.row, pos.col, dir};
		Traveler traveler;
		traveler.segmentList.push_back(seg);
		traveler.index = k;
		grid[pos.row][pos.col] = SquareType::TRAVELER;		//Use this line to change the grid
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
	
	//	free array of colors
	for (unsigned int k=0; k<numTravelers; k++)
		delete []travelerColor[k];
	delete []travelerColor;

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
	
	// While the traveler is still alive and the exit has not been found
	while(stillGoing && !exitFound) {
        
		// Get new direction
		getNewDirection(possibleDirections, travIndex);

		// If there are possible directions to move in
        if (!possibleDirections.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> distribution(0, (int) possibleDirections.size() - 1);
            
            newDir = possibleDirections[distribution(gen)];
            possibleDirections.clear();
            
			// Add a new segment if necessary
            if (moveCount == movesToGrowNewSegment || travelerList[travIndex].segmentList.size() == 1) {
                addNewSegment = true;
                moveCount = 0;
            }

			// Check if we've found the exit
            exitFound = checkExit(newDir, travIndex, headIndex);

            if (exitFound) {
				// If we have, finish and terminate the segment
                finishAndTerminateSegment(travIndex);
            } else {
				// If we haven't, update the current segment
                updateCurrentSegment(previousSegment, newDir, addNewSegment, travIndex);
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(travelerSleepTime));
            moveCount++;
        }
	}

}

void finishAndTerminateSegment(int &travIndex) {
    //Freeing all traveler spaces
    for(int k = (static_cast<int>(travelerList[travIndex].segmentList.size()) - 1); k >= 0 ; k--) {
        int tempRow = travelerList[travIndex].segmentList[k].row;
        int tempCol = travelerList[travIndex].segmentList[k].col;
        grid[tempRow][tempCol] = SquareType::FREE_SQUARE;
        travelerList[travIndex].segmentList.pop_back();
        std::this_thread::sleep_for(std::chrono::microseconds(travelerSleepTime));
    }
    
	// Removing the traveler from the screen
    travelerList[travIndex].stillAlive = false; 
    numTravelersDone++;
    numLiveThreads--;

    cout << "Traveler " << travIndex << " has found the exit!" << '\n';
}

void getNewDirection(vector<Direction> &possibleDirections, int travIndex) {

	// Checking if the traveler can move in each direction
	for (Direction dir : {Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST}) {
        if (boundsCheckObstacles(dir, travIndex, headIndex)) {
            possibleDirections.push_back(dir);
        }
    }
}

void updateCurrentSegment(TravelerSegment &previousSegment, Direction &newDir, bool &addNewSegment, int travIndex) {
    
    previousSegment = travelerList[travIndex].segmentList[headIndex];

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
	
    // Updating the rest of the segment
	travelerList[travIndex].segmentList[headIndex].dir = newDir;
	for(unsigned int i = 1; i < travelerList[travIndex].segmentList.size(); i++) {
        // Update the current segment to the previous and store the current segment in the previous
        std::swap(previousSegment, travelerList[travIndex].segmentList[i]);
		
		if(i == travelerList[travIndex].segmentList.size() - 1 && !addNewSegment) {
			grid[previousSegment.row][previousSegment.col] = SquareType::FREE_SQUARE;
		}
	}
    
	// Adding a new segment if necessary
    if (addNewSegment) {
        travelerList[travIndex].segmentList.push_back(previousSegment);
        addNewSegment = false;
    }
}

bool boundsCheckObstacles(Direction newDir, int travelerIndex, int segmentIndex){

	// Declaring variables
    Direction currentDir = travelerList[travelerIndex].segmentList[segmentIndex].dir;
    int row = travelerList[travelerIndex].segmentList[segmentIndex].row;
    int col = travelerList[travelerIndex].segmentList[segmentIndex].col;


	// Checking if the traveler can move in the direction
    if (newDir == Direction::NORTH && currentDir != Direction::SOUTH) {
        return (row > 0 && grid[row - 1][col] == SquareType::FREE_SQUARE) || (row > 0 && grid[row - 1][col] == SquareType::EXIT);
    } else if (newDir == Direction::SOUTH && currentDir != Direction::NORTH) {
        return (row + 1 < static_cast<int>(numRows) && grid[row + 1][col] == SquareType::FREE_SQUARE) || (row + 1 < static_cast<int>(numRows) && grid[row + 1][col] == SquareType::EXIT);
    } else if (newDir == Direction::EAST && currentDir != Direction::WEST) {
        return (col + 1 < static_cast<int>(numCols) && grid[row][col + 1] == SquareType::FREE_SQUARE) || (col + 1 < static_cast<int>(numCols) && grid[row][col + 1] == SquareType::EXIT);
    } else if (newDir == Direction::WEST && currentDir != Direction::EAST) {
        return (col > 0 && grid[row][col - 1] == SquareType::FREE_SQUARE) || (col > 0 && grid[row][col - 1] == SquareType::EXIT);
    } else {
        return false;
    }
}

bool checkExit(Direction newDir, int travelerIndex, int segmentIndex) {

	// Checking if the traveler has reached the exit

	if (newDir == Direction::NORTH) {
		if (travelerList[travelerIndex].segmentList[headIndex].row > 0 && grid[travelerList[travelerIndex].segmentList[headIndex].row - 1][travelerList[travelerIndex].segmentList[headIndex].col] == SquareType::EXIT) {
			return true;
		} else {
			return false;
		}
	} else if (newDir == Direction::SOUTH) {
		if (travelerList[travelerIndex].segmentList[headIndex].row + 1 < numRows && grid[travelerList[travelerIndex].segmentList[headIndex].row + 1][travelerList[travelerIndex].segmentList[headIndex].col] == SquareType::EXIT) {
			return true;
		} else {
			return false;
		}
	} else if (newDir == Direction::EAST) {
		if (travelerList[travelerIndex].segmentList[headIndex].col + 1 < numCols && grid[travelerList[travelerIndex].segmentList[headIndex].row][travelerList[travelerIndex].segmentList[headIndex].col + 1] == SquareType::EXIT) {
			return true;
		} else {
			return false;
		}
	} else if (newDir == Direction::WEST) {
        if (travelerList[travelerIndex].segmentList[headIndex].col > 0 && grid[travelerList[travelerIndex].segmentList[headIndex].row][travelerList[travelerIndex].segmentList[headIndex].col - 1] == SquareType::EXIT) {
			return true;
		} else {
			return false;
		}
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
			if (	currentSeg.row < numRows-1 &&
					grid[currentSeg.row+1][currentSeg.col] == SquareType::FREE_SQUARE)
			{
				newSeg.row = currentSeg.row+1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(Direction::SOUTH);
				grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case Direction::SOUTH:
			if (	currentSeg.row > 0 &&
					grid[currentSeg.row-1][currentSeg.col] == SquareType::FREE_SQUARE)
			{
				newSeg.row = currentSeg.row-1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(Direction::NORTH);
				grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case Direction::WEST:
			if (	currentSeg.col < numCols-1 &&
					grid[currentSeg.row][currentSeg.col+1] == SquareType::FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col+1;
				newSeg.dir = newDirection(Direction::EAST);
				grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case Direction::EAST:
			if (	currentSeg.col > 0 &&
					grid[currentSeg.row][currentSeg.col-1] == SquareType::FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col-1;
				newSeg.dir = newDirection(Direction::WEST);
				grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
				canAdd = true;
			}
			//	no more segment
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

	//	I decide that a wall length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its Direction
	const unsigned int MIN_WALL_LENGTH = 3;
	const unsigned int MAX_HORIZ_WALL_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_WALL_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodWall = true;
	
	//	Generate the vertical walls
	for (unsigned int w=0; w< NUM_WALLS; w++)
	{
		goodWall = false;
		
		//	Case of a vertical wall
		if (headsOrTails(engine))
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;
				
				//	select a column index
				unsigned int HSP = numCols/(NUM_WALLS/2+1);
				unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*HSP;
				unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_WALL_LENGTH-MIN_WALL_LENGTH+1);
				
				//	now a random start row
				unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodWall = false;
				}
				
				//	if the wall first, add it to the grid
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
			
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;
				
				//	select a column index
				unsigned int VSP = numRows/(NUM_WALLS/2+1);
				unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*VSP;
				unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_WALL_LENGTH-MIN_WALL_LENGTH+1);
				
				//	now a random start row
				unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodWall = false;
				}
				
				//	if the wall first, add it to the grid
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

	//	I decide that a partition length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its Direction
	const unsigned int MIN_PARTITION_LENGTH = 3;
	const unsigned int MAX_HORIZ_PART_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_PART_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodPart = true;

	for (unsigned int w=0; w< NUM_PARTS; w++)
	{
		goodPart = false;
		
		//	Case of a vertical partition
		if (headsOrTails(engine))
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;
				
				//	select a column index
				unsigned int HSP = numCols/(NUM_PARTS/2+1);
				unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*HSP + HSP/2;
				unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_PART_LENGTH-MIN_PARTITION_LENGTH+1);
				
				//	now a random start row
				unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodPart = false;
				}
				
				//	if the partition is possible,
				if (goodPart)
				{
					//	add it to the grid and to the partition list
					SlidingPartition part;
					part.isVertical = true;
					for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
					{
						grid[row][col] = SquareType::VERTICAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}
				}
			}
		}
		// case of a horizontal partition
		else
		{
			goodPart = false;
			
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;
				
				//	select a column index
				unsigned int VSP = numRows/(NUM_PARTS/2+1);
				unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*VSP + VSP/2;
				unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_PART_LENGTH-MIN_PARTITION_LENGTH+1);
				
				//	now a random start row
				unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodPart = false;
				}
				
				//	if the wall first, add it to the grid and build SlidingPartition object
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

