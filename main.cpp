#include <iostream>
using namespace std;

/*------------------------------*/
/*			INCLUDES			*/
/*------------------------------*/
/* User defined includes */
#include "evoConsoleGameEngine.h"
/* C/C++ STL includes */
#include <stdint.h>
#include <map>
#include <stack>
#include <vector>
#include <chrono>

// Size structure for maze and pixels (1 pixel = 1 unicode character)
struct MazeSize {short int width, height;};
struct PixelSize {short int width, height;};

// Graph class definition
template<class T>
class Graph {
    map<T, vector<T>> la;
public:
	// Add a new Path into graph
    void addPath(T node1, T node2) {
        la[node1].push_back(node2);
        la[node2].push_back(node1);
    }
	// Get all neighbours of the node
    vector<T>& getNeighbours(T node) {
        return la[node];
    }
};

class MazeGameEngine : public ConsoleGameEngine {
	public: 
		// Function for making a console maze of size.width and size.height, pixelSize and pathWidth
		int CreateMaze(const MazeSize size, const PixelSize pixelSize, const short int pathWidth) {
			// Setting the mazeWidth, mazeHeight and pathWidth
			[&](const MazeSize _size, const int _pathWidth) {
				this->mazeWidth = size.width;
				this->mazeHeight = size.height;
				this->pathWidth = _pathWidth;
			}(size, pathWidth);
			// Creating the console which will draw the maze
			return CreateConsole({(short)(size.width*(pathWidth+1)), (short)(size.height*(pathWidth+1))}, pixelSize.width, pixelSize.height);
		}
	private:
		/*---------------------------*/
		/*		POINTER TYPES		 */
		/*---------------------------*/
		int *maze; // The maze
		stack<pair<int, int>> *myStack; // One Stack of position vectors
		Graph<pair<int, int>> *myGraph; // One Graph of labyrinth nodes

		/*--------------------------*/
		/*		INTEGER TYPES		*/
		/*--------------------------*/
		int mazeWidth, mazeHeight;
		short pathWidth;
		int _noCellsVisited;

		/*--------------------------*/
		/*		OTHER TYPES			*/
		/*--------------------------*/
		pair<int, int> cellStarted, firstCell, currCell; // Starting Cell, first Dijkstra Cell, current Dijkstra Cell
		vector<pair<int, pair<int, int>>> table; // Dijkstra's path table

		//Cell Properties
		enum CELL_PROPERTY {
			CELL_PATH_N   = 0x01,
			CELL_PATH_E   = 0x02,
			CELL_PATH_S   = 0x04,
			CELL_PATH_W   = 0x08,
			CELL_VISITED  = 0x10,
			CELL_DIJKSTRA = 0x20
		};

		//2D Position Vector for a node 
		struct point{
			int x, y;
		};

