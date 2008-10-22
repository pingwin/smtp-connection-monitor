/***************************************************************************
 *   $Id: XKConfig.c,v 1.1 2008/10/22 16:03:51 pingwin Exp $		   *
 *   Copyright (C) 2008 by Brian Smith					   *
 *   pingwin@gmail.com							   *
 *                                                                         *
 *   Permission is hereby granted, free of charge, to any person obtaining *
 *   a copy of this software and associated documentation files (the       *
 *   "Software"), to deal in the Software without restriction, including   *
 *   without limitation the rights to use, copy, modify, merge, publish,   *
 *   distribute, sublicense, and/or sell copies of the Software, and to    *
 *   permit persons to whom the Software is furnished to do so, subject to *
 *   the following conditions:                                             *
 *                                                                         *
 *   The above copyright notice and this permission notice shall be        *
 *   included in all copies or substantial portions of the Software.       *
 *                                                                         *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
 *   OTHER DEALINGS IN THE SOFTWARE.                                       *
 ***************************************************************************/

#include "XKConfig.h"


// ------------------------------------------------------- TREE DESCRIPTION
// ------------------------------------------------------- TREE DESCRIPTION

// for struct XKCNode->type
enum {
	NODE_TYPE_NODE,
	NODE_TYPE_CHAR,
	NODE_TYPE_INT
};

typedef struct {
	/**
	 * Next node in chain
	 */
	struct {
		/**
		 * Key name for node (identifier)
		 */
		char key[32];

		/**
		 * 0 : NODE_TYPE_NODE : just node in chain, value is next node
		 * 1 : NODE_TYPE_CHAR : char
		 * 2 : NODE_TYPE_INT : int
		 * 3 : ???
		 */
		unsigned int type:2;

		/**
		 * Potential data holders, according on XKCNode.type
		 */
		char c_val[480];
		short c_val_len;

		int i_val;
		/**
		 * Indicator that this node is in use
		 */
		unsigned int set:1;
	} node[ MAX_CONFIG_OPTS ];

	/**
	 * Number of nodes in chain
	 */
	unsigned int num_nodes;

	/**
	 * Key name for node (identifier)
	 */
	char key[32];

	/**
	 * Indicator that this node is in use
	 */
	unsigned int set:1;
} XKCNode;


struct {
	/**
	 * Has tree been initialized
	 */
	unsigned int tree_initialized:1;

	/**
	 * Configuration file location.
	 */
	const char * configPath;

	/**
	 * Number of CLI Arugements
	 */
	int argc;

	/**
	 * CLI Arguements
	 */
	char *argv[];

} XKConfig_Status;


// the XKConfig Tree
static XKCNode XKConfig_Tree[ MAX_CONFIG_GROUPS ];


// ------------------------------------------------------- INITIALIZE TREE
// ------------------------------------------------------- INITIALIZE TREE

static void
XKConfig_initTree() {
	unsigned int i=0;
	unsigned int k=0;

	// --------------------------------------------------
	// Initilize all Nodes in tree
	// --------------------------------------------------
	for (; i<MAX_CONFIG_GROUPS; i ++) {
		XKConfig_Tree[i].set = 0;
		XKConfig_Tree[i].num_nodes = 0;

		for (; k<MAX_CONFIG_OPTS; k ++) {
			XKConfig_Tree[i].node[k].set = 0;
			XKConfig_Tree[i].node[k].type = NODE_TYPE_CHAR;
			XKConfig_Tree[i].node[k].c_val_len = 0;
		}
	}

	XKConfig_Status.tree_initialized = 1;
}

void
XKConfig_Shutdown() {

}



// ------------------------------------------------------- PARSE FILE
// ------------------------------------------------------- PARSE FILE

/**
 *	@short parse a config file and insert it into the XKConfig_Tree
 *	@param char* path to the file to parse
 *	@return unsigned in not zero on error
 */
