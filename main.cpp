/*main.cpp*/

//
// Creating a database implemented with
// the AVL-tree data structure that
// can be accessed using common SQL commands.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cassert>

#include "avl.h"
#include "util.h"

using namespace std;


struct node {
    string index;
    avltree<string, streamoff> dataTree;
};


//
// tokenize
//
// Given a string, breaks the string into individual tokens (words) based
// on spaces between the tokens.  Returns the tokens back in a vector.
//
// Example: "select * from students" would be tokenized into a vector
// containing 4 strings:  "select", "*", "from", "students".
//
vector<string> tokenize(string line)
{
    vector<string> tokens;
    stringstream  stream(line);
    string token;

    while (getline(stream, token, ' '))
    {
        tokens.push_back(token);
    }

    return tokens;
}


avltree<string, streamoff> createDatabase( string tablename, int recordSize, int numColumns, int columnNumber) {
	
    string dataname = tablename + ".data";
    ifstream data(dataname, ios::in | ios::binary);
	
    avltree<string, streamoff> dataTree;

    if (!data.good())
    {
        cout << "**Error: couldn't open data file '" << tablename << "'." << endl;
        return dataTree;
    }



    // read file record by record, and store indexed columns and offests into trees (Database)
    data.seekg(0, data.end);  // move to the end to get length of file:
    streamoff length = data.tellg();

    streamoff pos = 0;  // first record at offset 0:
    string    value;

    while (pos < length)
    {
        data.seekg(pos, data.beg);  // move to start of record:

        for (int i = 0; i < numColumns; ++i)  // read values, one per column:
        {
            data >> value;
            if ( i == columnNumber) {
                dataTree.insert(value, pos);
            }
        }

        pos += recordSize;  // move offset to start of next record:
    }

    return dataTree;

}


bool checkValid( vector<string> tokens, vector<string> columns, string tablename) {
	bool isValidSelectCol = false;
	bool isValidWhereCol = false;
	
	if ( tokens.size() > 5) {
		for ( auto x : columns) {
			if ( tokens[1] == x) {
				isValidSelectCol = true;
				break;
			}
		}

		for ( auto x : columns) {
			if ( tokens[5] == x) {
				isValidWhereCol = true;
				break;
			}
		}
	}
	
	
	if (tokens[0] != "select")  
		cout << "Unknown query, ignored..." << endl;
	else if (tokens[0] == "select" && tokens[1] != "*" && !isValidSelectCol)
		cout << "Invalid select column, ignored..." << endl;
	else if (tokens[2] != "from")
		cout << "Invalid select query, ignored..." << endl;
	else if (tokens[3]  != tablename)
		cout << "Invalid table name, ignored..." << endl;
	else if (!isValidWhereCol)
		cout << "Invalid where column, ignored..." << endl;
	else 
		return true;
	
	return false;
}


void printData( vector<string> tokens, vector<string> columns, string tablename, vector<node> trees, int recordSize, int numColumns) {

	vector<string> recordValues;
	vector<streamoff> positions;

	int index = -1; // used as match column or trees<node> index
	bool isIndexed = false;
	
	// check if column is stored in a tree
	for ( size_t i = 0; i < trees.size(); ++i) {
		if ( tokens.at( 5) == trees[ i].index) {
			isIndexed = true; // column found in tree
			index = i;
		}
	}

	if ( isIndexed) { // avltree search
		if ( trees[ index].dataTree.search( tokens.at( 7)) != NULL) {
			positions.push_back( *(trees[ index].dataTree.search( tokens.at( 7))));
		}
	}
	else { // linear search
		
		// find match column (index) for LinearSearch
		for ( size_t i = 0; i < columns.size(); ++i) {
			if ( tokens.at( 5) == columns.at( i)) {
				index = i; // match column found
				break;
			}
		}
		
		positions = LinearSearch( tablename, recordSize, numColumns, tokens.at(7), index+1);
	}


	if ( positions.size() == 0) {
		cout << "Not found..." << endl;
	}

	
	// output desired data
	for ( size_t i = 0; i < positions.size(); ++i) {
		recordValues = GetRecord( tablename, positions[ i], numColumns);
		for ( size_t j = 0; j < recordValues.size(); ++j) {
			if ( tokens.at( 1) == "*") { // print all data
				cout << columns.at( j) << ": ";
				cout << recordValues.at( j) << endl;
			}
			else { // print requested data
				if ( tokens[ 1] == columns[ j]) {
					cout << columns.at( j) << ": ";
					cout << recordValues.at( j) << endl;
				}
			}
		}
	}
}



int main()
{

    string tablename; // = "students";
	
    int recordSize;
    int numColumns;
	
    vector<string> columns;
    vector<bool> isIndexed;

	
    cout << "Welcome to myDB, please enter tablename> ";
    getline(cin, tablename);

    cout << "Reading meta-data..." << endl;
	
    
	
    // read meta-data file and store the data
    
    string metaname = tablename + ".meta";
    ifstream metadata(metaname, ios::in | ios::binary);

    if (!metadata.good()) {
        cout << "**Error: couldn't open data file '" << tablename << "'." << endl;
        return 0;
    }
	
    else { // collect data
        string word;

        metadata >> word;
        recordSize = stoi( word);

        metadata >> word;
        numColumns = stoi( word);

        metadata >> word;
		
        while ( !metadata.eof()) {
            columns.push_back( word);
            metadata >> word;

            isIndexed.push_back( stoi( word));
            metadata >> word;
        }
    }


    cout << "Building index tree(s)..." << endl;

    vector<node> trees;

	// build an avltree for every indexed column
    int columnNumber;
    for ( size_t i = 0; i < columns.size(); ++i) {
        if ( isIndexed.at( i) == 1) {
            columnNumber = i;
            node newTree;
            newTree.index = columns.at( i);
            newTree.dataTree = createDatabase( tablename, recordSize, numColumns, columnNumber);
            trees.push_back( newTree);
        }
    }


	// output the trees' attributes
    for ( size_t i = 0; i < trees.size(); ++i) {
        cout << "Index column: " << trees[ i].index << endl
            << "  Tree size: " << trees[ i].dataTree.size() << endl
            << "  Tree height: " << trees[ i].dataTree.height() << endl;
    }



    //
    // Main loop to input and execute queries from the user:
    //
    string query;

    cout << endl;
    cout << "Enter query> ";
    getline(cin, query);

    while (query != "exit")
    {
        vector<string> tokens = tokenize(query);
        
        // print requested data if query is valid
		if ( checkValid( tokens, columns, tablename)) {
			printData( tokens, columns, tablename, trees, recordSize, numColumns);
		}
			

        cout << endl;
        cout << "Enter query> ";
        getline(cin, query);
    }

    //
    // done:
    //
    return 0;
}