	protected:
		// Method for updating the screen
		virtual bool onUpdateCallback() {
			//At every run we clear the screen
			Sleep(1);
			DrawRectangle(0, 0, console.Width, console.Height, (PIXEL_TYPE)L' ', BG_BLACK);

			//If the user press C once we generate another maze
			while(GetAsyncKeyState(L'C') & 0x8000) {
				if(this->onInitializationRun() && (GetAsyncKeyState(L'C') & 0x8000) == 0) break;
			}

			// Let's generate the maze, shall we?
			// if we didn't visit all maze cells/nodes
			if(_noCellsVisited < mazeWidth*mazeHeight) {
				do {
					// Set the posible unvisited neighbours
					vector<int> neighbours;

					//Take the cardinal point for the direction
					auto cardinalPoint = [&](const bool coord, const point p, const int value) {
						// if it's the said cardinal point valid and we didn't visited the node mark the neighbour as valid
						if(coord && (maze[(myStack->top().second + p.y)*mazeWidth + (myStack->top().first + p.x)] & CELL_VISITED) == 0)
							neighbours.push_back(value);
					};

					//Verify the all cardinal points: N, E, S, W
					cardinalPoint(myStack->top().second >     0,         { 0, -1}, 0); //North
					cardinalPoint(myStack->top().first  < mazeWidth -1,  { 1,  0}, 1); //East
					cardinalPoint(myStack->top().second < mazeHeight-1,  { 0,  1}, 2); //South
					cardinalPoint(myStack->top().first  >     0,         {-1,  0}, 3); //West

					//If we can go in a direction
					if(!neighbours.empty()) {
						//Take a random unvisited direction 
						int dir_next_cell = neighbours[rand()%neighbours.size()];

						//Mark the path and push it to stack and create a Tree (Graph)
						auto switchCase = [&](const int no, const CELL_PROPERTY prop1, const CELL_PROPERTY prop2, const point p) {
							//If direction is the said number
							if(dir_next_cell == no) {
								// Mark the path
								maze[(myStack->top().second + p.y)*mazeWidth + (myStack->top().first + p.x)] |= prop2;
								maze[(myStack->top().second +  0 )*mazeWidth + (myStack->top().first +  0 )] |= prop1;
								// Save it in a Tree (Graph)
								myGraph->addPath(myStack->top(), make_pair(myStack->top().first + p.x, myStack->top().second + p.y));
								// Push the next cell on the stack
								myStack->push(make_pair(myStack->top().first + p.x, myStack->top().second + p.y));
							}
						};

						//Verify all the cardinal points and mark the corespondent paths
						switchCase(0, CELL_PATH_N, CELL_PATH_S, {0, -1}); //North
						switchCase(1, CELL_PATH_E, CELL_PATH_W, {1,  0}); //East
						switchCase(2, CELL_PATH_S, CELL_PATH_N, {0,  1}); //South
						switchCase(3, CELL_PATH_W, CELL_PATH_E, {-1, 0}); //West
						//Mark the current cell as visited and number it
						maze[(myStack->top().second +  0 )*mazeWidth + (myStack->top().first +  0 )] |= CELL_VISITED;
						_noCellsVisited++;
					//If all directions are visited, backtrack
					} else {
						//Pop the stack, we need to go back
						myStack->pop();
					}
				//Skip the generating animation if the 'V' key was pressed 
				} while((GetAsyncKeyState(L'V') & 0x8000) && _noCellsVisited < mazeWidth*mazeHeight);
			} else {
				// Maze has been generated, now we search the shortest path
				// We use Dijkstra's shortest path algorithm on trees
				Sleep(50); //Wait 50 ms between steps
				int currCellIndex = currCell.second*mazeWidth+currCell.first; //Mark the current cell index in the maze
				// If we didn't finished the maze
				if(currCell != myStack->top()) {
					// Mark a posible next cell with the shortest distance (path)
					// We don't have one, so we mark the distance as infinite with no previous cell
					pair<int, pair<int, int>> minDistNextCell = make_pair(INT32_MAX, make_pair(-1, -1));
					
					// Update the information in table
					auto update = [&](pair<int, pair<int, int>>& what, const pair<int, pair<int, int>>& with) {
						what.first  = with.first;
						what.second = with.second;
					};

					// For each neighbour of the current cell (posible one, maximum four)
					for(int i=0; i<myGraph->getNeighbours(currCell).size(); i++) {
						// Calculate the distance from the current cell to next one
						int d = table[currCellIndex].first + 1; 
						// If the distance is less than or equal to the distance from the start to the neighbour then
						if(d <= table[myGraph->getNeighbours(currCell)[i].second*mazeWidth+myGraph->getNeighbours(currCell)[i].first].first)
							// Update the neighbour with the distance via current cell
							update(table[myGraph->getNeighbours(currCell)[i].second*mazeWidth+myGraph->getNeighbours(currCell)[i].first], make_pair(d, currCell));
						// If the distance is less than or equal to the distance of the posible next cell
						if(d <= minDistNextCell.first)
							// Mark the next cell as being the current neighbour of the cell
							update(minDistNextCell, make_pair(d, myGraph->getNeighbours(currCell)[i]));
					}
					// Go to next cell (neighbour)
					currCell = minDistNextCell.second;
				} 
				// Mark the current cell as a Dijkstra cell (cell of the shortest path)
				maze[currCellIndex] |= CELL_DIJKSTRA;
			}

			// Draw the maze Cells
			for(int x=0; x<mazeWidth; x++) {
				for(int y=0; y<mazeHeight; y++) {
					// Draw the path between two cells
					auto DrawCellPath = [&](const point p0, const point p1, const CELL_PROPERTY PATH0, const CELL_PROPERTY PATH1) {
						// Based on direction of the path draw it of the said colour and coordinates
						auto DrawPath = [&](const point p0, const point p1, const CELL_PROPERTY PATH0, const CELL_PROPERTY PATH1, const PIXEL_TYPE glyph, const COLOUR colour) {
							if(maze[y*mazeWidth+x] & PATH0) Draw((pathWidth+1)*x+p0.x, (pathWidth+1)*y+p0.y, glyph, colour);
							if(maze[y*mazeWidth+x] & PATH1) Draw((pathWidth+1)*x+p1.x, (pathWidth+1)*y+p1.y, glyph, colour);
						};
						// If the cell is a Dijkstra Cell (cell of the shortest path)
						if((maze[y*mazeWidth+x] & CELL_DIJKSTRA) != 0) {
							// Draw it as being blue
							DrawPath(p0, p1, PATH0, PATH1, PIXEL_SOLID, FG_BLUE);
						} else {
							// Else, make it white
							DrawPath(p0, p1, PATH0, PATH1, PIXEL_SOLID, FG_WHITE);
						}
					};
					
					//Draw the cells
					for(int py=0; py<pathWidth; py++)
						for(int px=0; px<pathWidth; px++)
						{
							// If it is Dijkstra one, make it blue
							if(maze[y*mazeWidth+x] & CELL_DIJKSTRA) Draw((pathWidth+1)*x+px, (pathWidth+1)*y+py, PIXEL_SOLID, FG_BLUE);
							// Else, if it is visited, make it white
							else if(maze[y*mazeWidth+x] & CELL_VISITED) Draw((pathWidth+1)*x+px, (pathWidth+1)*y+py, PIXEL_SOLID, FG_WHITE);
							// Else, draw it as Dark Blue one
							else Draw((pathWidth+1)*x+px, (pathWidth+1)*y+py, PIXEL_SOLID, FG_DARK_BLUE);
						}
					
					for(int p=0; p<pathWidth; p++) {
						//Draw the paths at East and South of the specific size
						DrawCellPath({pathWidth, p}, {p, pathWidth}, CELL_PATH_E, CELL_PATH_S);
					}
				}
			}
			
			// Draw Unit - the top of the stack
			for(int px=0; px<pathWidth; px++)
				for(int py=0; py<pathWidth; py++)
					Draw(myStack->top().first*(pathWidth+1)+px, myStack->top().second*(pathWidth+1)+py, 0x2588, FG_RED);

			// Draw Unit - the start cell of the maze
			for(int px=0; px<pathWidth; px++)
				for(int py=0; py<pathWidth; py++)
					Draw(cellStarted.first*(pathWidth+1)+px, cellStarted.second*(pathWidth+1)+py, 0x2588, FG_GREEN);

			return true;
		}
		virtual bool onInitializationRun() {

			// Initialize the maze of the size mazeWidth*mazeHeight as one dimension array
			maze = new int[mazeWidth*mazeHeight];
			for(int i=0; i<this->mazeWidth*this->mazeHeight; i++)
				maze[i] = 0;

			// Create a new stack of nodes
			myStack = new stack<pair<int, int>>();
			// Calculate a random starting point and save it
			point startPoint = {rand() % mazeWidth, rand() % mazeHeight};
			cellStarted = make_pair(startPoint.x, startPoint.y);
			// Push it on the stack
			myStack->push(cellStarted);

			// Create a new empty Graph (it will save the maze)
			myGraph = new Graph<pair<int, int>>();
			
			// Mark the current and first Dijkstra Cell
			currCell = firstCell = make_pair(startPoint.x, startPoint.y);
			// Set table size as mazeWidth*mazeHeight of pairs of +infinity and pairs of -1 and -1
			table.resize(mazeWidth*mazeHeight, pair<int, pair<int, int>>(INT32_MAX, make_pair(-1, -1)));
			// Mark the firstCell as being of distance 0 via itself
			table[currCell.second*mazeWidth+currCell.first] = make_pair(0, currCell);
			// Mark the startPoint as visited and set the number of visited cells with 1
			this->maze[startPoint.y*mazeWidth+startPoint.x] = CELL_VISITED;
			this->_noCellsVisited = 1;
			return true;
		}
		virtual bool onKeyboardExit() {
			// If you press Escape
			if(GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
				// Clear the screen
				DrawRectangle(0, 0, console.Width, console.Height, (PIXEL_TYPE)L' ', BG_BLACK);
				// Clear any unnecessary informations from memory
				delete myStack;
				delete myGraph;
				delete[] maze;
				// Close the console
				return true;
			}
			return false;
		}
};

int main(int argc, char const *argv[])
{
	//Seed the random number generator
	srand(time(nullptr));
	//Create a new mazeConsole object
	MazeGameEngine mazeConsole;

	//If we can create the maze
	if(!mazeConsole.CreateMaze({75, 36}, {4, 4}, 3)) {
		//Open the console
		mazeConsole.Start();
	}
	return 0;
}