unsigned int
XKConfig_parseFile(const char * path) {
	if (path == NULL)  return -1;

	// file descriptor
	FILE *fd;

	// key value pairs
	unsigned int keyLen = 32;
	unsigned int valueLen = 480;
	unsigned int bufferLen = 512;

	// for parsing values
	short i = 0;
	short n = 0;
	short k = 0;
	short kp = 0;
	short curr;

	char *key = malloc(keyLen);
	char *value = malloc(valueLen);

	// string buffer -- line
	char *buffer = malloc(bufferLen);
	bzero(buffer, bufferLen);

	char *parent = malloc(keyLen);

	enum {
		KEY,
		VALUE,
		DONE
	};

	if (!XKConfig_Status.tree_initialized)
		XKConfig_initTree();


	// --------------------------------------------------
	// Setup File Descriptor to Read From File
	// --------------------------------------------------
	if ((fd = fopen(path, "r")) == NULL) {
		printf("Failed to open configuration file.\n");
		return -1;
	}

	// --------------------------------------------------
	// Go Through The File Line For Line
	// --------------------------------------------------
	while (fgets(buffer, bufferLen, fd) != NULL) {

		// --------------------------------------------------
		// Comments
		// --------------------------------------------------
		if (buffer[0] == ';' || buffer[0] == '#' || buffer[0] == '\n') {
			bzero(buffer, bufferLen);
			continue; // comments
		}

		buffer[ strlen(buffer)-1 ] = '\0';

		bzero(key, keyLen);

		if (sscanf(buffer, "[%[^#=][32]s]", key) == 1) {
			// --------------------------------------------------
			// New Node
			// --------------------------------------------------
			if (key[ strlen(key)-1 ] == ']') {
				key[ strlen(key)-1 ] = '\0';
			}

			bzero(parent, keyLen);
			strcpy(parent, key);

		} else {
			if (parent == NULL) {
				printf("Config File Format is Invalid.");
				#ifdef AppShutdown
				AppShutdown(EXIT_FAILURE);
				#else
				exit(EXIT_FAILURE);
				#endif
			}

			curr = KEY;
			bzero(value, valueLen);

			for(i=0, n=strlen(buffer)-1; i<=n; i ++) {
				switch(curr) {
				case KEY:
					if (i >= keyLen) {
						printf("Key value too long.");
						#ifdef AppShutdown
						AppShutdown(EXIT_FAILURE);
						#else
						exit(EXIT_FAILURE);
						#endif
					}

					if (buffer[i] == '=' || buffer[i] == ';' || buffer[i] == '#') {
						// position of end of key
						kp = i;

						for(k=0; k<i; k ++) {
							key[k] = buffer[k];
						}

						if (key[k-1] == ' ' || key[k-1] == '\t')
							key[k-1] = '\0';
						curr = VALUE;
					}
					break;

				case VALUE:

					if ( (i-kp) >= valueLen) {
						printf("Value of key is too long\n");
						#ifdef AppShutdown
						AppShutdown(EXIT_FAILURE);
						#else
						exit(EXIT_FAILURE);
						#endif
					}

					if ((i == kp+1) && (buffer[i] == ' ' || buffer[i] == '\t')) {
						kp ++;
						continue;
					}

					if (buffer[i] == ';' || buffer[i] == '#' || i == n) {
						for(k=kp+1; k<=i; k ++) {
							value[k-(kp+1)] = buffer[k];
						}

						// go in reverse to clean up any white space
						for( ; k>kp; k--) {
							if (value[k] == ' ' || value[k] == '\t')
								value[k] = '\0';
						}

						curr = DONE;
					}
					break;
				}
				if (curr == DONE) {
					break; // done
				}
			}

			// DO NOT overwrite already existing parameters
			if (!XKConfig_isDefined(parent, key))
				XKConfig_set(parent, key, value, strlen(value));

		}

		bzero(buffer, bufferLen);
	}

// --------------------------------------------------
// Clean Up
// --------------------------------------------------
	free(key);
	free(value);

	free(parent);
	free(buffer);

	fclose(fd);

	XKConfig_Status.configPath = path;

	return 0;
}


// ------------------------------------------------------- FETCH A VARIABLE OUT OF THE TREE
// ------------------------------------------------------- FETCH A VARIABLE OUT OF THE TREE

const char*
XKConfig_get(const char *parent, const char *node) {
	unsigned int i=0;
	unsigned int k=0;

	// --------------------------------------------------
	// Look through all groups
	// --------------------------------------------------
	for (i=0; i<MAX_CONFIG_GROUPS; i ++) {
		if (!XKConfig_Tree[i].set) continue;

		// --------------------------------------------------
		// If Parent Matches Key we have found the group
		// --------------------------------------------------
		if (strcmp(XKConfig_Tree[i].key, parent) == 0) {

			// --------------------------------------------------
			// Now look through group nodes for matching node
			// --------------------------------------------------
			for (k=0; k<XKConfig_Tree[i].num_nodes; k ++) {
				if (!XKConfig_Tree[i].node[k].set) continue;

				// --------------------------------------------------
				// If nodes match, return value
				// --------------------------------------------------
				if (strcmp(XKConfig_Tree[i].node[k].key, node) == 0) {
					return (const char*)XKConfig_Tree[i].node[k].c_val;
				}
			}
		}
	}

	return NULL;
}


// ------------------------------------------------------- SET A VARIABLE IN THE TREE
// ------------------------------------------------------- SET A VARIABLE IN THE TREE

unsigned int
XKConfig_set(const char * parent, const char * node, const char * value, size_t len) {
	unsigned int i =0;
	unsigned int k =0;

	if (!XKConfig_Status.tree_initialized)
		XKConfig_initTree();

	// --------------------------------------------------
	// Look for open slots in groups
	// --------------------------------------------------
	for (i=0; i<MAX_CONFIG_GROUPS; i ++) {
		if (!XKConfig_Tree[i].set) continue;

		if (strcmp(XKConfig_Tree[i].key, parent) == 0) {
			// --------------------------------------------------
			// Look for Matching Node
			// --------------------------------------------------
			for (k=0; k<XKConfig_Tree[i].num_nodes; k ++) {
				if (!XKConfig_Tree[i].node[k].set) continue;

				// --------------------------------------------------
				// MATCHING NODE
				// --------------------------------------------------
				if (strcmp(XKConfig_Tree[i].node[k].key, node) == 0) {
					XKConfig_Tree[i].node[k].c_val_len = len;
					strcpy(XKConfig_Tree[i].node[k].c_val, value);
					return EXIT_SUCCESS;
				}
			}

			// --------------------------------------------------
			// Node Not Set, Find Next Empty One And Use It
			// --------------------------------------------------
			for (k=0; k<MAX_CONFIG_OPTS; k ++) {
				if (XKConfig_Tree[i].node[k].set) continue;

				// --------------------------------------------------
				// This Will be Set
				// --------------------------------------------------
				XKConfig_Tree[i].node[k].set = 1;
				XKConfig_Tree[i].node[k].type = NODE_TYPE_CHAR;
				XKConfig_Tree[i].num_nodes ++;

				strcpy(XKConfig_Tree[i].node[k].key, node);

				XKConfig_Tree[i].node[k].c_val_len = len;
				strcpy(XKConfig_Tree[i].node[k].c_val, value);
				return EXIT_SUCCESS;
			}
		}

	}

	// --------------------------------------------------
	// Otherwise create a new group.
	// --------------------------------------------------
	for (i=0; i<MAX_CONFIG_GROUPS; i ++) {
		if (XKConfig_Tree[i].set) continue;

		XKConfig_Tree[i].set = 1;
		XKConfig_Tree[i].num_nodes ++;

		strcpy(XKConfig_Tree[i].key, parent);

		XKConfig_Tree[i].node[0].set = 1;
		XKConfig_Tree[i].node[0].type = NODE_TYPE_CHAR;
		XKConfig_Tree[i].node[0].c_val_len = len;

		strcpy(XKConfig_Tree[i].node[0].key, node);
		strcpy(XKConfig_Tree[i].node[0].c_val, value);
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}


// ------------------------------------------------------- PRINT THE CONFIGURATION TREE
// ------------------------------------------------------- PRINT THE CONFIGURATION TREE

void
XKConfig_print() {
	unsigned int i=0;
	unsigned int k=0;

	// --------------------------------------------------
	// Export Tree to STDOUT
	// --------------------------------------------------
	for(; i<MAX_CONFIG_GROUPS; i ++) {
		if (!XKConfig_Tree[i].set) continue;

		printf("\n[%s]\n", XKConfig_Tree[i].key);

		for(k=0; k<XKConfig_Tree[i].num_nodes; k ++) {
			if (!XKConfig_Tree[i].set) continue;
			printf("%s = %s\n", XKConfig_Tree[i].node[k].key, XKConfig_Tree[i].node[k].c_val);
		}
	}
}


unsigned int
XKConfig_isDefined(const char *parent, const char *node) {
	if (XKConfig_get(parent, node) == NULL) {
		return 0;
	}

	return 1;
}


#ifdef STANDALONE
int main(int argc, char *argv[]) {
	XKConfig_parseFile("config.cnf");

	XKConfig_print();


	printf("DBUser: %s\nDBPass: %s\n", XKConfig_get("database", "user"), XKConfig_get("database", "password"));

	return 1;
}
#endif

/* newline */
